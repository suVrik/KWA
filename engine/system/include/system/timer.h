#pragma once

#include <cstdint>

namespace kw {

class Timer {
public:
    Timer();

    void update();

    float get_elapsed_time() const;
    float get_absolute_time() const;

private:
    uint64_t m_startup_time;
    uint64_t m_previous_frame;

    float m_elapsed_time;
    float m_absolute_time;
};

} // namespace kw
