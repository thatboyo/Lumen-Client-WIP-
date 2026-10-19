package net.lumen.client.setting;

import net.minecraft.client.util.InputUtil;

public class KeybindSetting extends Setting<Integer> {
    public KeybindSetting(String name, String description, int defaultKey) {
        super(name, description, defaultKey);
    }

    public InputUtil.Key getKey() {
        return InputUtil.Type.KEYSYM.createFromCode(getValue());
    }
}
