package dev.lumin.bridge.mixin;

import dev.lumin.bridge.Bridge;
import net.minecraft.client.MinecraftClient;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

/** Tail of the client tick: after world/player have ticked, so velocity writes stick. */
@Mixin(MinecraftClient.class)
public class ClientTickMixin {
    @Inject(method = "tick", at = @At("TAIL"))
    private void lumin$onClientTick(CallbackInfo ci) {
        Bridge.onClientTick((MinecraftClient) (Object) this);
    }
}
