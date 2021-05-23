#include "core/concurrency/spinlock.h"

namespace kw {

void Spinlock::lock() {
    while (m_is_locked.test_and_set(std::memory_order_acquire));
}

bool Spinlock::try_lock() {
    return m_is_locked.test_and_set(std::memory_order_acquire);
}

void Spinlock::unlock() {
    m_is_locked.clear(std::memory_order_release);
}

} // namespace kw
