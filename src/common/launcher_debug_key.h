// Scripted key events must target the launcher's window, just like real keys.
#ifndef LAUNCHER_DEBUG_KEY_H
#define LAUNCHER_DEBUG_KEY_H

#include "launcher_sdlcompat.h"

static inline void launcher_debug_synth_key(SDL_Window* window, SDL_Keycode key) {
    if (!window) return;
    SDL_Scancode sc = SDL_GetScancodeFromKey(key
#if defined(LNG_SDL3)
        , NULL
#endif
        );
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_DOWN;
    // ImGui's SDL backend rejects events with an unknown window ID. Zero here
    // made LNG_SCRIPT key commands silently disappear before navigation.
    e.key.windowID = SDL_GetWindowID(window);
#if defined(LNG_SDL3)
    e.key.key = key;
    e.key.scancode = sc;
    e.key.down = true;
#else
    e.key.keysym.sym = key;
    e.key.keysym.scancode = sc;
    e.key.state = SDL_PRESSED;
#endif
    SDL_PushEvent(&e);
    e.type = SDL_EVENT_KEY_UP;
#if defined(LNG_SDL3)
    e.key.down = false;
#else
    e.key.state = SDL_RELEASED;
#endif
    SDL_PushEvent(&e);
}

#endif
