package net.lumen.client.module.ghost;

import net.lumen.client.module.Category;
import net.lumen.client.module.Module;

/**
 * Placeholder for future legitimate functionality.
 *
 * This module is intentionally inert and hidden from the module list.
 * It does not send or modify network packets.
 */
public class DisablerModule extends Module {
    public DisablerModule() {
        super("Disabler", "Disabled placeholder; no packet modifications are performed.", Category.GHOST, 0);
    }
}
