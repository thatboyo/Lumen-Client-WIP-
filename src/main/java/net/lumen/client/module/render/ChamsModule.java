package net.lumen.client.module.render;

import net.lumen.client.module.Category;
import net.lumen.client.module.Module;

/**
 * Temporarily disabled: the prior implementation changed global render state
 * but did not hook entity rendering, and used removed APIs. Original source is
 * preserved under src/main/resources/disabled-module-sources/.
 */
public final class ChamsModule extends Module {
    public ChamsModule() {
        super("Chams", "Temporarily disabled pending a safe Minecraft 1.21.11 rendering port.", Category.RENDER, 0);
    }
}
