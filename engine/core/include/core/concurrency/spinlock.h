#pragma once

#include <atomic>

namespace kw {

class Spinlock {
public:
    void lock();
    bool try_lock();
    void unlock();

private:
    std::atomic_flag m_is_locked = ATOMIC_FLAG_INIT;
};

} // namespace kw
