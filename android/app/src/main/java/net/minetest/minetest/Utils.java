package net.minetest.minetest;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.Log;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.Objects;

public class Utils {
	@NonNull
	public static File createDirs(@NonNull File root, @NonNull String dir) {
		File f = new File(root, dir);
		if (!f.isDirectory())
			if (!f.mkdirs())
				Log.e("Utils", "Directory " + dir + " cannot be created");

		return f;
	}

	@NonNull
	public static File getUserDataDirectory(@NonNull Context context) {
		File extDir = Objects.requireNonNull(
			context.getExternalFilesDir(null),
			"Cannot get external file directory"
		);
		return createDirs(extDir, "Minetest");
	}

	@NonNull
	public static File getCacheDirectory(@NonNull Context context) {
		return Objects.requireNonNull(
			context.getCacheDir(),
			"Cannot get cache directory"
		);
	}

	public static boolean isInstallValid(@NonNull Context context) {
		File userDataDirectory = getUserDataDirectory(context);
		return userDataDirectory.isDirectory() &&
			new File(userDataDirectory, "builtin").isDirectory() &&
			new File(userDataDirectory, "client").isDirectory() &&
			new File(userDataDirectory, "textures").isDirectory();
	}

	public static String getFileName(Context context, Uri uri) {
		String result = null;
		if (Objects.equals(uri.getScheme(), "content")) {
			try (Cursor cursor = context.getContentResolver().query(uri, null, null, null, null)) {
				if (cursor != null && cursor.moveToFirst()) {
					int idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
					if (idx != -1) result = cursor.getString(idx);
				}
			}
		}
		if (result == null) {
			result = uri.getPath();
			if (result != null) {
				int cut = result.lastIndexOf('/');
				if (cut != -1) {
					result = result.substring(cut + 1);
				}
			}
		}
		return result;
	}

	public static File copyUriToTempFile(Context context, Uri uri) {
		String fileName = getFileName(context, uri);
		if (fileName == null) fileName = "import.zip";
		try (InputStream inputStream = context.getContentResolver().openInputStream(uri)) {
			if (inputStream == null) return null;
			File tempFile = new File(context.getCacheDir(), fileName);
			try (FileOutputStream outputStream = new FileOutputStream(tempFile)) {
				byte[] buffer = new byte[8192];
				int length;
				while ((length = inputStream.read(buffer)) > 0) {
					outputStream.write(buffer, 0, length);
				}
				return tempFile;
			}
		} catch (Exception e) {
			Log.e("Utils", "Failed to copy URI to temp file", e);
			return null;
		}
	}
}
