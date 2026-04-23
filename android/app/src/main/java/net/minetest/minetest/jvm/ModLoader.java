package net.minetest.minetest.jvm;

import android.content.Context;
import android.util.Log;
import dalvik.system.DexClassLoader;
import org.json.JSONObject;
import java.io.File;
import java.io.FileInputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import androidx.annotation.Keep;
import net.minetest.minetest.Utils;

@Keep
public class ModLoader {
    private static final String TAG = "ModLoader";
    private Context context;
    private EngineAPI engineAPI;
    private static boolean modsLoaded = false;

    public ModLoader(Context context) {
        this.context = context;
        this.engineAPI = new EngineAPIImpl();
    }

    @Keep
    public void loadMods() {
        if (modsLoaded) {
            Log.i(TAG, "JVM Mods already loaded, skipping.");
            return;
        }

        Log.i(TAG, "Starting JVM Mod Loading process...");

        File userDataDir = Utils.getUserDataDirectory(context);

        // 1. Scan standard "mods" directory
        scanDirectory(new File(userDataDir, "mods"));

        // 2. Scan "mods/mods" directory as requested
        scanDirectory(new File(userDataDir, "mods/mods"));

        // 3. Scan dedicated "jvm_mods" directory
        scanDirectory(new File(userDataDir, "jvm_mods"));

        // 4. Scan internal storage
        scanDirectory(new File(context.getFilesDir(), "mods"));

        modsLoaded = true;
    }

    private void scanDirectory(File modsDir) {
        Log.i(TAG, "Scanning for JVM mods in " + modsDir.getAbsolutePath());
        if (!modsDir.exists()) {
            modsDir.mkdirs();
            return;
        }

        File[] mods = modsDir.listFiles();
        if (mods == null) return;

        for (File modDir : mods) {
            if (modDir.isDirectory()) {
                loadMod(modDir);
            }
        }
    }

    private void loadMod(File modDir) {
        File configFile = new File(modDir, "mod.json");
        if (!configFile.exists()) {
            return;
        }

        try {
            String jsonStr = readTextFile(configFile);
            JSONObject json = new JSONObject(jsonStr);
            String entryClass = json.getString("entry");
            String modId = json.getString("id");

            File jarFile = new File(modDir, modId + ".jar");
            if (!jarFile.exists()) {
                File[] files = modDir.listFiles((dir, name) -> name.endsWith(".jar"));
                if (files != null && files.length > 0) {
                    jarFile = files[0];
                } else {
                    Log.e(TAG, "JVM Mod " + modId + " found but no .jar file in " + modDir.getAbsolutePath());
                    return;
                }
            }

            Log.i(TAG, ">>> Loading JVM Mod: " + modId + " from " + jarFile.getAbsolutePath());

            File optimizedDexDir = context.getDir("dex", Context.MODE_PRIVATE);

            DexClassLoader classLoader = new DexClassLoader(
                jarFile.getAbsolutePath(),
                optimizedDexDir.getAbsolutePath(),
                null,
                context.getClassLoader()
            );

            try {
                Class<?> clazz = classLoader.loadClass(entryClass);
                clazz.getConstructor(EngineAPI.class).newInstance(engineAPI);
                Log.i(TAG, ">>> JVM Mod Started: " + modId);
            } catch (ClassNotFoundException e) {
                Log.e(TAG, "Class " + entryClass + " not found. Ensure your JAR is dexed correctly.");
            } catch (NoSuchMethodException e) {
                Log.e(TAG, "Constructor " + entryClass + "(EngineAPI) not found.");
            } catch (Exception e) {
                Log.e(TAG, "Failed to instantiate mod: " + modId, e);
            }

        } catch (Exception e) {
            Log.e(TAG, "Error parsing mod.json in " + modDir.getAbsolutePath(), e);
        }
    }

    private String readTextFile(File file) throws Exception {
        try (FileInputStream stream = new FileInputStream(file)) {
            FileChannel fc = stream.getChannel();
            MappedByteBuffer bb = fc.map(FileChannel.MapMode.READ_ONLY, 0, fc.size());
            return Charset.defaultCharset().decode(bb).toString();
        }
    }
}
