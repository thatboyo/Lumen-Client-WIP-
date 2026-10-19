package net.lumen.client.theme;

import net.minecraft.util.Identifier;

/**
 * Theme palettes recovered from the compiled WPF theme resources in the original
 * Lumin desktop application. WPF gradients and effects are approximated by the
 * Fabric GUI's current solid-color renderer.
 */
public enum LumenTheme {
    LUMIN(
        0xFF000001, 0xFF050609, 0x35FFFFFF,
        0xFFDDF5FF, 0xFF7F8792, 0xFFFFFFFF,
        0x99DDF5FF, 0xB50B0F15, 0xD70D1219
    ),
    RAIN(
        0xFF02070B, 0xFF050E15, 0x3558A6B8,
        0xFF4CB8D0, 0xFF526B7A, 0xFFBDEEFF,
        0x6652BBD1, 0xB50A1B28, 0xD70B2433
    ),
    SYNTH(
        0xFF04030A, 0xFF0B0715, 0x334C3B83,
        0xFFFF4FA3, 0xFF766A91, 0xFFFFD6F1,
        0x88FF3E9E, 0xC5100C22, 0xE01A1033
    ),

    // Backwards-compatible names for existing saved profiles.
    GHOST(0xFF0A0A0A, 0xFF111111, 0xFF222222, 0xFFFFFFFF, 0xFF666666, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000),
    CYBER(0xFF050510, 0xFF0A0A1A, 0xFF0D0D2B, 0xFF00AAFF, 0xFF0066CC, 0xFF00AAFF, 0x00000000, 0x00000000, 0x00000000),
    HACKER(0xFF010F01, 0xFF051005, 0xFF0A1F0A, 0xFF00FF41, 0xFF1A6B1A, 0xFF00FF41, 0x00000000, 0x00000000, 0x00000000);

    /** Main canvas color (WorldBlack). */
    public final int background;
    /** Main panel fill (WorldCharcoal; translucent effects are layered separately). */
    public final int panel;
    /** Subtle glass border color. */
    public final int border;
    /** Primary theme accent. */
    public final int accent;
    /** Secondary/muted text color. */
    public final int muted;
    /** Bright accent used for enabled module indicators. */
    public final int enabledIndicator;
    /** Accent halo color; alpha is retained for translucent drawing. */
    public final int glow;
    /** Translucent enabled module row surface. */
    public final int enabledSurface;
    /** Translucent panel/header surface. */
    public final int selectedSurface;

    public static LumenTheme active = LUMIN;

    LumenTheme(int background, int panel, int border, int accent, int muted,
               int enabledIndicator, int glow, int enabledSurface, int selectedSurface) {
        this.background = background;
        this.panel = panel;
        this.border = border;
        this.accent = accent;
        this.muted = muted;
        this.enabledIndicator = enabledIndicator;
        this.glow = glow;
        this.enabledSurface = enabledSurface;
        this.selectedSurface = selectedSurface;
    }

    public Identifier watermarkAsset() {
        return Identifier.of("lumen", "textures/gui/watermark.png");
    }

    public Identifier nametagWatermarkAsset() {
        return Identifier.of("lumen", "textures/gui/nametag_watermark.png");
    }
}
