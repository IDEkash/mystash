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
        File modsDir = new File(context.getFilesDir(), "mods");
        Log.i(TAG, "Scanning for JVM mods in " + modsDir.getAbsolutePath());
        if (!modsDir.exists()) {
            modsDir.mkdirs();
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
                // Try looking for any jar file if the name doesn't match ID
                File[] files = modDir.listFiles((dir, name) -> name.endsWith(".jar"));
                if (files != null && files.length > 0) {
                    jarFile = files[0];
                } else {
                    Log.e(TAG, "Jar file not found for mod: " + modId);
                    return;
                }
            }

            Log.i(TAG, "Loading mod " + modId + " with entry " + entryClass);

            DexClassLoader classLoader = new DexClassLoader(
                jarFile.getAbsolutePath(),
                context.getDir("dex", Context.MODE_PRIVATE).getAbsolutePath(),
                null,
                context.getClassLoader()
            );

            Class<?> clazz = classLoader.loadClass(entryClass);
            clazz.getConstructor(EngineAPI.class).newInstance(engineAPI);
            Log.i(TAG, "Successfully instantiated mod: " + modId);

        } catch (Exception e) {
            Log.e(TAG, "Failed to load mod in " + modDir.getAbsolutePath(), e);
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
