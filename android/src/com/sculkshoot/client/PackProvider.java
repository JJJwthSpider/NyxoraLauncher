package com.sculkshoot.client;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import java.io.File;
import java.io.FileNotFoundException;

/** Gives other apps (Minecraft launchers, messengers, file managers) read access to the exported .mrpack files and control layouts. */
public final class PackProvider extends ContentProvider {
    static final String AUTHORITY = "com.sculkshoot.client.packs";

    static Uri uriFor(File file) {
        return Uri.parse("content://" + AUTHORITY + "/" + Uri.encode(file.getName()));
    }

    static File exportDir(android.content.Context context) {
        File dir = new File(context.getCacheDir(), "exports");
        dir.mkdirs();
        return dir;
    }

    private File fileFor(Uri uri) throws FileNotFoundException {
        String name = uri.getLastPathSegment();
        if (name == null || name.contains("/") || name.startsWith(".")) {
            throw new FileNotFoundException();
        }
        File file = new File(exportDir(getContext()), name);
        if (!file.isFile()) {
            throw new FileNotFoundException(name);
        }
        return file;
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) {
            throw new SecurityException("read only");
        }
        return ParcelFileDescriptor.open(fileFor(uri), ParcelFileDescriptor.MODE_READ_ONLY);
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        try {
            File file = fileFor(uri);
            MatrixCursor cursor = new MatrixCursor(new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE });
            cursor.addRow(new Object[] { file.getName(), file.length() });
            return cursor;
        } catch (FileNotFoundException e) {
            return null;
        }
    }

    @Override
    public String getType(Uri uri) {
        String name = uri.getLastPathSegment();
        return name != null && name.endsWith(".json") ? "application/json" : "application/x-modrinth-modpack+zip";
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }
}
