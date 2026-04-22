package net.minetest.minetest.jvm;

public interface EngineAPI {
    void spawnEntity(String id, float x, float y, float z);
    void registerModelFormat(String extension, ModelParser parser);
    void setFOV(int fov);
    void openSocket(String host, int port);
    String readFile(String path);
    void writeFile(String path, byte[] data);
}
