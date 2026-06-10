package net.minetest.minetest

import android.content.Context
import android.net.Uri
import android.util.Log
import java.io.File
import java.io.FileOutputStream

data class NativeMod(
    val name: String,
    val path: String,
    var enabled: Boolean
)

object NativeModManager {
    private const val MOD_DIR = "native_mods"
    private const val PREFS_NAME = "NativeModPrefs"
    private const val TAG = "NativeModManager"

    fun getModDir(context: Context): File {
        val dir = File(context.filesDir, MOD_DIR)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        return dir
    }

    fun getInstalledMods(context: Context): List<NativeMod> {
        val dir = getModDir(context)
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

        return dir.listFiles { _, name -> name.endsWith(".so") }?.map { file ->
            NativeMod(
                name = file.name,
                path = file.absolutePath,
                enabled = prefs.getBoolean(file.name, true)
            )
        } ?: emptyList()
    }

    fun setModEnabled(context: Context, modName: String, enabled: Boolean) {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit().putBoolean(modName, enabled).apply()
    }

    fun importMod(context: Context, uri: Uri): Boolean {
        try {
            val contentResolver = context.contentResolver
            val fileName = getFileName(context, uri) ?: return false
            if (!fileName.endsWith(".so")) return false

            // Security: sanitize file name to prevent path traversal
            val sanitizedFileName = File(fileName).name
            if (sanitizedFileName.isEmpty() || sanitizedFileName == ".." || sanitizedFileName == ".") {
                return false
            }

            val destFile = File(getModDir(context), sanitizedFileName)
            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(destFile).use { output ->
                    input.copyTo(output)
                }
            }
            return true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to import mod", e)
            return false
        }
    }

    fun deleteMod(context: Context, modName: String): Boolean {
        val file = File(getModDir(context), modName)
        if (file.exists()) {
            val deleted = file.delete()
            if (deleted) {
                val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
                prefs.edit().remove(modName).apply()
            }
            return deleted
        }
        return false
    }

    private fun getFileName(context: Context, uri: Uri): String? {
        var name: String? = null
        val cursor = context.contentResolver.query(uri, null, null, null, null)
        cursor?.use {
            if (it.moveToFirst()) {
                val nameIndex = it.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
                if (nameIndex != -1) {
                    name = it.getString(nameIndex)
                }
            }
        }
        return name ?: uri.path?.let { path ->
            val cut = path.lastIndexOf('/')
            if (cut != -1) path.substring(cut + 1) else path
        }
    }
}
