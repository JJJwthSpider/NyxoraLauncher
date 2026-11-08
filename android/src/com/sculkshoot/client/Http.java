package com.sculkshoot.client;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;

/** Small blocking HTTP client; always called from a background thread. */
final class Http {
    private Http() {}

    static final String USER_AGENT = "Vellora-Client-Android/1.0 (github.com/nvsdjonvljw/Script)";

    static final class Response {
        final int status;
        final String body;

        Response(int status, String body) {
            this.status = status;
            this.body = body;
        }

        boolean ok() {
            return status == 200;
        }
    }

    static Response get(String url) throws IOException {
        return request("GET", url, null, null, (String[]) null);
    }

    /** a GET with extra headers: name, value, name, value... */
    static Response get(String url, String... headers) throws IOException {
        return request("GET", url, null, null, headers);
    }

    /** a POST of a form or JSON body (contentType says which), with extra headers */
    static Response post(String url, String contentType, String body, String... headers) throws IOException {
        return request("POST", url, contentType, body, headers);
    }

    /** a PUT with a JSON body (Minecraft services: the active cape) */
    static Response put(String url, String json, String... headers) throws IOException {
        return request("PUT", url, "application/json", json, headers);
    }

    static Response delete(String url, String... headers) throws IOException {
        return request("DELETE", url, null, null, headers);
    }

    /** an application/x-www-form-urlencoded body: name, value, name, value... */
    static String form(String... pairs) {
        StringBuilder out = new StringBuilder();
        for (int i = 0; i + 1 < pairs.length; i += 2) {
            out.append(out.length() == 0 ? "" : "&").append(encode(pairs[i])).append('=').append(encode(pairs[i + 1]));
        }
        return out.toString();
    }

    private static Response request(String method, String url, String contentType, String body, String[] headers) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setRequestMethod(method);
        connection.setRequestProperty("User-Agent", USER_AGENT);
        connection.setRequestProperty("Accept", "application/json");
        for (int i = 0; headers != null && i + 1 < headers.length; i += 2) {
            connection.setRequestProperty(headers[i], headers[i + 1]);
        }
        connection.setConnectTimeout(20000);
        connection.setReadTimeout(30000);
        try {
            if (body != null) {
                byte[] bytes = body.getBytes("UTF-8");
                connection.setDoOutput(true);
                connection.setRequestProperty("Content-Type", contentType);
                connection.setFixedLengthStreamingMode(bytes.length);
                java.io.OutputStream out = connection.getOutputStream();
                try {
                    out.write(bytes);
                } finally {
                    out.close();
                }
            }
            int status;
            try {
                status = connection.getResponseCode();
            } catch (IOException e) {
                // some Android versions throw on "401" without a WWW-Authenticate header (Xbox Live answers so);
                // the status is known by then
                if (e.getMessage() != null && e.getMessage().contains("authentication challenge")) {
                    status = 401;
                } else {
                    throw e;
                }
            }
            InputStream in = status >= 400 ? connection.getErrorStream() : connection.getInputStream();
            return new Response(status, in == null ? "" : read(in));
        } finally {
            connection.disconnect();
        }
    }

    /** downloads a file (a modpack) */
    static void download(String url, java.io.File target) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setRequestProperty("User-Agent", USER_AGENT);
        connection.setConnectTimeout(20000);
        connection.setReadTimeout(60000);
        try {
            if (connection.getResponseCode() != 200) {
                throw new IOException("HTTP " + connection.getResponseCode());
            }
            InputStream in = connection.getInputStream();
            java.io.FileOutputStream out = new java.io.FileOutputStream(target);
            try {
                byte[] buffer = new byte[65536];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            } finally {
                in.close();
                out.close();
            }
        } finally {
            connection.disconnect();
        }
    }

    static String read(InputStream in) throws IOException {
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buffer = new byte[16384];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
            return out.toString("UTF-8");
        } finally {
            in.close();
        }
    }

    static String encode(String value) {
        try {
            return URLEncoder.encode(value, "UTF-8").replace("+", "%20");
        } catch (IOException e) {
            return value;
        }
    }
}
