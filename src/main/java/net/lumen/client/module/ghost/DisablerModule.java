package net.lumen.client.module.ghost;

import net.lumen.client.event.EventPacket;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.BooleanSetting;

import java.util.HashMap;
import java.util.Map;

/**
 * Outbound packet diagnostics only.
 *
 * This module observes packets already being sent by the client. It never cancels,
 * changes, fabricates, delays, or replays packets. It remains hidden from the normal
 * module UI while its behavior is under review.
 */
public class DisablerModule extends Module {
    private final BooleanSetting logPacketClasses;
    private final Map<String, Long> packetCounts = new HashMap<>();
    private long totalPacketsObserved;

    public DisablerModule() {
        super("Disabler", "Review-only packet diagnostics; observes outbound packet types without modifying traffic.", Category.GHOST, 0);

        logPacketClasses = new BooleanSetting(
                "Log packet classes",
                "Print each observed outbound packet's class name to the game log. May produce substantial output.",
                false
        );
        registerSetting(logPacketClasses);
    }

    @Override
    protected void onEnable() {
        totalPacketsObserved = 0;
        packetCounts.clear();
        subscribe(EventPacket.class, this::onPacket);
        System.out.println("[Lumen Packet Diagnostics] Enabled. Observing outbound packets only; no packets will be modified.");
    }

    @Override
    protected void onDisable() {
        System.out.println("[Lumen Packet Diagnostics] Disabled. Observed " + totalPacketsObserved
                + " outbound packets across " + packetCounts.size() + " packet types.");
        packetCounts.entrySet().stream()
                .sorted(Map.Entry.<String, Long>comparingByValue().reversed())
                .limit(10)
                .forEach(entry -> System.out.println("[Lumen Packet Diagnostics] "
                        + entry.getKey() + ": " + entry.getValue()));
    }

    private void onPacket(EventPacket event) {
        if (event == null || event.packet == null) {
            return;
        }

        // EventPacket is currently posted by ClientPlayNetworkHandlerMixin.sendPacket,
        // so these counts represent outbound client packets, not inbound server packets.
        String packetType = event.packet.getClass().getSimpleName();
        totalPacketsObserved++;
        packetCounts.merge(packetType, 1L, Long::sum);

        if (Boolean.TRUE.equals(logPacketClasses.getValue())) {
            System.out.println("[Lumen Packet Diagnostics] outbound #" + totalPacketsObserved + ": " + packetType);
        }

        // Intentionally do not read or write event.cancelled and do not alter event.packet.
    }

    public long getTotalPacketsObserved() {
        return totalPacketsObserved;
    }

    public Map<String, Long> getPacketCounts() {
        return Map.copyOf(packetCounts);
    }
}
