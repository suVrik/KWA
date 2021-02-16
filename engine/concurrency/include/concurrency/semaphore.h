#pragma once

#include <condition_variable>
#include <mutex>

namespace kw {

class Semaphore {
public:
    Semaphore(size_t initial = 0);

    /** Decrement the internal counter or wait. */
    void acquire();

    /** Try to decrement the internal counter without waiting. Return true on success. */
    bool try_acquire();

    /** Increment the internal counter and unblock waiting acquirer. */
    void release();

private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    size_t m_counter;
};

} // namespace kw
