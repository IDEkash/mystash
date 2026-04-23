package net.minetest.minetest.jvm;

import android.util.Log;
import androidx.annotation.Keep;

@Keep
public class EngineAPIImpl implements EngineAPI {
    @Override
    public native void spawnEntity(String id, float x, float y, float z);

    @Override
    public native void registerModelFormat(String extension, ModelParser parser);

    @Override
    public native void registerMesh(String name, byte[] data);

    @Override
    public native void setFOV(int fov);

    @Override
    public void openSocket(String host, int port) {
        Log.i("EngineAPI", "Opening socket to " + host + ":" + port);
        new Thread(() -> {
            try {
                java.net.Socket socket = new java.net.Socket(host, port);
                Log.i("EngineAPI", "Socket connected to " + host + ":" + port);
                // Mods have raw access so they can use standard Java APIs
            } catch (Exception e) {
                Log.e("EngineAPI", "Failed to connect to " + host + ":" + port, e);
            }
        }).start();
    }

    @Override
    public native byte[] readFile(String path);

    @Override
    public native void writeFile(String path, byte[] data);

    @Override
    public native void log(String message);
}
