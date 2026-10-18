#define SDL_MAIN_HANDLED
#include "launcher_debug_key.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include <cassert>

int main() {
    SDL_SetMainReady();
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_Window* window = SDL_CreateWindow("Synthetic key test", 0, 0, 320, 240, SDL_WINDOW_HIDDEN);
    assert(window && SDL_GetWindowID(window) != 0);
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    assert(ImGui_ImplSDL2_InitForOther(window));
    SDL_Event event;
    const SDL_Keycode keys[] = {SDLK_BACKSPACE, SDLK_RETURN, SDLK_SPACE, SDLK_TAB, SDLK_ESCAPE, SDLK_LEFT};
    for (const auto key : keys) {
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        launcher_debug_synth_key(window, key);
        int count = 0;
        while (SDL_PollEvent(&event)) {
            if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) continue;
            assert(event.key.windowID == SDL_GetWindowID(window));
            assert(event.key.keysym.sym == key);
            assert(event.key.keysym.scancode == SDL_GetScancodeFromKey(key));
            assert(event.type == (count == 0 ? SDL_KEYDOWN : SDL_KEYUP));
            assert(event.key.state == (count == 0 ? SDL_PRESSED : SDL_RELEASED));
            assert(ImGui_ImplSDL2_ProcessEvent(&event));
            // Reproduce the old bug: the same key with no target is rejected.
            event.key.windowID = 0;
            assert(!ImGui_ImplSDL2_ProcessEvent(&event));
            ++count;
        }
        assert(count == 2);
    }
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    launcher_debug_synth_key(nullptr, SDLK_RETURN);
    assert(!SDL_HasEvents(SDL_KEYDOWN, SDL_KEYUP));
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
}
