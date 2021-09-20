#pragma once

#include "physics/physics_ptr.h"

namespace kw {

template<typename T>
using acquireReference_t = decltype(std::declval<T&>().acquireReference());

template <typename T>
constexpr bool has_acquireReference = std::is_detected_v<toString_t, T>;

template <typename T>
PhysicsPtr<T>::PhysicsPtr(T* value)
    : m_value(value)
{
}

template <typename T>
PhysicsPtr<T>::PhysicsPtr(const PhysicsPtr& other)
    : m_value(other.m_value)
{
    static_assert(has_acquireReference<T>, "Only counted reference objects can use copy constructor.");

    if (m_value != nullptr) {
        m_value->acquireReference();
    }
}

template <typename T>
PhysicsPtr<T>::PhysicsPtr(PhysicsPtr&& other) noexcept
    : m_value(other.m_value)
{
    other.m_value = nullptr;
}


template <typename T>
PhysicsPtr<T>::~PhysicsPtr() {
    if (m_value != nullptr) {
        m_value->release();
    }
}

template <typename T>
PhysicsPtr<T>& PhysicsPtr<T>::operator=(T* value) {
    if (m_value != nullptr) {
        m_value->release();
    }

    m_value = value;

    return *this;
}

template <typename T>
PhysicsPtr<T>& PhysicsPtr<T>::operator=(const PhysicsPtr& other) {
    static_assert(has_acquireReference<T>, "Only counted reference objects can use copy assignment operator.");

    if (&other != this) {
        if (m_value != nullptr) {
            m_value->release();
        }

        m_value = other.m_value;

        if (m_value != nullptr) {
            m_value->acquireReference();
        }
    }

    return *this;
}

template <typename T>
PhysicsPtr<T>& PhysicsPtr<T>::operator=(PhysicsPtr&& other) noexcept {
    if (&other != this) {
        if (m_value != nullptr) {
            m_value->release();
        }

        m_value = other.m_value;
        other.m_value = nullptr;
    }

    return *this;
}

template <typename T>
T* PhysicsPtr<T>::release() {
    return m_value;
}

template <typename T>
void PhysicsPtr<T>::reset(T* value) {
    if (m_value != nullptr) {
        m_value->release();
    }

    m_value = value;
}

template <typename T>
void PhysicsPtr<T>::swap(PhysicsPtr& other) {
    T* temp = m_value;
    m_value = other.m_value;
    other.m_value = temp;
}

template <typename T>
T* PhysicsPtr<T>::get() const {
    return m_value;
}

template <typename T>
T& PhysicsPtr<T>::operator*() const {
    return *m_value;
}

template <typename T>
T* PhysicsPtr<T>::operator->() const {
    return m_value;
}

template <typename T>
PhysicsPtr<T>::operator bool() const {
    return m_value != nullptr;
}

template <typename T>
bool PhysicsPtr<T>::operator==(const PhysicsPtr& other) const {
    return m_value == other.m_value;
}

template <typename T>
bool PhysicsPtr<T>::operator!=(const PhysicsPtr& other) const {
    return m_value != other.m_value;
}

template <typename T>
bool PhysicsPtr<T>::operator<(const PhysicsPtr& other) const {
    return m_value < other.m_value;
}

template <typename T>
bool PhysicsPtr<T>::operator<=(const PhysicsPtr& other) const {
    return m_value <= other.m_value;
}

template <typename T>
bool PhysicsPtr<T>::operator>(const PhysicsPtr& other) const {
    return m_value > other.m_value;
}

template <typename T>
bool PhysicsPtr<T>::operator>=(const PhysicsPtr& other) const {
    return m_value >= other.m_value;
}

template <typename T>
bool PhysicsPtr<T>::operator==(const T* other) const {
    return m_value == other;
}

template <typename T>
bool PhysicsPtr<T>::operator!=(const T* other) const {
    return m_value != other;
}

template <typename T>
bool PhysicsPtr<T>::operator<(const T* other) const {
    return m_value < other;
}

template <typename T>
bool PhysicsPtr<T>::operator<=(const T* other) const {
    return m_value <= other;
}

template <typename T>
bool PhysicsPtr<T>::operator>(const T* other) const {
    return m_value > other;
}

template <typename T>
bool PhysicsPtr<T>::operator>=(const T* other) const {
    return m_value >= other;
}

} // namespace kw
