#pragma once

#include <cstdint>

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
}
