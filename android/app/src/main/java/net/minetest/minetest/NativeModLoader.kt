package net.minetest.minetest

import android.content.Context
import android.util.Log

object NativeModLoader {
    private const val TAG = "NativeModLoader"

    fun loadAll(context: Context) {
        val mods = NativeModManager.getInstalledMods(context)
        for (mod in mods) {
            if (mod.enabled) {
                try {
                    Log.i(TAG, "Loading native mod: ${mod.name} from ${mod.path}")
                    System.load(mod.path)
                    Log.i(TAG, "Successfully loaded ${mod.name}")
                } catch (e: UnsatisfiedLinkError) {
                    Log.e(TAG, "Failed to load native mod: ${mod.name}", e)
                } catch (e: Exception) {
                    Log.e(TAG, "Unexpected error loading native mod: ${mod.name}", e)
                }
            } else {
                Log.i(TAG, "Skipping disabled native mod: ${mod.name}")
            }
        }
    }
}
