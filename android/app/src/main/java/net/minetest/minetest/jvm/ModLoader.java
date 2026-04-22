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

    public ModLoader(Context context) {
        this.context = context;
        this.engineAPI = new EngineAPIImpl();
    }

    @Keep
    public void loadMods() {
        // Internal storage
        scanDirectory(new File(context.getFilesDir(), "mods"));

        // External (accessible) storage
        try {
            File externalDir = new File(Utils.getUserDataDirectory(context), "jvm_mods");
            scanDirectory(externalDir);
        } catch (Exception e) {
            Log.e(TAG, "Could not access external mods directory", e);
        }
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
            Log.w(TAG, "No mod.json found in " + modDir.getName());
            return;
        }

        try {
            String jsonStr = readFile(configFile);
            JSONObject json = new JSONObject(jsonStr);
            String entryClass = json.getString("entry");
            String modId = json.getString("id");

            File jarFile = new File(modDir, modId + ".jar");
            if (!jarFile.exists()) {
                File[] files = modDir.listFiles((dir, name) -> name.endsWith(".jar"));
                if (files != null && files.length > 0) {
                    jarFile = files[0];
                } else {
                    Log.e(TAG, "Jar file not found for mod: " + modId + " in " + modDir.getAbsolutePath());
                    return;
                }
            }

            Log.i(TAG, "Attempting to load mod: " + modId + " [" + jarFile.getName() + "]");
            Log.d(TAG, "Entry class: " + entryClass);

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
                Log.i(TAG, ">>> Successfully loaded and started mod: " + modId);
            } catch (ClassNotFoundException e) {
                Log.e(TAG, "Entry class " + entryClass + " not found in " + jarFile.getName() + ". Did you dex your JAR?");
            } catch (NoSuchMethodException e) {
                Log.e(TAG, "Entry class must have a constructor that accepts EngineAPI: " + entryClass + "(EngineAPI engine)");
            }

        } catch (Exception e) {
            Log.e(TAG, "Critical failure loading mod in " + modDir.getAbsolutePath(), e);
        }
    }

    private String readFile(File file) throws Exception {
        try (FileInputStream stream = new FileInputStream(file)) {
            FileChannel fc = stream.getChannel();
            MappedByteBuffer bb = fc.map(FileChannel.MapMode.READ_ONLY, 0, fc.size());
            return Charset.defaultCharset().decode(bb).toString();
        }
    }
}
