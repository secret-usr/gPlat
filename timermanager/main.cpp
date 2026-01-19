#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/eventfd.h>

#include <iostream>
#include "../include/TimerManager.h"

//关键点说明
//线程安全：
//
//使用互斥锁(pthread_mutex_t)保护堆操作和epoll操作
//
//在访问共享数据结构前加锁，操作完成后解锁
//
//唤醒机制：
//
//使用eventfd作为唤醒机制
//
//当添加新定时器时，写入eventfd唤醒epoll_wait
//
//事件处理：
//
//处理线程负责所有定时器事件的检测和回调执行
//
//主线程或其他线程可以安全地添加定时器
//
//超时计算：
//
//每次epoll_wait前计算最近的定时器到期时间作为超时参数
//
//确保定时器能准时触发
//
//资源清理：
//
//定时器触发后正确清理相关资源(fd、内存等)
//
//所有清理操作都在锁保护下进行
//
//这种实现方式适合需要高性能定时器的多线程应用场景，如网络服务器、游戏服务器等。
void* timer_manager_process_thread(void* arg);

#define HASH_TABLE_SIZE 1024

typedef struct hash_node {
	int key;
	void* value;
	struct hash_node* next;
} hash_node_t;

typedef struct hash_table {
	hash_node_t* buckets[HASH_TABLE_SIZE];
} hash_table_t;

unsigned int hash_func(int key) {
	return key % HASH_TABLE_SIZE;
}

void hash_table_insert(hash_table_t* ht, int key, void* value) {
	unsigned int bucket = hash_func(key);
	hash_node_t* node = (hash_node_t*)malloc(sizeof(hash_node_t));
	node->key = key;
	node->value = value;
	node->next = ht->buckets[bucket];
	ht->buckets[bucket] = node;
}

void* hash_table_get(hash_table_t* ht, int key) {
	unsigned int bucket = hash_func(key);
	hash_node_t* current = ht->buckets[bucket];
	while (current) {
		if (current->key == key) {
			return current->value;
		}
		current = current->next;
	}
	return NULL;
}

void hash_table_remove(hash_table_t* ht, int key) {
	unsigned int bucket = hash_func(key);
	hash_node_t** pp = &ht->buckets[bucket];
	while (*pp) {
		if ((*pp)->key == key) {
			hash_node_t* to_free = *pp;
			*pp = to_free->next;
			free(to_free);
			return;
		}
		pp = &(*pp)->next;
	}
}


// 最小堆实现
typedef struct timer_event {
	int fd;
	void (*callback)(void* arg);
	void* arg;
	uint64_t expire;
} timer_event_t;

typedef struct min_heap {
	timer_event_t** data;
	int size;
	int capacity;
} min_heap_t;

min_heap_t* min_heap_create(int capacity) {
	min_heap_t* heap = (min_heap_t*)malloc(sizeof(min_heap_t));
	if (!heap) return NULL;

	heap->data = (timer_event_t**)malloc(sizeof(timer_event_t*) * capacity);
	if (!heap->data) {
		free(heap);
		return NULL;
	}

	heap->size = 0;
	heap->capacity = capacity;
	return heap;
}

void min_heap_destroy(min_heap_t* heap) {
	if (!heap) return;
	free(heap->data);
	free(heap);
}

void min_heap_swap(min_heap_t* heap, int i, int j) {
	timer_event_t* tmp = heap->data[i];
	heap->data[i] = heap->data[j];
	heap->data[j] = tmp;
}

void min_heap_up(min_heap_t* heap, int i) {
	while (i > 0) {
		int parent = (i - 1) / 2;
		if (heap->data[parent]->expire <= heap->data[i]->expire) break;
		min_heap_swap(heap, parent, i);
		i = parent;
	}
}

void min_heap_down(min_heap_t* heap, int i) {
	while (1) {
		int left = 2 * i + 1;
		int right = 2 * i + 2;
		int smallest = i;

		if (left < heap->size && heap->data[left]->expire < heap->data[smallest]->expire)
			smallest = left;
		if (right < heap->size && heap->data[right]->expire < heap->data[smallest]->expire)
			smallest = right;
		if (smallest == i) break;

		min_heap_swap(heap, i, smallest);
		i = smallest;
	}
}

int min_heap_push(min_heap_t* heap, timer_event_t* event) {
	if (heap->size >= heap->capacity) {
		int new_capacity = heap->capacity * 2;
		timer_event_t** new_data = (timer_event_t**)realloc(heap->data, sizeof(timer_event_t*) * new_capacity);
		if (!new_data) return -1;
		heap->data = new_data;
		heap->capacity = new_capacity;
	}

	heap->data[heap->size] = event;
	min_heap_up(heap, heap->size);
	heap->size++;
	return 0;
}

timer_event_t* min_heap_pop(min_heap_t* heap) {
	if (heap->size == 0) return NULL;

	timer_event_t* event = heap->data[0];
	heap->size--;
	heap->data[0] = heap->data[heap->size];
	min_heap_down(heap, 0);
	return event;
}

timer_event_t* min_heap_top(min_heap_t* heap) {
	if (heap->size == 0) return NULL;
	return heap->data[0];
}

// 在timer_manager结构体中添加哈希表用于快速查找
typedef struct timer_manager {
	min_heap_t* heap;
	int epoll_fd;
	int wakeup_fd;          // 用于唤醒epoll_wait
	pthread_mutex_t lock;  // 保护堆和epoll操作
	volatile int shutdown;
	pthread_t thread_id;
	// 添加哈希表用于快速查找定时器
	hash_table_t* timer_map;  // key: timer_fd, value: timer_event_t*
} timer_manager_t;

timer_manager_t* timer_manager_create() {
	timer_manager_t* tm = (timer_manager_t*)malloc(sizeof(timer_manager_t));
	if (!tm) return NULL;

	tm->heap = min_heap_create(64);
	if (!tm->heap) {
		free(tm);
		return NULL;
	}

	tm->epoll_fd = epoll_create1(0);
	if (tm->epoll_fd == -1) {
		min_heap_destroy(tm->heap);
		free(tm);
		return NULL;
	}

	// 创建eventfd用于唤醒epoll_wait
	tm->wakeup_fd = eventfd(0, EFD_NONBLOCK);
	if (tm->wakeup_fd == -1) {
		close(tm->epoll_fd);
		min_heap_destroy(tm->heap);
		free(tm);
		return NULL;
	}

	// 初始化互斥锁
	if (pthread_mutex_init(&tm->lock, NULL) != 0) {
		close(tm->wakeup_fd);
		close(tm->epoll_fd);
		min_heap_destroy(tm->heap);
		free(tm);
		return NULL;
	}

	// 将wakeup_fd加入epoll监听
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = tm->wakeup_fd;
	if (epoll_ctl(tm->epoll_fd, EPOLL_CTL_ADD, tm->wakeup_fd, &ev) == -1) {
		pthread_mutex_destroy(&tm->lock);
		close(tm->wakeup_fd);
		close(tm->epoll_fd);
		min_heap_destroy(tm->heap);
		free(tm);
		return NULL;
	}

	// 在 timer_manager_create 里添加如下代码
	tm->timer_map = (hash_table_t*)calloc(1, sizeof(hash_table_t));
	if (!tm->timer_map) {
		pthread_mutex_destroy(&tm->lock);
		close(tm->wakeup_fd);
		close(tm->epoll_fd);
		min_heap_destroy(tm->heap);
		free(tm);
		return NULL;
	}

	tm->shutdown = 0;
	return tm;
}

// 启动事件循环线程
int timer_manager_start(timer_manager_t* tm) {
	return pthread_create(&tm->thread_id, NULL, timer_manager_process_thread, tm);
}

// 优雅关闭
void timer_manager_stop(timer_manager_t* tm) {
	if (!tm) return;

	// 设置关闭标志
	tm->shutdown = 1;

	// 唤醒事件循环
	uint64_t one = 1;
	write(tm->wakeup_fd, &one, sizeof(one));

	// 等待线程结束
	pthread_join(tm->thread_id, NULL);
}

void timer_manager_destroy(timer_manager_t* tm) {
	if (!tm) return;

	close(tm->epoll_fd);
	min_heap_destroy(tm->heap);
	free(tm->timer_map);
	free(tm);
}

uint64_t get_current_ms() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

//添加定时器函数修改（线程安全）
int timer_manager_add(timer_manager_t* tm, uint64_t delay_ms, void (*callback)(void*), void* arg) {
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (timer_fd == -1) return -1;

	struct itimerspec its;
	its.it_value.tv_sec = delay_ms / 1000;
	its.it_value.tv_nsec = (delay_ms % 1000) * 1000000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (timerfd_settime(timer_fd, 0, &its, NULL) == -1) {
		close(timer_fd);
		return -1;
	}

	timer_event_t* event = (timer_event_t*)malloc(sizeof(timer_event_t));
	if (!event) {
		close(timer_fd);
		return -1;
	}

	event->fd = timer_fd;
	event->callback = callback;
	event->arg = arg;
	event->expire = get_current_ms() + delay_ms;

	// 加锁保护堆操作和epoll操作
	pthread_mutex_lock(&tm->lock);

	if (min_heap_push(tm->heap, event)) {
		pthread_mutex_unlock(&tm->lock);
		free(event);
		close(timer_fd);
		return -1;
	}

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = event;

	if (epoll_ctl(tm->epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
		// 从堆中移除
		min_heap_pop(tm->heap); // 简化处理，实际需要找到并移除对应事件
		pthread_mutex_unlock(&tm->lock);
		free(event);
		close(timer_fd);
		return -1;
	}

	// 添加到哈希表
	hash_table_insert(tm->timer_map, timer_fd, event);

	// 解锁后唤醒epoll_wait
	pthread_mutex_unlock(&tm->lock);

	// 发送唤醒信号
	uint64_t one = 1;
	write(tm->wakeup_fd, &one, sizeof(one));

	return timer_fd;  // 返回定时器ID用于后续删除
}

int timer_manager_remove(timer_manager_t* tm, int timer_id) {
	if (!tm || timer_id < 0) return -1;

	pthread_mutex_lock(&tm->lock);

	// 从哈希表中查找定时器
	timer_event_t* event = (timer_event_t * )hash_table_get(tm->timer_map, timer_id);
	if (!event) {
		pthread_mutex_unlock(&tm->lock);
		return -1;  // 未找到
	}

	// 从epoll中删除
	epoll_ctl(tm->epoll_fd, EPOLL_CTL_DEL, event->fd, NULL);

	// 从堆中删除（需要遍历堆）
	int found = 0;
	for (int i = 0; i < tm->heap->size; i++) {
		if (tm->heap->data[i] == event) {
			// 将找到的元素与最后一个元素交换
			min_heap_swap(tm->heap, i, tm->heap->size - 1);
			// 移除最后一个元素
			tm->heap->size--;
			// 调整堆
			min_heap_down(tm->heap, i);
			found = 1;
			break;
		}
	}

	if (!found) {
		pthread_mutex_unlock(&tm->lock);
		return -1;
	}

	// 从哈希表中删除
	hash_table_remove(tm->timer_map, timer_id);

	// 释放资源
	close(event->fd);
	free(event);

	pthread_mutex_unlock(&tm->lock);

	return 0;
}

void* timer_manager_process_thread(void* arg) {
	timer_manager_t* tm = (timer_manager_t*)arg;
	struct epoll_event events[64];
	int nfds;

	while (!tm->shutdown) {
		// 计算下一个定时器的等待时间
		pthread_mutex_lock(&tm->lock);
		timer_event_t* top = min_heap_top(tm->heap);
		pthread_mutex_unlock(&tm->lock);

		int timeout = -1; // 默认无限等待
		if (top) {
			uint64_t now = get_current_ms();
			if (top->expire > now) {
				timeout = (int)(top->expire - now);
			}
			else {
				timeout = 0; // 立即处理
			}
		}

		nfds = epoll_wait(tm->epoll_fd, events, 64, timeout);
		if (nfds == -1) {
			if (errno == EINTR) continue;
			perror("epoll_wait");
			break;
		}

		// 处理事件
		for (int i = 0; i < nfds; i++) {
			if (events[i].data.fd == tm->wakeup_fd) {
				// 唤醒事件，读取并继续
				uint64_t dummy;
				read(tm->wakeup_fd, &dummy, sizeof(dummy));
				continue;
			}

			timer_event_t* event = (timer_event_t*)events[i].data.ptr;

			// 读取timerfd
			uint64_t expirations;
			read(event->fd, &expirations, sizeof(expirations));

			// 执行回调
			event->callback(event->arg);

			// 加锁清理资源
			pthread_mutex_lock(&tm->lock);
			epoll_ctl(tm->epoll_fd, EPOLL_CTL_DEL, event->fd, NULL);
			min_heap_pop(tm->heap); // 简化处理
			close(event->fd);
			free(event);
			pthread_mutex_unlock(&tm->lock);
		}

		// 处理可能漏掉的超时事件
		pthread_mutex_lock(&tm->lock);
		while (1) {
			timer_event_t* top = min_heap_top(tm->heap);
			if (!top) break;

			uint64_t now = get_current_ms();
			if (top->expire > now) break;

			// 从epoll中删除
			epoll_ctl(tm->epoll_fd, EPOLL_CTL_DEL, top->fd, NULL);

			// 执行回调
			top->callback(top->arg);

			// 从堆中删除
			min_heap_pop(tm->heap);

			// 清理资源
			close(top->fd);
			free(top);
		}
		pthread_mutex_unlock(&tm->lock);
	}

	return NULL;
}

// 示例回调函数
void timer_callback(void* arg) {
	printf("Timer expired! Argument: %s\n", (char*)arg);
}

int main1() {
	timer_manager_t* tm = timer_manager_create();
	timer_manager_start(tm);

	// 添加定时器并保存ID
    int timer1 = timer_manager_add(tm, 1000, timer_callback, (void*)"Timer 1");
	int timer2 = timer_manager_add(tm, 2000, timer_callback, (void*)"Timer 2");

	// 取消timer2
	//timer_manager_remove(tm, timer2);

	sleep(5);

	// 优雅退出
	timer_manager_stop(tm);
	timer_manager_destroy(tm);

	return 0;
}

int main() {
	try {
		TimerManager tm;

		tm.start();  // 启动定时器管理器线程

		// 添加定时器
		auto timer1 = tm.addTimer(1000, [](void*) {
			std::cout << "Timer 1 fired after 1 second" << std::endl;
			}, nullptr);

		auto timer2 = tm.addTimer(2000, [](void*) {
			std::cout << "Timer 2 fired after 2 seconds" << std::endl;
			}, nullptr);

		// 3秒后取消timer2
		//std::this_thread::sleep_for(std::chrono::seconds(1));
		//if (tm.removeTimer(timer2)) {
		//	std::cout << "Timer 2 cancelled successfully" << std::endl;
		//}

		// 等待剩余定时器执行
		std::this_thread::sleep_for(std::chrono::seconds(5));

		tm.stop();  // 停止定时器管理器线程
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}