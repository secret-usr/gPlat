#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <stdexcept> // Add this include directive at the top of the file
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <chrono>
#include <memory>
#include <thread>
#include <atomic>
#include <sys/epoll.h>
#include <algorithm>
#include <string.h>

class TimerManager {
public:
    using TimerCallback = std::function<void(void*)>;
    using TimerID = int;

    TimerManager() : epoll_fd_(-1), wakeup_fd_(-1), shutdown_(false) {
        
    }

    ~TimerManager() {
        close(epoll_fd_);
        close(wakeup_fd_);
    }

    void start() {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            throw std::runtime_error("Failed to create epoll: " +
                std::string(strerror(errno)));
        }

        wakeup_fd_ = eventfd(0, EFD_NONBLOCK);
        if (wakeup_fd_ == -1) {
            close(epoll_fd_);
            throw std::runtime_error("Failed to create eventfd: " +
                std::string(strerror(errno)));
        }

        // 添加wakeup_fd到epoll
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = wakeup_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wakeup_fd_, &ev) == -1) {
            close(epoll_fd_);
            close(wakeup_fd_);
            throw std::runtime_error("Failed to add wakeup fd to epoll");
        }

        // 启动事件处理线程
        worker_thread_ = std::thread(&TimerManager::processTimers, this);
	}

    void stop() {
        if (!shutdown_.exchange(true))
        {
            wakeup();

            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
	}

    TimerID addTimer(uint64_t delay_ms, uint64_t interval_ms, TimerCallback callback, void * user) {
        int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timer_fd == -1) {
			std::cerr << "Failed to create timerfd: " << strerror(errno) << std::endl;
            return -1;
        }

        // 设置定时器
        struct itimerspec its {};
        its.it_value.tv_sec = delay_ms / 1000;
        its.it_value.tv_nsec = (delay_ms % 1000) * 1000000;
        its.it_interval.tv_sec = interval_ms / 1000;      // 关键修改：设置间隔
        its.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;

        if (timerfd_settime(timer_fd, 0, &its, nullptr) == -1) {
			std::cerr << "Failed to set timerfd time: " << strerror(errno) << std::endl;
            close(timer_fd);
            return -1;
        }

        auto event = std::make_shared<TimerEvent>();
        event->fd = timer_fd;
        event->callback = std::move(callback);
        event->expire = getCurrentMs() + delay_ms;
        event->interval_ms = interval_ms;
        event->is_periodic = (interval_ms > 0);
		event->user = user;

        std::lock_guard<std::mutex> lock(mutex_);

        // 添加到最小堆
        heap_.push_back(event);
        std::push_heap(heap_.begin(), heap_.end(), TimerCompare());

        // 添加到epoll
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = event.get();

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
            heap_.erase(std::find(heap_.begin(), heap_.end(), event));
            close(timer_fd);
            return -1;
        }

        // 添加到映射表
        timer_map_[timer_fd] = event;

        // 唤醒epoll_wait
        wakeup();

        return timer_fd;
    }

    bool removeTimer(TimerID timer_id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = timer_map_.find(timer_id);
        if (it == timer_map_.end()) {
            return false;
        }

        auto event = it->second;

        //mark 删除红黑树中节点，目前没这个需求【socket关闭这项会自动从红黑树移除】，所以将来再扩展
        // 从epoll中删除
        //epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event->fd, nullptr);

        // 从映射表中删除
        timer_map_.erase(it);

        // 从堆中删除
        auto heap_it = std::find(heap_.begin(), heap_.end(), event);
        if (heap_it != heap_.end()) {
            heap_.erase(heap_it);
            //mark
            std::make_heap(heap_.begin(), heap_.end(), TimerCompare());
        }

        // 关闭文件描述符
        close(event->fd);

        return true;
    }

    //void shutdown() {
    //    if (!shutdown_.exchange(true)) {
    //        wakeup();
    //    }
    //}

private:
    struct TimerEvent {
        int fd;
        TimerCallback callback;
        uint64_t expire;
        uint64_t interval_ms;  // 新增：触发间隔（0表示单次）
        bool is_periodic;      // 新增：是否周期性定时器
		void* user;
    };

    struct TimerCompare {
        bool operator()(const std::shared_ptr<TimerEvent>& a,
            const std::shared_ptr<TimerEvent>& b) const {
            return a->expire > b->expire; // 最小堆
        }
    };

    void processTimers() {
        constexpr int MAX_EVENTS = 64;
        epoll_event events[MAX_EVENTS];

        while (!shutdown_) {
            int timeout = -1; // 默认无限等待

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!heap_.empty()) {
                    uint64_t now = getCurrentMs();
                    if (heap_.front()->expire > now) {
                        timeout = static_cast<int>(heap_.front()->expire - now);
                    }
                    else {
                        timeout = 0; // 立即处理
                    }
                }
            }

            std::cout << "timeout=" << timeout << std::endl;

            int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
            if (nfds == -1) {
                if (errno == EINTR) continue;
                std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
                break; // 发生错误
            }

            for (int i = 0; i < nfds; ++i) {
                if (events[i].data.fd == wakeup_fd_) {
                    // 处理唤醒事件
                    uint64_t dummy;
                    read(wakeup_fd_, &dummy, sizeof(dummy));
                    if (shutdown_) break;  // 检查是否退出
                    continue;
                }

                //auto* event = static_cast<TimerEvent*>(events[i].data.ptr);
                //强化指针转换安全性：使用shared_ptr
                auto* raw_event = static_cast<TimerEvent*>(events[i].data.ptr);
                std::shared_ptr<TimerEvent> event;

                { // 加锁查找
                    //std::lock_guard<std::mutex> lock(mutex_);
                    //if (auto it = timer_map_.find(raw_event->fd); it != timer_map_.end()) {
                    //    event = it->second; // 获取共享所有权
                    //}
                }

                if (!event) {
                    std::cerr << "Timer already removed!" << std::endl;
                    return;
                }

                // 读取定时器事件
                uint64_t expirations;
                read(event->fd, &expirations, sizeof(expirations));

                // 执行回调
                event->callback(event->user);

                std::cout << "come 1" << std::endl;

                if (event->is_periodic) {
                    std::cout << "come 2" << std::endl;
                    // 原子性更新下次触发时间（避免竞争）
                    std::lock_guard<std::mutex> lock(mutex_);

                    std::cout << "come 3" << std::endl;

                    // 安全转换：从map中获取shared_ptr
                    //if (auto it = timer_map_.find(event->fd); it != timer_map_.end()) {
                    //    auto shared_event = it->second;

                    //    // 从堆中移除旧条目
                    //    auto heap_it = std::find(heap_.begin(), heap_.end(), shared_event);

                    //    if (heap_it != heap_.end()) {
                    //        heap_.erase(heap_it);

                    //        // 更新并重新插入（使用智能指针）
                    //        shared_event->expire = getCurrentMs() + shared_event->interval_ms;
                    //        heap_.push_back(shared_event);
                    //        std::push_heap(heap_.begin(), heap_.end(), TimerCompare());

                    //        std::cout << "*******************************";
                    //        for (const auto& ev : heap_) {
                    //            std::cout << ev->fd << "->" << ev->expire << " ";
                    //        }
                    //        std::cout << std::endl;
                    //    }
                    //    else
                    //    {
                    //        std::cerr << "heap_ not found!" << event->fd << std::endl;
                    //    }
                    //}
                    //else
                    //{
                    //    std::cerr << "timer_map_ not found!" << event->fd << std::endl;
                    //}

                    // 4. 重置系统定时器
                    //struct itimerspec its {};
                    //its.it_value.tv_sec = event->interval_ms / 1000;
                    //its.it_value.tv_nsec = (event->interval_ms % 1000) * 1000000;
                    //its.it_interval = its.it_value;  // 保持相同间隔
                    //if (timerfd_settime(event->fd, 0, &its, nullptr) == -1) {
                    //    std::cerr << "Failed to set timerfd time: " << strerror(errno) << std::endl;
                    //}
                }
                else {
                    std::cout << "come 4" << std::endl;
                    removeTimer(event->fd);

                    // 重新调整堆结构（周期性定时器时间已更新）
                    std::make_heap(heap_.begin(), heap_.end(), TimerCompare());

                    std::cout << "##################################";
                    for (const auto& ev : heap_) {
                        std::cout << ev->fd << "->" << ev->expire << " ";
                    }
                    std::cout << std::endl;
                }
            }




            //std::cout << "come 5" << std::endl;

   //         // 处理可能漏掉的超时事件
   //         std::lock_guard<std::mutex> lock(mutex_);
   //         // 如果担心性能影响，可以优化为：只在检测到时间间隔较大时执行完整检查
   //         //uint64_t now = getCurrentMs();
   //         //if (!heap_.empty() && (now - heap_.front()->expire) > 5 /*ms*/) {
   //         while (!heap_.empty() && heap_.front()->expire <= getCurrentMs()) {
   //             std::cout << "come 6" << std::endl;
   //             auto event = heap_.front();
   //             std::pop_heap(heap_.begin(), heap_.end(), TimerCompare());
   //             heap_.pop_back();

   //             std::cout << "come 7" << std::endl;

   //             // 从epoll中删除
   //             epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event->fd, nullptr);

   //             // 执行回调
   //             event->callback(event->user);

   //             std::cout << "come 9" << std::endl;

   //             std::cout << "处理了可能漏掉的超时事件";

   //             if (event->is_periodic) {
   //                 std::cout << "come 10" << std::endl;
   //                 // 原子性更新下次触发时间（避免竞争）
   //                 //std::lock_guard<std::mutex> lock(mutex_);
   //                 std::cout << "come 11" << std::endl;

   //                 // 安全转换：从map中获取shared_ptr
   //                 if (auto it = timer_map_.find(event->fd); it != timer_map_.end()) {
   //                     auto shared_event = it->second;

   //                     // 从堆中移除旧条目
   //                     auto heap_it = std::find(heap_.begin(), heap_.end(), shared_event);

   //                     if (heap_it != heap_.end()) {
   //                         heap_.erase(heap_it);

   //                         // 更新并重新插入（使用智能指针）
   //                         shared_event->expire = getCurrentMs() + shared_event->interval_ms;
   //                         heap_.push_back(shared_event);
   //                         std::push_heap(heap_.begin(), heap_.end(), TimerCompare());

   //                         std::cout << "---------------------------------------";
   //                         for (const auto& ev : heap_) {
   //                             std::cout << ev->fd << "->" << ev->expire << " ";
   //                         }
   //                         std::cout << std::endl;
   //                     }
   //                 }

   //                 // 4. 重置系统定时器
   //                 //struct itimerspec its {};
   //                 //its.it_value.tv_sec = event->interval_ms / 1000;
   //                 //its.it_value.tv_nsec = (event->interval_ms % 1000) * 1000000;
   //                 //its.it_interval = its.it_value;  // 保持相同间隔
   //                 //if (timerfd_settime(event->fd, 0, &its, nullptr) == -1) {
   //                 //    std::cerr << "Failed to set timerfd time: " << strerror(errno) << std::endl;
   //                 //}
   //             }
   //             else {
   //                 std::cout << "come 12" << std::endl;
   //                 removeTimer(event->fd);

   //                 // 重新调整堆结构（周期性定时器时间已更新）
   //                 std::make_heap(heap_.begin(), heap_.end(), TimerCompare());

   //                 std::cout << "##################################";
   //                 for (const auto& ev : heap_) {
   //                     std::cout << ev->fd << "->" << ev->expire << " ";
   //                 }
   //                 std::cout << std::endl;
   //             }
   //         }
            //}
        }
    }

    void wakeup() {
        uint64_t one = 1;
        write(wakeup_fd_, &one, sizeof(one));
    }

    static uint64_t getCurrentMs() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }

    int epoll_fd_;
    int wakeup_fd_;
    std::atomic<bool> shutdown_;
    std::thread worker_thread_;

    std::mutex mutex_;
    std::vector<std::shared_ptr<TimerEvent>> heap_;
    std::unordered_map<TimerID, std::shared_ptr<TimerEvent>> timer_map_;
};

#endif // TIMER_MANAGER_H