#define SDL_MAIN_HANDLED
#include "launcher_capture_events.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include <cassert>
#include <initializer_list>

// ROM-free regression for the production capture gate plus the pinned backend.
// This deliberately does not claim to exercise launcher_imgui.cpp's dispatcher
// or binding assignments: full-launcher integration is a separate check.
namespace {
struct Counts {
    int added = 0, removed = 0, remapped = 0;
    int keys = 0, buttons = 0, axes = 0;
} counts;

void dispatch(const SDL_Event& event, bool capturing) {
    if (capturing && !launcher_capture_event_passthrough(&event)) {
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) ++counts.keys;
        if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP)
            ++counts.buttons;
        if (event.type == SDL_CONTROLLERAXISMOTION) ++counts.axes;
        return;
    }
    if (event.type == SDL_CONTROLLERDEVICEADDED) ++counts.added;
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) ++counts.removed;
    if (event.type == SDL_CONTROLLERDEVICEREMAPPED) ++counts.remapped;
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void frame(bool capturing) {
    SDL_JoystickUpdate();
    SDL_Event event;
    while (SDL_PollEvent(&event)) dispatch(event, capturing);
    auto& io = ImGui::GetIO();
    // Match the launcher's independent suppression of polled pad navigation.
    if (capturing) io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    else io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::EndFrame();
}

bool has_gamepad() {
    return (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
}

struct VirtualPad {
    SDL_Joystick* joystick = nullptr;
    int index = -1;
    SDL_JoystickID id = -1;

    void attach() {
        assert(!joystick);
        index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 0);
        assert(index >= 0 && SDL_IsGameController(index));
        joystick = SDL_JoystickOpen(index);
        assert(joystick);
        id = SDL_JoystickInstanceID(joystick);
        assert(id >= 0);
        axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
        axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    }

    void button(int button, bool down) {
        assert(SDL_JoystickSetVirtualButton(joystick, button, down ? 1 : 0) == 0);
    }

    void axis(int axis, Sint16 value) {
        assert(SDL_JoystickSetVirtualAxis(joystick, axis, value) == 0);
    }

    void detach() {
        assert(joystick);
        SDL_JoystickClose(joystick);
        joystick = nullptr;
        assert(SDL_JoystickDetachVirtual(index) == 0);
        index = -1;
    }
};

void check_gate_types() {
    assert(!launcher_capture_event_passthrough(nullptr));
    SDL_Event event{};
    for (Uint32 type : {SDL_CONTROLLERDEVICEADDED, SDL_CONTROLLERDEVICEREMOVED,
                        SDL_CONTROLLERDEVICEREMAPPED}) {
        event.type = type;
        assert(launcher_capture_event_passthrough(&event));
    }
    for (Uint32 type : {SDL_KEYDOWN, SDL_KEYUP, SDL_CONTROLLERBUTTONDOWN,
                        SDL_CONTROLLERBUTTONUP, SDL_CONTROLLERAXISMOTION,
                        SDL_JOYDEVICEADDED, SDL_JOYDEVICEREMOVED, SDL_JOYBUTTONDOWN,
                        SDL_JOYAXISMOTION, SDL_MOUSEBUTTONDOWN, SDL_QUIT}) {
        event.type = type;
        assert(!launcher_capture_event_passthrough(&event));
    }
}
} // namespace

int main() {
    check_gate_types();
    SDL_SetMainReady();
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "0");
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
    SDL_SetHint(SDL_HINT_DIRECTINPUT_ENABLED, "0");
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) == 0);
    // Never use or alter a physical controller as an implicit test fixture.
    assert(SDL_NumJoysticks() == 0);
    SDL_Window* window = SDL_CreateWindow("Capture hotplug test", 0, 0, 320, 240,
                                          SDL_WINDOW_HIDDEN);
    assert(window);
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigInputTrickleEventQueue = false;
    io.Fonts->AddFontDefault();
    unsigned char* font_pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
    assert(ImGui_ImplSDL2_InitForOther(window));

    // Consume the backend's initial list refresh with no device present.
    // Otherwise a swallowed ADDED event can be masked by startup discovery.
    frame(false);
    assert(!has_gamepad());
    VirtualPad pad;
    pad.attach();
    frame(true);
    assert(counts.added == 1);
    frame(false);
    assert(has_gamepad());
    pad.button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, true);
    frame(false);
    assert(ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));
    pad.button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, false);
    frame(false);
    assert(!ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));

    // Actual device input and a targeted keyboard fixture remain consumed.
    const Counts before_binding = counts;
    pad.button(SDL_CONTROLLER_BUTTON_X, true);
    pad.axis(SDL_CONTROLLER_AXIS_LEFTX, 24000);
    SDL_Event key{};
    key.type = SDL_KEYDOWN;
    key.key.windowID = SDL_GetWindowID(window);
    key.key.state = SDL_PRESSED;
    key.key.keysym.sym = SDLK_k;
    key.key.keysym.scancode = SDL_SCANCODE_K;
    assert(SDL_PushEvent(&key) == 1);
    frame(true);
    assert(counts.buttons > before_binding.buttons);
    assert(counts.axes > before_binding.axes);
    assert(counts.keys == before_binding.keys + 1);
    assert(!ImGui::IsKeyDown(ImGuiKey_K));
    assert(!ImGui::IsKeyDown(ImGuiKey_GamepadFaceLeft));
    pad.button(SDL_CONTROLLER_BUTTON_X, false);
    pad.axis(SDL_CONTROLLER_AXIS_LEFTX, 0);
    key.type = SDL_KEYUP;
    key.key.state = SDL_RELEASED;
    assert(SDL_PushEvent(&key) == 1);
    frame(true);
    frame(false);
    assert(!ImGui::IsKeyDown(ImGuiKey_K));
    assert(!ImGui::IsKeyDown(ImGuiKey_GamepadFaceLeft));

    // REMAPPED is a routing fixture, not a claim about physical remapping.
    SDL_Event remapped{};
    remapped.type = SDL_CONTROLLERDEVICEREMAPPED;
    remapped.cdevice.which = pad.id;
    dispatch(remapped, true);
    assert(counts.remapped == 1);

    // Remove a held device during capture, then resume navigation: the backend
    // must lose HasGamepad and ImGui must clear its prior held-key state.
    pad.button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, true);
    frame(false);
    assert(ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));
    const SDL_JoystickID old_id = pad.id;
    pad.detach();
    frame(true);
    assert(counts.removed == 1);
    frame(false);
    assert(!has_gamepad() && !ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));

    pad.attach();
    assert(pad.id != old_id);
    frame(true);
    frame(false);
    assert(counts.added == 2 && has_gamepad());
    pad.button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, true);
    frame(false);
    assert(ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));

    // Also replace the device entirely within one capture interval. HasGamepad
    // alone would miss a stale open handle; the new instance's key must work.
    const SDL_JoystickID replaced_id = pad.id;
    pad.detach();
    pad.attach();
    assert(pad.id != replaced_id);
    frame(true);
    frame(false);
    assert(counts.removed == 2 && counts.added == 3 && has_gamepad());
    assert(!ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));
    pad.button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, true);
    frame(false);
    assert(ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));
    pad.detach();
    frame(false);
    assert(!has_gamepad() && !ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight));

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
}
