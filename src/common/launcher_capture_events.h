#ifndef LAUNCHER_CAPTURE_EVENTS_H
#define LAUNCHER_CAPTURE_EVENTS_H

#include "launcher_platform.h"

// Binding capture consumes input, but the UI backend must still learn when its
// mapped gamepad list changes. Do not pass button, axis, or keyboard input here.
static inline bool launcher_capture_event_passthrough(const SDL_Event* event) {
    if (!event) return false;
#if defined(LNG_SDL3)
    return event->type == SDL_EVENT_GAMEPAD_ADDED ||
           event->type == SDL_EVENT_GAMEPAD_REMOVED ||
           event->type == SDL_EVENT_GAMEPAD_REMAPPED;
#else
    return event->type == SDL_CONTROLLERDEVICEADDED ||
           event->type == SDL_CONTROLLERDEVICEREMOVED ||
           event->type == SDL_CONTROLLERDEVICEREMAPPED;
#endif
}

#endif
