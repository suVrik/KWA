#pragma once

#include <functional>
#include <memory>

namespace kw {

class ThreadPool {
public:
    ThreadPool(size_t count);
    ~ThreadPool();

    /** First callback argument goes for iteration, second goes for thread index. */
    void parallel_for(std::function<void(size_t, size_t)> callback, size_t iterations);

    size_t get_count() const;

private:
    class PImpl;
    std::unique_ptr<PImpl> m_impl;
};

} // namespace kw
