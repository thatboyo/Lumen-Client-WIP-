package dev.lumin.bridge.mixin;

import net.minecraft.client.option.SimpleOption;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/**
 * Fullbright needs a gamma value above 1.0, but {@link SimpleOption#setValue} clamps to the
 * option's slider range. Writing the backing field skips the callback, which is exactly what we
 * want: no validation, no clamping, and the lightmap reads it next frame.
 */
@Mixin(SimpleOption.class)
public interface SimpleOptionAccessor {
    @Accessor("value")
    void lumin$setValue(Object value);
}
