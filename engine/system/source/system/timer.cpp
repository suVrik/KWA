#include "system/timer.h"

#include <SDL_timer.h>

namespace kw {

Timer::Timer()
    : m_startup_time(SDL_GetPerformanceCounter())
    , m_previous_frame(m_startup_time)
    , m_elapsed_time(0.016f)
    , m_absolute_time(0.f)
{
}

void Timer::update() {
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();

    m_elapsed_time = static_cast<float>(now - m_previous_frame) / frequency;
    m_absolute_time = static_cast<float>(now - m_startup_time) / frequency;

    m_previous_frame = now;
}

float Timer::get_elapsed_time() const {
    return m_elapsed_time;
}

float Timer::get_absolute_time() const {
    return m_absolute_time;
}

} // namespace kw
