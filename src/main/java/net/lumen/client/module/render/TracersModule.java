package net.lumen.client.module.render;

import net.lumen.client.module.Category;
import net.lumen.client.module.Module;

/**
 * Temporarily disabled while the original implementation is ported to the
 * Minecraft 1.21.11 rendering pipeline. Original source is preserved under
 * src/main/resources/disabled-module-sources/.
 */
public final class TracersModule extends Module {
    public TracersModule() {
        super("Tracers", "Temporarily disabled pending a Minecraft 1.21.11 rendering port.", Category.RENDER, 0);
    }
}
