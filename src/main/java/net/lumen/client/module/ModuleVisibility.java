package net.lumen.client.module;

import java.util.Locale;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Central allow/deny list for modules that should not be exposed in the client UI.
 * Keep restricted or unfinished modules here until their implementations are reviewed.
 */
public final class ModuleVisibility {
    private static final Set<String> HIDDEN_MODULES = Set.of(
            "disabler"
    ).stream().map(name -> name.toLowerCase(Locale.ROOT)).collect(Collectors.toUnmodifiableSet());

    private ModuleVisibility() {
    }

    public static boolean isVisible(Module module) {
        return module != null && !isHiddenName(module.getName());
    }

    public static boolean isHiddenName(String name) {
        return name != null && HIDDEN_MODULES.contains(name.toLowerCase(Locale.ROOT));
    }
}
