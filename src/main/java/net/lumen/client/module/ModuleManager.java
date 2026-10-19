package net.lumen.client.module;

import net.lumen.client.module.combat.AntiKnockbackModule;
import net.lumen.client.module.combat.AutoClickerModule;
import net.lumen.client.module.combat.AutoHitModule;
import net.lumen.client.module.combat.AutoPotModule;
import net.lumen.client.module.combat.CriticalsModule;
import net.lumen.client.module.combat.ReachModule;
import net.lumen.client.module.combat.SilentAuraModule;
import net.lumen.client.module.combat.VelocityModule;
import net.lumen.client.module.ghost.AntiAimModule;
import net.lumen.client.module.ghost.BlinkModule;
import net.lumen.client.module.ghost.DisablerModule;
import net.lumen.client.module.ghost.MLGWaterModule;
import net.lumen.client.module.ghost.ScaffoldGhostModule;
import net.lumen.client.module.ghost.TimerHackModule;
import net.lumen.client.module.legit.ChatSuffixModule;
import net.lumen.client.module.legit.FreeCamModule;
import net.lumen.client.module.legit.FullBrightModule;
import net.lumen.client.module.legit.ZoomModule;
import net.lumen.client.module.movement.AutoJumpModule;
import net.lumen.client.module.movement.FlyModule;
import net.lumen.client.module.movement.NoSlowModule;
import net.lumen.client.module.movement.SprintModule;
import net.lumen.client.module.movement.SpeedModule;
import net.lumen.client.module.movement.StrafeModule;
import net.lumen.client.module.movement.ScaffoldModule;
import net.lumen.client.module.player.ArrayListModule;
import net.lumen.client.module.player.ArmorDisplayModule;
import net.lumen.client.module.player.ClockModule;
import net.lumen.client.module.player.CoordinatesModule;
import net.lumen.client.module.player.CPSDisplayModule;
import net.lumen.client.module.player.FPSDisplayModule;
import net.lumen.client.module.player.HitboxDisplayModule;
import net.lumen.client.module.player.KeystrokesModule;
import net.lumen.client.module.player.PingDisplayModule;
import net.lumen.client.module.player.PotCounterModule;
import net.lumen.client.module.player.WatermarkModule;
import net.lumen.client.module.render.BlockESPModule;
import net.lumen.client.module.render.ChamsModule;
import net.lumen.client.module.render.ESPModule;
import net.lumen.client.module.render.HitboxESPModule;
import net.lumen.client.module.render.NametagsModule;
import net.lumen.client.module.render.TracersModule;
import net.lumen.client.module.render.ViewModelModule;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;

public class ModuleManager {
    private final List<Module> modules = new ArrayList<>();

    public void initialize() {
        modules.clear();
        modules.add(new SilentAuraModule());
        modules.add(new AutoHitModule());
        modules.add(new ReachModule());
        modules.add(new VelocityModule());
        modules.add(new AntiKnockbackModule());
        modules.add(new AutoClickerModule());
        modules.add(new CriticalsModule());
        modules.add(new AutoPotModule());
        modules.add(new MLGWaterModule());
        modules.add(new BlinkModule());
        modules.add(new TimerHackModule());
        modules.add(new ScaffoldGhostModule());
        modules.add(new AntiAimModule());
        modules.add(new DisablerModule());
        modules.add(new SprintModule());
        modules.add(new NoSlowModule());
        modules.add(new SpeedModule());
        modules.add(new FlyModule());
        modules.add(new ScaffoldModule());
        modules.add(new AutoJumpModule());
        modules.add(new StrafeModule());
        modules.add(new ESPModule());
        modules.add(new TracersModule());
        modules.add(new ChamsModule());
        modules.add(new NametagsModule());
        modules.add(new HitboxESPModule());
        modules.add(new BlockESPModule());
        modules.add(new ViewModelModule());
        modules.add(new ArrayListModule());
        modules.add(new WatermarkModule());
        modules.add(new CoordinatesModule());
        modules.add(new FPSDisplayModule());
        modules.add(new KeystrokesModule());
        modules.add(new ClockModule());
        modules.add(new PingDisplayModule());
        modules.add(new CPSDisplayModule());
        modules.add(new HitboxDisplayModule());
        modules.add(new FullBrightModule());
        modules.add(new ZoomModule());
        modules.add(new FreeCamModule());
        modules.add(new ChatSuffixModule());
    }

    public List<Module> getModules(Category category) {
        if (category == Category.FAVORITE) {
            return getFavorites();
        }
        return modules.stream().filter(ModuleVisibility::isVisible).filter(module -> module.getCategory() == category).collect(Collectors.toList());
    }

    public Module getModule(String name) {
        return modules.stream().filter(ModuleVisibility::isVisible).filter(module -> module.getName().equalsIgnoreCase(name)).findFirst().orElse(null);
    }

    public List<Module> getFavorites() {
        return modules.stream().filter(ModuleVisibility::isVisible).filter(Module::isFavorited).collect(Collectors.toList());
    }

    public List<Module> getGhostMirrors() {
        return modules.stream().filter(ModuleVisibility::isVisible).filter(Module::isGhost).collect(Collectors.toList());
    }

    public List<Module> getEnabled() {
        return modules.stream()
                .filter(ModuleVisibility::isVisible)
                .filter(Module::isEnabled)
                .sorted(Comparator.comparingInt(module -> -module.getName().length()))
                .toList();
    }

    public List<Module> getAllModules() {
        return modules.stream().filter(ModuleVisibility::isVisible).collect(Collectors.toCollection(ArrayList::new));
    }
}
