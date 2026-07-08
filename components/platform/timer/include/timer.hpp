/******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by
 * the sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef TIMER_HPP
#define TIMER_HPP

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_SECOND 1000  // 1 second in milliseconds

/**
 * \brief Timer structure (opaque to C code)
 */
struct timer_handle_t {
        void *ptr; /* Timer entry pointer */
};

/**
 * \brief      Initialize the timer library.
 */
void timer_init(void);

/**
 * \brief      Set a callback timer.
 */
void timer_set(struct timer_handle_t *t, uint64_t interval, void (*callback)(void *), void *ptr);

/**
 * \brief      Stop a pending timer.
 */
void timer_stop(struct timer_handle_t *t);

/**
 * \brief      Restart a timer from the current point in time
 */
void timer_restart(struct timer_handle_t *t);

/**
 * \brief      Reset a timer with the same interval as was previously set.
 */
void timer_reset(struct timer_handle_t *t);

/**
 * \brief      Check if a timer has expired.
 */
bool timer_expired(struct timer_handle_t *t);

/**
 * \brief      Check if a timer is up and running.
 */
bool timer_running(struct timer_handle_t *t);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cstdint>

namespace timer
{

    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration  = Clock::duration;

    struct TimerEntry {
            TimePoint expiration_time;
            TimePoint last_expiration_time;
            std::function<void()> callback;
            Duration interval;
            bool is_active;
            bool is_set;
    };

    struct TimerEntryComparator {
            bool operator()(const std::shared_ptr<TimerEntry> &a, const std::shared_ptr<TimerEntry> &b) const
            {
                return a->expiration_time > b->expiration_time;
            }
    };

    /**
     * \brief TimerManager class for managing timers using C++ chrono
     */
    class TimerManager
    {
        public:
            TimerManager() : running_(false), next_id_(1) {}

            void init()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!running_) {
                    running_       = true;
                    worker_thread_ = std::thread(&TimerManager::worker_loop, this);
                }
            }

            void shutdown()
            {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    running_ = false;
                    condition_.notify_all();
                }
                if (worker_thread_.joinable()) {
                    worker_thread_.join();
                }
            }

            void *create_timer()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto entry       = std::make_shared<TimerEntry>();
                entry->is_active = false;
                entry->is_set    = false;
                void *id         = reinterpret_cast<void *>(next_id_++);
                timers_[id]      = entry;
                return id;
            }

            void set_timer(void *timer_id, uint64_t interval, std::function<void()> callback)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = timers_.find(timer_id);
                if (it == timers_.end()) {
                    return;
                }

                auto entry                  = it->second;
                entry->interval             = std::chrono::milliseconds(interval);
                entry->expiration_time      = Clock::now() + entry->interval;
                entry->last_expiration_time = entry->expiration_time;
                entry->callback             = callback;
                entry->is_active            = true;
                entry->is_set               = true;

                rebuild_queue();
                condition_.notify_one();
            }

            void stop_timer(void *timer_id)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = timers_.find(timer_id);
                if (it != timers_.end()) {
                    it->second->is_active = false;
                    it->second->is_set    = false;
                    condition_.notify_one();
                }
            }

            void restart_timer(void *timer_id)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = timers_.find(timer_id);
                if (it == timers_.end() || !it->second->is_set) {
                    return;
                }

                auto entry             = it->second;
                entry->expiration_time = Clock::now() + entry->interval;
                entry->is_active       = true;
                rebuild_queue();
                condition_.notify_one();
            }

            void reset_timer(void *timer_id)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = timers_.find(timer_id);
                if (it == timers_.end() || !it->second->is_set) {
                    return;
                }

                auto entry = it->second;

                entry->expiration_time = entry->expiration_time + entry->interval;
                entry->is_active       = true;

                entry->last_expiration_time = entry->expiration_time;

                rebuild_queue();
                condition_.notify_one();
            }

            bool expired_timer(void *timer_id)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = timers_.find(timer_id);
                if (it == timers_.end()) {
                    return true;
                }
                if (!it->second->is_set) {
                    return true;
                }
                if (!it->second->is_active) {
                    return true;
                }
                return (it->second->expiration_time <= Clock::now()) ? true : false;
            }

            bool timer_exists(void *timer_id)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return timers_.find(timer_id) != timers_.end();
            }

            bool is_timer_running(void *timer_id)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = timers_.find(timer_id);
                if (it == timers_.end()) {
                    return false;
                }
                auto entry = it->second;
                if (!entry->is_set || !entry->is_active) {
                    return false;
                }
                return entry->expiration_time > Clock::now();
            }

        private:
            void rebuild_queue()
            {
                std::priority_queue<std::shared_ptr<TimerEntry>, std::vector<std::shared_ptr<TimerEntry>>, TimerEntryComparator> new_queue;
                for (auto &pair: timers_) {
                    if (pair.second->is_active && pair.second->is_set) {
                        new_queue.push(pair.second);
                    }
                }
                timer_queue_ = std::move(new_queue);
            }

            void worker_loop()
            {
                while (running_) {
                    std::unique_lock<std::mutex> lock(mutex_);

                    while (!timer_queue_.empty()) {
                        auto entry = timer_queue_.top();

                        if (!entry->is_active) {
                            timer_queue_.pop();
                            continue;
                        }

                        if (entry->expiration_time > Clock::now()) {
                            break;
                        }

                        timer_queue_.pop();
                        entry->last_expiration_time = entry->expiration_time;
                        entry->is_active            = false;
                        lock.unlock();
                        if (entry->callback) {
                            entry->callback();
                        }
                        lock.lock();
                        rebuild_queue();
                    }

                    if (timer_queue_.empty()) {
                        condition_.wait(lock);
                    } else {
                        auto next_expiration = timer_queue_.top()->expiration_time;
                        auto now             = Clock::now();
                        if (next_expiration > now) {
                            condition_.wait_for(lock, next_expiration - now);
                        }
                    }
                }
            }

            std::atomic<bool> running_;
            std::mutex mutex_;
            std::condition_variable condition_;
            std::thread worker_thread_;
            std::priority_queue<std::shared_ptr<TimerEntry>, std::vector<std::shared_ptr<TimerEntry>>, TimerEntryComparator> timer_queue_;
            std::unordered_map<void *, std::shared_ptr<TimerEntry>> timers_;
            uintptr_t next_id_;
    };

    void initialize_timer_manager();
    TimerManager *get_timer_manager();

}  // namespace timer

#endif  // __cplusplus

#endif /* TIMER_HPP */
