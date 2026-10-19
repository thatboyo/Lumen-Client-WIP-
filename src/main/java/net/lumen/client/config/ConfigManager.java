package net.lumen.client.config;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import net.lumen.client.LumenClient;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.module.ModuleManager;
import net.lumen.client.theme.LumenTheme;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Collectors;

public class ConfigManager {
    private static final String ROOT_FOLDER = ".minecraft/lumen";
    private static final String PROFILES_FOLDER = ROOT_FOLDER + "/profiles";
    private static final String DEFAULT_PROFILE = "default.json";
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private File baseFolder;
    private File profilesFolder;
    private String activeProfileName = DEFAULT_PROFILE;

    public void initialize() {
        baseFolder = new File(ROOT_FOLDER);
        profilesFolder = new File(PROFILES_FOLDER);
        if (!baseFolder.exists()) {
            baseFolder.mkdirs();
        }
        if (!profilesFolder.exists()) {
            profilesFolder.mkdirs();
        }
        loadProfile(activeProfileName);
    }

    public void saveProfile(String name) {
        if (name == null || name.isBlank()) {
            name = DEFAULT_PROFILE;
        }
        File profileFile = new File(profilesFolder, name);
        ProfileData profileData = buildCurrentProfile();
        try (FileWriter writer = new FileWriter(profileFile)) {
            gson.toJson(profileData, writer);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void loadProfile(String name) {
        if (name == null || name.isBlank()) {
            name = DEFAULT_PROFILE;
        }
        activeProfileName = name;
        File profileFile = new File(profilesFolder, name);
        if (!profileFile.exists()) {
            saveProfile(name);
            return;
        }
        try (FileReader reader = new FileReader(profileFile)) {
            ProfileData profileData = gson.fromJson(reader, ProfileData.class);
            if (profileData != null) {
                applyProfile(profileData);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void deleteProfile(String name) {
        File profileFile = new File(profilesFolder, name);
        if (profileFile.exists()) {
            profileFile.delete();
        }
    }

    public List<String> listProfiles() {
        if (profilesFolder == null || !profilesFolder.exists()) {
            return Collections.emptyList();
        }
        String[] files = profilesFolder.list((dir, filename) -> filename.endsWith(".json"));
        if (files == null) {
            return Collections.emptyList();
        }
        return Arrays.stream(files).sorted().toList();
    }

    public static LumenTheme loadTheme() {
        // Load theme from saved profile if available. In this skeleton we default to GHOST.
        return LumenTheme.LUMIN;
    }

    private ProfileData buildCurrentProfile() {
        ModuleManager moduleManager = LumenClient.getModuleManager();
        ProfileData data = new ProfileData();
        data.theme = LumenTheme.active.name();
        data.guiStyle = "CLASSIC";
        data.modules = new TreeMap<>();
        if (moduleManager != null) {
            for (Module module : moduleManager.getAllModules()) {
                ModuleData moduleData = new ModuleData();
                moduleData.enabled = module.isEnabled();
                moduleData.settings = new TreeMap<>();
                module.getSettings().forEach(setting -> {
                    Object value = setting.getValue();
                    moduleData.settings.put(setting.getName(), value);
                });
                data.modules.put(module.getName(), moduleData);
            }
        }
        return data;
    }

    private void applyProfile(ProfileData profileData) {
        ThemeManagerHolder.apply(profileData.theme);
        ModuleManager moduleManager = LumenClient.getModuleManager();
        if (moduleManager == null) {
            return;
        }
        for (Map.Entry<String, ModuleData> entry : profileData.modules.entrySet()) {
            Module module = moduleManager.getModule(entry.getKey());
            if (module == null) {
                continue;
            }
            if (entry.getValue().enabled && !module.isEnabled()) {
                module.enable();
            } else if (!entry.getValue().enabled && module.isEnabled()) {
                module.disable();
            }
            for (var setting : module.getSettings()) {
                if (entry.getValue().settings.containsKey(setting.getName())) {
                    Object value = entry.getValue().settings.get(setting.getName());
                    try {
                        Field field = setting.getClass().getDeclaredField("value");
                        field.setAccessible(true);
                        field.set(setting, value);
                    } catch (Exception ignored) {
                    }
                }
            }
        }
    }

    private static class ThemeManagerHolder {
        public static void apply(String themeName) {
            try {
                LumenTheme theme = LumenTheme.valueOf(themeName);
                LumenClient.getThemeManager().applyTheme(theme);
            } catch (Exception ignored) {
            }
        }
    }

    private static class ProfileData {
        public String theme;
        public String guiStyle;
        public Map<String, ModuleData> modules = new TreeMap<>();
    }

    private static class ModuleData {
        public boolean enabled;
        public Map<String, Object> settings = new TreeMap<>();
    }
}
