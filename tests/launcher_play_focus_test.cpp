// ROM-free ImGui frame regression: no renderer, screenshots or framebuffer.
#define SDL_MAIN_HANDLED
#include "launcher_debug_key.h"
#include "backends/imgui/launcher_play_focus.h"
#include "backends/imgui_impl_sdl2.h"
#include <cassert>

namespace {
struct PlayFocusFrameTest {
    SDL_Window* window = nullptr;
    bool with_netplay, play_enabled;
    bool play_focused = false, nav_visible = false;
    int play_clicks = 0, netplay_clicks = 0, key_events = 0;

    PlayFocusFrameTest(bool netplay, bool enabled)
        : with_netplay(netplay), play_enabled(enabled) {
        window = SDL_CreateWindow("PLAY focus unit", 0, 0, 800, 600, SDL_WINDOW_HIDDEN);
        assert(window);
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.Fonts->AddFontDefault();
        assert(io.Fonts->Build());
        assert(ImGui_ImplSDL2_InitForOther(window));
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        SDL_Event focus{};
        focus.type = SDL_WINDOWEVENT;
        focus.window.windowID = SDL_GetWindowID(window);
        focus.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
        assert(ImGui_ImplSDL2_ProcessEvent(&focus));
    }

    ~PlayFocusFrameTest() {
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyWindow(window);
    }

    void frame() {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            const bool accepted = ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                assert(accepted);
                assert(event.key.windowID == SDL_GetWindowID(window));
                ++key_events;
            }
        }
        ImGui_ImplSDL2_NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(800, 600);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(800, 600));
        ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::BeginChild("body", ImVec2(0, 450));
        ImGui::BeginChild("page", ImVec2(0, 0));
        ImGui::TextUnformatted("Graphics");
        ImGui::Button("Representative settings control");
        ImGui::EndChild();
        ImGui::EndChild();
        if (with_netplay) {
            ImGui::SetCursorPos(ImVec2(350, 500));
            if (ImGui::Button("NETPLAY", ImVec2(170, 46))) ++netplay_clicks;
        }
        ImGui::SetCursorPos(ImVec2(560, 500));
        // The production helper must immediately precede the actual PLAY item.
        launcher_imgui_focus_play_on_shortcut();
        if (!play_enabled) ImGui::BeginDisabled();
        if (ImGui::InvisibleButton("##play", ImVec2(210, 46), ImGuiButtonFlags_EnableNav) && play_enabled)
            ++play_clicks;
        play_focused = ImGui::IsItemFocused();
        nav_visible = io.NavVisible;
        if (!play_enabled) ImGui::EndDisabled();
        ImGui::SetItemDefaultFocus();
        ImGui::End();
        ImGui::EndFrame(); // Intentionally no Render() or rendering backend.
    }

    void advance(int frames = 5) { while (frames-- > 0) frame(); }

    void keyboard_key(SDL_Keycode key) {
        const int before = key_events;
        launcher_debug_synth_key(window, key);
        advance(); // Down/Up trickle across actual ImGui frames.
        assert(key_events == before + 2);
    }

    void shortcut(ImGuiKey key) {
        if (key == ImGuiKey_Backspace) {
            keyboard_key(SDLK_BACKSPACE);
        } else {
            // Unit-test the semantic pad keys, not physical controller polling.
            // Keyboard input above/below exercises the actual SDL event route.
            // ImGui otherwise discards gamepad keys when no pad is advertised.
            ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_HasGamepad;
            ImGui::GetIO().AddKeyEvent(key, true);
            frame();
            ImGui::GetIO().AddKeyEvent(key, false);
            advance();
        }
    }
};

void check_route(ImGuiKey shortcut, bool netplay, bool enabled) {
    PlayFocusFrameTest test(netplay, enabled);
    test.advance(8);
    assert(!test.nav_visible); // Fresh mouse/cold state, no Tab workaround.
    assert(test.play_clicks == 0 && test.netplay_clicks == 0);
    test.shortcut(shortcut);
    assert(test.play_clicks == 0 && test.netplay_clicks == 0); // Focus alone never launches.
    if (enabled) {
        assert(test.play_focused);
        assert(test.nav_visible);
    }
    test.keyboard_key(SDLK_RETURN);
    assert(test.play_clicks == (enabled ? 1 : 0));
    assert(test.netplay_clicks == 0); // Re-home must not target an intervening footer button.
    test.advance();
    assert(test.play_clicks == (enabled ? 1 : 0)); // No duplicate activation on release.
}
} // namespace

void test_launcher_play_focus() {
    const ImGuiKey shortcuts[] = {ImGuiKey_Backspace, ImGuiKey_GamepadStart, ImGuiKey_GamepadFaceRight};
    for (const auto key : shortcuts) {
        check_route(key, false, true);
        check_route(key, true, true);
        check_route(key, true, false);
    }
}
