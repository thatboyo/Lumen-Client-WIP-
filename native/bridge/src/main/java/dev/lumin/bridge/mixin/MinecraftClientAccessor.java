package dev.lumin.bridge.mixin;

import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

/**
 * {@code itemUseCooldown} is private and has no setter, and it is what spaces out block
 * placement and item use. FastPlace/FastUse write it directly.
 */
@Mixin(MinecraftClient.class)
public interface MinecraftClientAccessor {
    @Accessor("itemUseCooldown")
    void lumin$setItemUseCooldown(int value);
}
