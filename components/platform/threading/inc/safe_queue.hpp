/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by
 * the sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#ifndef SAFE_QUEUE_HPP
#define SAFE_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cstddef>
#include <chrono>

namespace threading
{
    /**
     * @brief Thread-safe queue template class
     *
     * This class provides a thread-safe wrapper around std::queue. All operations
     * are protected by a mutex to ensure thread safety. The queue supports
     * blocking and non-blocking operations.
     *
     * @tparam T The type of elements stored in the queue
     */
    template<typename T> class safe_queue
    {
        public:
            /**
             * @brief Default constructor
             */
            safe_queue() = default;

            /**
             * @brief Destructor
             */
            ~safe_queue() = default;

            // Delete copy constructor and assignment operator for thread safety
            safe_queue(const safe_queue &)            = delete;
            safe_queue &operator=(const safe_queue &) = delete;

            /**
             * @brief Push an element to the back of the queue
             *
             * @param value The value to push
             */
            void push(const T &value)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(value);
                condition_.notify_one();
            }

            /**
             * @brief Push an element to the back of the queue (move version)
             *
             * @param value The value to push (will be moved)
             */
            void push(T &&value)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(std::move(value));
                condition_.notify_one();
            }

            /**
             * @brief Pop an element from the front of the queue with a timeout (blocking)
             *
             * This function will block until an element is available or the timeout expires.
             *
             * @param timeout_ms Timeout in milliseconds
             * @return std::optional containing the value if available, empty if timeout expired
             */
            std::optional<T> pop(uint32_t timeout_ms = 0)
            {
                std::unique_lock<std::mutex> lock(mutex_);
                auto timeout   = std::chrono::milliseconds(timeout_ms);
                bool has_value = condition_.wait_for(lock, timeout, [this] { return !queue_.empty(); });
                if (has_value) {
                    T value = std::move(queue_.front());
                    queue_.pop();
                    return value;
                }
                return std::nullopt;
            }

            /**
             * @brief Try to pop an element from the front of the queue (non-blocking)
             *
             * @param value Reference to store the popped value
             * @return true if an element was popped, false if queue was empty
             */
            bool try_pop(T &value)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (queue_.empty()) {
                    return false;
                }
                value = std::move(queue_.front());
                queue_.pop();
                return true;
            }

            /**
             * @brief Try to pop an element from the front of the queue (non-blocking)
             *
             * @return std::optional containing the value if available, empty otherwise
             */
            std::optional<T> try_pop()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (queue_.empty()) {
                    return std::nullopt;
                }
                T value = std::move(queue_.front());
                queue_.pop();
                return value;
            }

            /**
             * @brief Check if the queue is empty
             *
             * @return true if queue is empty, false otherwise
             */
            bool empty() const
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return queue_.empty();
            }

            /**
             * @brief Get the number of elements in the queue
             *
             * @return The number of elements
             */
            size_t size() const
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return queue_.size();
            }

            /**
             * @brief Clear all elements from the queue
             */
            void clear()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                while (!queue_.empty()) {
                    queue_.pop();
                }
            }

            /**
             * @brief Count occurrences of a specific value in the queue
             *
             * @param value The value to count
             * @return The number of occurrences
             */
            size_t count(const T &value) const
            {
                std::lock_guard<std::mutex> lock(mutex_);
                size_t count             = 0;
                std::queue<T> temp_queue = queue_;
                while (!temp_queue.empty()) {
                    if (temp_queue.front() == value) {
                        count++;
                    }
                    temp_queue.pop();
                }
                return count;
            }

        private:
            mutable std::mutex mutex_;           ///< Mutex for thread synchronization
            std::condition_variable condition_;  ///< Condition variable for blocking operations
            std::queue<T> queue_;                ///< Underlying queue
    };

}  // namespace threading

#endif  // SAFE_QUEUE_HPP
