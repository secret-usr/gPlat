#pragma once

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <thread> // Ensure this header is included for std::thread
#include <stdexcept> // Include this for std::runtime_error
#include <sys/eventfd.h>


class TimerManager {
public:
	using TimerCallback = std::function<void(void*)>;

	TimerManager() :
		epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
		timer_fd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
		wakeup_fd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
		running_(false) {

		if (epoll_fd_ == -1 || timer_fd_ == -1) {
			throw std::runtime_error("Failed to create timer resources");
		}

		// 将timer_fd_加入epoll监控
		epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = timer_fd_;
		epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &ev);

		// 将wakeup_fd_加入epoll监控
		epoll_event ev_wake{};
		ev_wake.events = EPOLLIN;
		ev_wake.data.fd = wakeup_fd_;
		epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_fd_, &ev_wake);
	}

	~TimerManager() {
		stop();
		close(timer_fd_);
		close(epoll_fd_);
	}

	void start() {
		if (!running_.exchange(true)) {
			worker_thread_ = std::thread(&TimerManager::processTimers, this);
		}
	}

	void stop() {
		if (running_.exchange(false)) {
			uint64_t one = 1;
			write(wakeup_fd_, &one, sizeof(one)); // 正确唤醒方式
			if (worker_thread_.joinable()) {
				worker_thread_.join();
			}
		}
	}

	int add_periodic(uint64_t interval_ms, TimerCallback cb, void* arg) {
		return addTimer(interval_ms, interval_ms, std::move(cb), arg);
	}

	int add_once(uint64_t delay_ms, TimerCallback cb, void* arg) {
		return addTimer(delay_ms, 0, std::move(cb), arg);
	}

	bool cancel(int timer_id) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = timers_.find(timer_id);
		if (it == timers_.end()) return false;

		// 从堆中移除
		auto& timer = it->second;
		timer->valid = false; // 标记为无效

		// 如果取消的是下一个要触发的定时器，需要重置timerfd
		if (!heap_.empty() && heap_.top()->id == timer_id) {
			resetTimerFd();
		}

		timers_.erase(it);
		return true;
	}

private:
	struct TimerNode {
		uint64_t expire;      // 绝对到期时间(ms)
		uint64_t interval;    // 间隔时间(ms)
		int id;               // 定时器ID
		TimerCallback cb;     // 回调函数
		void* arg;            // 回调参数
		bool valid = true;    // 是否有效
		bool periodic;        // 是否周期性
	};

	struct TimerCompare {
		bool operator()(const std::shared_ptr<TimerNode>& a,
			const std::shared_ptr<TimerNode>& b) const {
			return a->expire > b->expire; // 最小堆
		}
	};

	int epoll_fd_;
	int timer_fd_;
	int wakeup_fd_; // 新增专用唤醒fd
	std::atomic<bool> running_;
	std::thread worker_thread_;
	std::mutex mutex_;
	std::unordered_map<int, std::shared_ptr<TimerNode>> timers_;
	//std::priority_queue<std::shared_ptr<TimerNode>> heap_;
	std::priority_queue<
		std::shared_ptr<TimerNode>,
		std::vector<std::shared_ptr<TimerNode>>,
		TimerCompare // 自定义比较器
	> heap_;
	int next_id_ = 1;

	int addTimer(uint64_t delay_ms, uint64_t interval_ms, TimerCallback cb, void* arg) {
		std::lock_guard<std::mutex> lock(mutex_);

		auto timer = std::make_shared<TimerNode>();
		timer->id = next_id_++;
		timer->expire = getCurrentMs() + delay_ms;
		timer->interval = interval_ms;
		timer->cb = std::move(cb);
		timer->arg = arg;
		timer->periodic = (interval_ms > 0);

		bool is_next_timer = !heap_.empty() && (timer->expire < heap_.top()->expire);
		timers_[timer->id] = timer;
		heap_.push(timer);

		if (is_next_timer || heap_.size() == 1) {
			resetTimerFd();

			// 下面的代码有误，不能通过wakeup_fd_唤醒epoll_wait
			// 强制唤醒epoll_wait以立即处理新定时器
			//uint64_t one = 1;
			//write(timer_fd_, &one, sizeof(one));

			// 正确唤醒方式
			uint64_t one = 1;
			write(wakeup_fd_, &one, sizeof(one)); 
		}

		return timer->id;
	}

	void resetTimerFd() {
		if (heap_.empty()) {
			// 没有定时器，禁用timerfd
			struct itimerspec its = {};
			timerfd_settime(timer_fd_, 0, &its, nullptr);
			return;
		}

		auto next_timer = heap_.top();
		uint64_t now = getCurrentMs();
		uint64_t delay = (next_timer->expire > now) ? (next_timer->expire - now) : 1; // 至少1ms

		struct itimerspec its;
		its.it_value.tv_sec = delay / 1000;
		its.it_value.tv_nsec = (delay % 1000) * 1000000;
		its.it_interval.tv_sec = 0; // 单次触发
		its.it_interval.tv_nsec = 0;

		timerfd_settime(timer_fd_, 0, &its, nullptr);
	}

	void processTimers() {
		constexpr int MAX_EVENTS = 32;
		struct epoll_event events[MAX_EVENTS];

		while (running_) {
			int timeout = -1;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (!heap_.empty()) {
					uint64_t now = getCurrentMs();
					timeout = std::max(1, (int)(heap_.top()->expire - now));
				}
			}

			int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout);
			if (!running_) break;

			if (nfds == -1) {
				if (errno == EINTR) continue;
				perror("epoll_wait");
				break;
			}

			for (int i = 0; i < nfds; ++i) {
				// 处理唤醒事件
				if (events[i].data.fd == wakeup_fd_) {
					uint64_t dummy;
					read(wakeup_fd_, &dummy, sizeof(dummy));
					continue; // 纯唤醒，无业务逻辑
				}
				// 处理定时器事件...
				if (events[i].data.fd == timer_fd_) {
					uint64_t expirations;
					read(timer_fd_, &expirations, sizeof(expirations));
					processExpiredTimers();
				}
			}

			//mark 处理超时未触发的情况（补偿机制）?
			if (nfds == 0) {
				// 没有事件，检查是否有过期的定时器
				printf("......No events, checking for expired timers......\n");
				processExpiredTimers();
			}
		}
	}

	void processExpiredTimers() {
		std::unique_lock<std::mutex> lock(mutex_);
		uint64_t now = getCurrentMs();

		while (!heap_.empty() && heap_.top()->expire <= now) {
			auto timer = heap_.top();
			heap_.pop();

			if (!timer->valid) {
				continue; // 已被取消
			}

			// 执行回调（解锁避免死锁）
			lock.unlock();
			timer->cb(timer->arg);
			lock.lock();

			// 处理周期性定时器
			if (timer->periodic && timer->valid) {
				// 时间补偿逻辑
				uint64_t drift = now - timer->expire;
				timer->expire = now + timer->interval - std::min(drift, timer->interval);
				heap_.push(timer);
			}
			else {
				timers_.erase(timer->id);
			}
		}

		// 重置timerfd为下一个定时器
		resetTimerFd();
	}

	static uint64_t getCurrentMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}
};
