#pragma once

#include <condition_variable>
#include <mutex>

namespace kw {

class Semaphore {
public:
    Semaphore(size_t initial = 0);

    /** Decrement the internal counter or wait. */
    void lock(size_t count = 1);

    /** Increment the internal counter and unblock waiting acquirer. */
    void unlock(size_t count = 1);

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    size_t m_counter;
};

} // namespace kw
