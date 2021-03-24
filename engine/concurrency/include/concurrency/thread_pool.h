#pragma once

#include <functional>
#include <memory>

namespace kw {

class ThreadPool {
public:
    ThreadPool(size_t count);
    ~ThreadPool();

    void parallel_for(std::function<void(size_t)> callback, size_t iterations);

private:
    class PImpl;
    std::unique_ptr<PImpl> m_impl;
};

} // namespace kw
