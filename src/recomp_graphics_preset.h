#ifndef RECOMP_GRAPHICS_PRESET_H
#define RECOMP_GRAPHICS_PRESET_H

/* Host-owned filter recipes, shared by launcher and live menu.
 * They deliberately exclude colour model, viewport/aspect, window and gameplay.
 * Scaling: 0 nearest, 1 linear, 2 sharp, 3 smooth. Effect: 0 off, 1 LCD, 2 CRT.
 * No preset ID is persisted: recognize the actual settings, or show Custom. */
typedef struct RecompGraphicsPreset {
    const char* name;
    const char* description;
    int scaling;
    int effect;
    int strength;
} RecompGraphicsPreset;

static inline int recomp_graphics_preset_valid(const RecompGraphicsPreset* p) {
    return p && p->name && p->description && p->scaling >= 0 && p->scaling <= 3 &&
        p->effect >= 0 && p->effect <= 2 && p->strength >= 0 && p->strength <= 100;
}
static inline int recomp_graphics_preset_match(const RecompGraphicsPreset* presets,
                                               int count, int scaling, int effect, int strength) {
    if (!presets || count < 1 || count > 16) return -1;
    for (int i = 0; i < count; ++i) {
        const RecompGraphicsPreset* p = &presets[i];
        if (recomp_graphics_preset_valid(p) && p->scaling == scaling && p->effect == effect &&
            (!effect || p->strength == strength)) return i;
    }
    return -1;
}
#endif
