// launcher_files.c — native ROM picker via tinyfiledialogs (zlib).

#include "launcher_files.h"

#include "third_party/tinyfiledialogs.h"

#include <stdio.h>
#include <string.h>

bool launcher_pick_rom(char* out_path, size_t out_cap) {
    if (!out_path || out_cap == 0) return false;
    out_path[0] = '\0';

    // No filter: this fallback runs only when the active console's profile
    // supplied no rom_filter, and it cannot know what that console accepts.
    // Offering "All files" is honest; naming one system's extensions here is
    // how a PSX build ended up asking players for a SNES cartridge.
    const char* sel = tinyfd_openFileDialog(
        "Select game file",
        "",       // default path/file
        0, NULL,  // no patterns -> all files
        NULL,     // no filter description
        0);       // single select
    if (!sel || !sel[0]) return false;

    snprintf(out_path, out_cap, "%s", sel);
    return true;
}

bool launcher_pick_folder(const char* title, char* out_path, size_t out_cap) {
    if (!out_path || out_cap == 0) return false;
    out_path[0] = '\0';
    const char* sel = tinyfd_selectFolderDialog(title ? title : "Select folder", "");
    if (!sel || !sel[0]) return false;
    snprintf(out_path, out_cap, "%s", sel);
    return true;
}

bool launcher_pick_file(const char* title, const char* const* patterns, int num_patterns,
                        const char* desc, char* out_path, size_t out_cap) {
    if (!out_path || out_cap == 0) return false;
    out_path[0] = '\0';

    const char* sel = tinyfd_openFileDialog(
        title ? title : "Select file",
        "",
        num_patterns > 0 ? num_patterns : 0,
        num_patterns > 0 ? patterns : NULL,
        desc,
        0);
    if (!sel || !sel[0]) return false;

    snprintf(out_path, out_cap, "%s", sel);
    return true;
}

bool launcher_pick_save_file(const char* title, const char* const* patterns, int num_patterns,
                             const char* desc, char* out_path, size_t out_cap) {
    if (!out_path || out_cap == 0) return false;
    out_path[0] = '\0';

    const char* sel = tinyfd_saveFileDialog(
        title ? title : "Save file",
        "",
        num_patterns > 0 ? num_patterns : 0,
        num_patterns > 0 ? patterns : NULL,
        desc);
    if (!sel || !sel[0]) return false;

    snprintf(out_path, out_cap, "%s", sel);
    return true;
}
