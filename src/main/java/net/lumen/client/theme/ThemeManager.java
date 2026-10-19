package net.lumen.client.theme;

import net.lumen.client.config.ConfigManager;

public class ThemeManager {
    private LumenTheme loadedTheme = LumenTheme.LUMIN;

    public void initialize() {
        loadedTheme = ConfigManager.loadTheme();
        applyTheme(loadedTheme);
    }

    public void applyTheme(LumenTheme theme) {
        if (theme == null) {
            theme = LumenTheme.LUMIN;
        }
        LumenTheme.active = theme;
        loadedTheme = theme;
    }

    public LumenTheme getActiveTheme() {
        return loadedTheme;
    }
}
