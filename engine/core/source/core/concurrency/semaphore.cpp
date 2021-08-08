#include "core/concurrency/semaphore.h"

namespace kw {

Semaphore::Semaphore(size_t initial)
    : m_counter(initial)
{
}

void Semaphore::lock(size_t count) {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_counter < count) {
        m_condition_variable.wait(lock);
    }
    m_counter -= count;
}

void Semaphore::unlock(size_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counter += count;
    m_condition_variable.notify_all();
}

} // namespace kw
