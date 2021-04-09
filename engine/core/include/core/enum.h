#pragma once

#include <cstdint>

#define KW_DECLARE_ENUM_BITMASK(type)                                                  \
friend constexpr type operator|(type lhs, type rhs);                                   \
friend constexpr type operator&(type lhs, type rhs);                                   \
friend constexpr type operator^(type lhs, type rhs);                                   \
friend constexpr type& operator|=(type& lhs, type rhs);                                \
friend constexpr type& operator&=(type& lhs, type rhs);                                \
friend constexpr type& operator^=(type& lhs, type rhs);                                \
friend constexpr type operator~(type e)

#define KW_DEFINE_ENUM_BITMASK(type)                                                   \
constexpr type operator|(type lhs, type rhs) {                                         \
    return static_cast<type>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); \
}                                                                                      \
constexpr type operator&(type lhs, type rhs) {                                         \
    return static_cast<type>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); \
}                                                                                      \
constexpr type operator^(type lhs, type rhs) {                                         \
    return static_cast<type>(static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs)); \
}                                                                                      \
constexpr type& operator|=(type& lhs, type rhs) {                                      \
    lhs = lhs | rhs;                                                                   \
    return lhs;                                                                        \
}                                                                                      \
constexpr type& operator&=(type& lhs, type rhs) {                                      \
    lhs = lhs & rhs;                                                                   \
    return lhs;                                                                        \
}                                                                                      \
constexpr type& operator^=(type& lhs, type rhs) {                                      \
    lhs = lhs ^ rhs;                                                                   \
    return lhs;                                                                        \
}                                                                                      \
constexpr type operator~(type e) {                                                     \
    return static_cast<type>(~static_cast<uint32_t>(e));                               \
}                                                                                      \
static_assert(true)
