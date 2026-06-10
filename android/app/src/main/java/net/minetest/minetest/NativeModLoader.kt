package net.minetest.minetest

import android.content.Context
import android.util.Log

object NativeModLoader {
    private const val TAG = "NativeModLoader"

    fun loadAll(context: Context) {
        val modDir = NativeModManager.getModDir(context)
        loadRecursively(context, modDir)
    }

    private fun loadRecursively(context: Context, dir: File) {
        val prefs = context.getSharedPreferences("NativeModPrefs", Context.MODE_PRIVATE)
        dir.listFiles()?.forEach { file ->
            if (file.isDirectory) {
                loadRecursively(context, file)
            } else if (file.name.endsWith(".so")) {
                val enabled = prefs.getBoolean(file.name, true)
                if (enabled) {
                    try {
                        Log.i(TAG, "Loading native mod: ${file.name} from ${file.absolutePath}")
                        System.load(file.absolutePath)
                        Log.i(TAG, "Successfully loaded ${file.name}")
                    } catch (e: UnsatisfiedLinkError) {
                        Log.e(TAG, "Failed to load native mod: ${file.name}", e)
                    } catch (e: Exception) {
                        Log.e(TAG, "Unexpected error loading native mod: ${file.name}", e)
                    }
                } else {
                    Log.i(TAG, "Skipping disabled native mod: ${file.name}")
                }
            }
        }
    }
}
