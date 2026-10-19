# Lumin desktop themes recovered from the packaged executable

Source inspected: the user-provided `Lumin.exe`, a Windows PE32+ executable containing the compiled WPF resources `themes/lumin.baml`, `themes/rain.baml`, and `themes/synth.baml`.

The executable includes the following theme descriptions:
- **Lumin:** “Lumin keeps the interface monochrome with a restrained icy blue-white module glow.”
- **Rain:** “Rain brings storm-slate glass and cool aqua highlights.”
- **Synth:** “Synth pairs deep violet surfaces with neon rose and cyan accents.”

The values below are the palette values recovered from the compiled theme resource strings. Hex values are ARGB (`#AARRGGBB`); the Java source uses the equivalent `0xAARRGGBB` integers. WPF blur, gradients, and glass effects may need approximations in Minecraft's `DrawContext` renderer.

| Role | Lumin | Rain | Synth |
|---|---|---|---|
| Main background | `#FF000001` | `#FF050A0D` | `#FF08050F` |
| Main panel | `#FF050609` | `#FF0B141A` | `#FF151020` |
| Glass border | `#35FFFFFF` | `#2A8EEBFF` | `#33EBD4FF` |
| Primary accent | `#FFDDF5FF` | `#FF7ADCF2` | `#FFFF6BD6` |
| Muted text | `#FF7F8792` | `#FF728C96` | `#FF90839D` |
| Bright enabled indicator | `#FFFFFFFF` | `#FFE5FAFF` | `#FFFFE8FB` |
| Accent halo | `#99DDF5FF` | `#777ADCF2` | `#88FF6BD6` |
| Enabled surface | `#B50B0F15` | `#C5162831` | `#C51F1729` |
| Selected surface | `#D70D1219` | `#E0182E38` | `#E0281A35` |

Additional theme-resource colors include subtle hover/overlay variants. The current Java palette preserves the main colors and representative translucent surface colors; it does not yet reproduce every WPF control template, animation, blur, or gradient.

The palette implementation is in `src/main/java/net/lumen/client/theme/LumenTheme.java`. The Minecraft GUI uses solid-color drawing, so visual fidelity is an approximation rather than a direct execution of the original WPF BAML.
