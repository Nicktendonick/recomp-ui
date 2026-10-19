#ifndef LAUNCHER_IMGUI_PLAY_FOCUS_H
#define LAUNCHER_IMGUI_PLAY_FOCUS_H

#include "imgui.h"

// Call immediately before submitting PLAY, after any optional footer buttons.
// Re-home only: the next explicit activate key/button still owns launching.
inline void launcher_imgui_focus_play_on_shortcut() {
    if (!ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false) &&
        !ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false) &&
        !ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
        return;
    ImGui::SetKeyboardFocusHere();
#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM >= 19140
    // Focus requests deliberately preserve cursor visibility in modern ImGui,
    // but Enter/Space activation requires it. Cold-start and mouse-first users
    // must not need an unrelated Tab/arrow before this explicit nav shortcut.
    ImGui::SetNavCursorVisible(true);
#endif
    // Older HOST_IMGUI providers lack this public API; retain their existing
    // focus behavior without depending on private ImGui context fields.
}

#endif
