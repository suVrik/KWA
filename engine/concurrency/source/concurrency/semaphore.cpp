#include "concurrency/semaphore.h"

namespace kw {

Semaphore::Semaphore(size_t initial)
    : m_counter(initial) {
}

void Semaphore::acquire() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_counter == 0) {
        m_condition_variable.wait(lock);
    }
    m_counter--;
}

bool Semaphore::try_acquire() {
    if (m_mutex.try_lock()) {
        if (m_counter > 0) {
            m_counter--;
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
    }
    return false;
}

void Semaphore::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counter++;
    m_condition_variable.notify_one();
}

} // namespace kw
