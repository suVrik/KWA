#include "concurrency/thread_pool.h"
#include "concurrency/concurrency_utils.h"
#include "concurrency/semaphore.h"

#include <debug/assert.h>

namespace kw {

class ThreadPool::PImpl {
public:
    PImpl(size_t count) {
        KW_ASSERT(count > 0);

        m_threads.reserve(count - 1);
        for (size_t i = 0; i < count - 1; i++) {
            m_threads.push_back(std::thread(&PImpl::worker_thread, this, i));

            char name_buffer[24];
            sprintf_s(name_buffer, sizeof(name_buffer), "Worker Thread %zu", i);
            ConcurrencyUtils::set_thread_name(m_threads.back(), name_buffer);
        }

        m_finished_threads.lock(m_threads.size());
    }

    ~PImpl() {
        m_is_running = false;

        m_started_threads.unlock(m_threads.size());

        for (std::thread& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void parallel_for(std::function<void(size_t)>&& callback, size_t iterations) {
        m_callback = std::move(callback);
        m_total_iterations = iterations;
        m_current_iteration = 0;

        m_started_threads.unlock(m_threads.size());

        while (true) {
            size_t current_iteration = m_current_iteration++;
            if (current_iteration < m_total_iterations) {
                m_callback(current_iteration);
            } else {
                break;
            }
        }

        m_finished_threads.lock(m_threads.size());

        m_callback = std::function<void(size_t)>();
        m_total_iterations = 0;
        m_current_iteration = 0;
    }

private:
    void worker_thread(size_t index) {
        while (m_is_running) {
            size_t current_iteration = m_current_iteration++;
            if (current_iteration < m_total_iterations) {
                m_callback(current_iteration);
            } else {
                m_finished_threads.unlock();
                m_started_threads.lock();
            }
        }
    }

    std::vector<std::thread> m_threads;
    Semaphore m_started_threads;
    Semaphore m_finished_threads;
    std::atomic_bool m_is_running = true;

    std::function<void(size_t)> m_callback;
    size_t m_total_iterations = 0;
    std::atomic_size_t m_current_iteration = 0;
};

ThreadPool::ThreadPool(size_t count)
    : m_impl(new PImpl(count)) {
}

ThreadPool::~ThreadPool() = default;

void ThreadPool::parallel_for(std::function<void(size_t)> callback, size_t iterations) {
    m_impl->parallel_for(std::move(callback), iterations);
}

} // namespace kw
