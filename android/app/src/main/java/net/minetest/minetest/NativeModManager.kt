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
        val mods = mutableListOf<NativeMod>()
        findModsRecursively(context, dir, mods)
        return mods
    }

    private fun findModsRecursively(context: Context, dir: File, mods: MutableList<NativeMod>) {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val files = dir.listFiles() ?: return
        for (file in files) {
            if (file.isDirectory) {
                findModsRecursively(context, file, mods)
            } else if (file.name.endsWith(".so")) {
                mods.add(NativeMod(
                    name = file.name,
                    path = file.absolutePath,
                    enabled = prefs.getBoolean(file.name, true)
                ))
            }
        }
    }

    fun setModEnabled(context: Context, modName: String, enabled: Boolean) {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit().putBoolean(modName, enabled).apply()
    }

    fun importMod(context: Context, uri: Uri): Boolean {
        try {
            val contentResolver = context.contentResolver
            val fileName = getFileName(context, uri) ?: return false

            // Security: sanitize file name to prevent path traversal
            val sanitizedFileName = File(fileName).name
            if (sanitizedFileName.isEmpty() || sanitizedFileName == ".." || sanitizedFileName == ".") {
                return false
            }

            if (sanitizedFileName.endsWith(".so")) {
                val destFile = File(getModDir(context), sanitizedFileName)
                contentResolver.openInputStream(uri)?.use { input ->
                    FileOutputStream(destFile).use { output ->
                        input.copyTo(output)
                    }
                }
                return true
            } else if (sanitizedFileName.endsWith(".zip")) {
                val modDir = getModDir(context)
                val outDir = File(modDir, sanitizedFileName.removeSuffix(".zip"))
                if (!outDir.exists()) outDir.mkdirs()

                contentResolver.openInputStream(uri)?.use { input ->
                    java.util.zip.ZipInputStream(input).use { zis ->
                        var entry = zis.nextEntry
                        while (entry != null) {
                            val outFile = File(outDir, entry.name)
                            if (!outFile.canonicalPath.startsWith(outDir.canonicalPath)) {
                                throw SecurityException("Zip traversal attempt: ${entry.name}")
                            }
                            if (entry.isDirectory) {
                                outFile.mkdirs()
                            } else {
                                outFile.parentFile?.mkdirs()
                                FileOutputStream(outFile).use { output ->
                                    zis.copyTo(output)
                                }
                            }
                            zis.closeEntry()
                            entry = zis.nextEntry
                        }
                    }
                }
                return true
            }
            return false
        } catch (e: Exception) {
            Log.e(TAG, "Failed to import mod", e)
            return false
        }
    }

    fun deleteMod(context: Context, modName: String): Boolean {
        val dir = getModDir(context)
        val file = findFileRecursively(dir, modName)
        if (file != null && file.exists()) {
            val deleted = if (file.isDirectory) file.deleteRecursively() else file.delete()
            if (deleted) {
                val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
                prefs.edit().remove(modName).apply()
            }
            return deleted
        }
        return false
    }

    private fun findFileRecursively(dir: File, name: String): File? {
        val files = dir.listFiles() ?: return null
        for (file in files) {
            if (file.name == name) return file
            if (file.isDirectory) {
                val found = findFileRecursively(file, name)
                if (found != null) return found
            }
        }
        return null
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
