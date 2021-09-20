#pragma once

#include <utility>

namespace kw {

template <typename T>
class PhysicsPtr {
public:
    PhysicsPtr(T* value = nullptr);
    PhysicsPtr(const PhysicsPtr& other);
    PhysicsPtr(PhysicsPtr&& other) noexcept;
    ~PhysicsPtr();
    PhysicsPtr& operator=(T* value);
    PhysicsPtr& operator=(const PhysicsPtr& other);
    PhysicsPtr& operator=(PhysicsPtr&& other) noexcept;

    T* release();
    void reset(T* value = nullptr);
    void swap(PhysicsPtr& other);

    T* get() const;
    T& operator*() const;
    T* operator->() const;

    explicit operator bool() const;

    bool operator==(const PhysicsPtr& other) const;
    bool operator!=(const PhysicsPtr& other) const;
    bool operator<(const PhysicsPtr& other) const;
    bool operator<=(const PhysicsPtr& other) const;
    bool operator>(const PhysicsPtr& other) const;
    bool operator>=(const PhysicsPtr& other) const;

    bool operator==(const T* other) const;
    bool operator!=(const T* other) const;
    bool operator<(const T* other) const;
    bool operator<=(const T* other) const;
    bool operator>(const T* other) const;
    bool operator>=(const T* other) const;

private:
    T* m_value;
};

} // namespace kw

#include "physics/physics_ptr_impl.h"
