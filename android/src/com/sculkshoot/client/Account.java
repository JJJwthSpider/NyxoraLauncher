package com.sculkshoot.client;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONArray;
import org.json.JSONObject;

/**
 * The player's Microsoft account, signed in like in the desktop launcher: a code entered on microsoft.com/link,
 * then Xbox Live, XSTS and Minecraft services give the Minecraft profile (name, UUID, skin). The refresh token stays
 * in the app's private storage, so the next start signs in silently. All network methods block (background thread).
 */
final class Account {
    /** the same Microsoft app as the desktop Lullaby Launcher (CMakeLists.txt: Launcher_MSA_CLIENT_ID) */
    static String CLIENT_ID = "c36a9fb6-4f2a-41ff-90bd-ae7cc92031eb";
    static String MICROSOFT = "https://login.microsoftonline.com/consumers/oauth2/v2.0";
    static String XBOX_USER = "https://user.auth.xboxlive.com/user/authenticate";
    static String XSTS = "https://xsts.auth.xboxlive.com/xsts/authorize";
    static String MINECRAFT = "https://api.minecraftservices.com";
    private static final String SCOPE = "XboxLive.signin offline_access";

    static final class LoginException extends Exception {
        LoginException(String message) {
            super(message);
        }
    }

    /** what the player sees while signing in: open this page and type this code */
    static final class DeviceCode {
        String deviceCode = "";
        String userCode = "";
        String verificationUri = "https://www.microsoft.com/link";
        int interval = 5;
        long expiresAt;
    }

    String name = "";
    String uuid = "";
    String skinUrl = "";
    boolean ownsJava;
    String refreshToken = "";
    String minecraftToken = "";
    long minecraftTokenExpires;

    boolean signedIn() {
        return !refreshToken.isEmpty();
    }

    /* ------------------------------------------------------------ storage */

    private static SharedPreferences prefs(Context context) {
        return context.getSharedPreferences("account", Context.MODE_PRIVATE);
    }

    static Account load(Context context) {
        SharedPreferences p = prefs(context);
        Account a = new Account();
        a.name = p.getString("name", "");
        a.uuid = p.getString("uuid", "");
        a.skinUrl = p.getString("skinUrl", "");
        a.ownsJava = p.getBoolean("ownsJava", false);
        a.refreshToken = p.getString("refreshToken", "");
        a.minecraftToken = p.getString("minecraftToken", "");
        a.minecraftTokenExpires = p.getLong("minecraftTokenExpires", 0);
        return a;
    }

    void save(Context context) {
        prefs(context).edit()
            .putString("name", name)
            .putString("uuid", uuid)
            .putString("skinUrl", skinUrl)
            .putBoolean("ownsJava", ownsJava)
            .putString("refreshToken", refreshToken)
            .putString("minecraftToken", minecraftToken)
            .putLong("minecraftTokenExpires", minecraftTokenExpires)
            .apply();
    }

    static void signOut(Context context) {
        prefs(context).edit().clear().apply();
    }

    /* ------------------------------------------------------------ Microsoft */

    /** step 1: a code the player types on microsoft.com/link */
    static DeviceCode startDeviceCode() throws Exception {
        Http.Response r = Http.post(MICROSOFT + "/devicecode", "application/x-www-form-urlencoded",
                                    Http.form("client_id", CLIENT_ID, "scope", SCOPE));
        if (!r.ok()) {
            throw new LoginException("Microsoft не выдал код входа (HTTP " + r.status + ")");
        }
        JSONObject o = new JSONObject(r.body);
        DeviceCode code = new DeviceCode();
        code.deviceCode = o.getString("device_code");
        code.userCode = o.getString("user_code");
        code.verificationUri = o.optString("verification_uri", code.verificationUri);
        code.interval = Math.max(1, o.optInt("interval", 5));
        code.expiresAt = System.currentTimeMillis() + o.optInt("expires_in", 900) * 1000L;
        return code;
    }

    /**
     * step 2: asks Microsoft once whether the player has entered the code. Returns the Microsoft access token, or
     * null while the player has not finished yet (call again after code.interval seconds).
     */
    String pollDeviceCode(DeviceCode code) throws Exception {
        if (System.currentTimeMillis() > code.expiresAt) {
            throw new LoginException("Код устарел. Нажми «Войти» ещё раз.");
        }
        Http.Response r = Http.post(MICROSOFT + "/token", "application/x-www-form-urlencoded",
                                    Http.form("grant_type", "urn:ietf:params:oauth:grant-type:device_code", "client_id", CLIENT_ID,
                                              "device_code", code.deviceCode));
        JSONObject o = new JSONObject(r.body.isEmpty() ? "{}" : r.body);
        if (r.ok()) {
            refreshToken = o.optString("refresh_token", refreshToken);
            return o.getString("access_token");
        }
        String error = o.optString("error");
        if ("authorization_pending".equals(error)) {
            return null;
        }
        if ("slow_down".equals(error)) {
            code.interval += 5;
            return null;
        }
        if ("authorization_declined".equals(error)) {
            throw new LoginException("Вход отменён на странице Microsoft.");
        }
        if ("expired_token".equals(error)) {
            throw new LoginException("Код устарел. Нажми «Войти» ещё раз.");
        }
        throw new LoginException("Microsoft отказал во входе: " + (error.isEmpty() ? "HTTP " + r.status : error));
    }

    /** a new Microsoft access token from the saved refresh token (the silent sign-in on the next start) */
    String refreshMicrosoft() throws Exception {
        Http.Response r = Http.post(MICROSOFT + "/token", "application/x-www-form-urlencoded",
                                    Http.form("grant_type", "refresh_token", "client_id", CLIENT_ID, "refresh_token", refreshToken,
                                              "scope", SCOPE));
        if (r.status == 400 || r.status == 401) {
            throw new LoginException("Вход устарел, войди в аккаунт Microsoft ещё раз.");
        }
        if (!r.ok()) {
            throw new java.io.IOException("HTTP " + r.status);
        }
        JSONObject o = new JSONObject(r.body);
        refreshToken = o.optString("refresh_token", refreshToken);
        return o.getString("access_token");
    }

    /* ------------------------------------------------------------ Xbox Live and Minecraft */

    /** step 3: Xbox Live, XSTS and Minecraft services; fills the profile */
    void finish(String microsoftToken) throws Exception {
        JSONObject xblBody = new JSONObject()
            .put("Properties", new JSONObject().put("AuthMethod", "RPS").put("SiteName", "user.auth.xboxlive.com").put("RpsTicket", "d=" + microsoftToken))
            .put("RelyingParty", "http://auth.xboxlive.com")
            .put("TokenType", "JWT");
        Http.Response xbl = Http.post(XBOX_USER, "application/json", xblBody.toString(), "x-xbl-contract-version", "1");
        if (!xbl.ok()) {
            throw new LoginException("Xbox Live не принял вход (HTTP " + xbl.status + ")");
        }
        JSONObject xblJson = new JSONObject(xbl.body);
        String xblToken = xblJson.getString("Token");

        JSONObject xstsBody = new JSONObject()
            .put("Properties", new JSONObject().put("SandboxId", "RETAIL").put("UserTokens", new JSONArray().put(xblToken)))
            .put("RelyingParty", "rp://api.minecraftservices.com/")
            .put("TokenType", "JWT");
        Http.Response xsts = Http.post(XSTS, "application/json", xstsBody.toString(), "x-xbl-contract-version", "1");
        if (xsts.status == 401) {
            throw new LoginException(xstsError(new JSONObject(xsts.body.isEmpty() ? "{}" : xsts.body).optLong("XErr")));
        }
        if (!xsts.ok()) {
            throw new LoginException("Xbox Live не пустил в Minecraft (HTTP " + xsts.status + ")");
        }
        JSONObject xstsJson = new JSONObject(xsts.body);
        String uhs = xstsJson.getJSONObject("DisplayClaims").getJSONArray("xui").getJSONObject(0).getString("uhs");

        Http.Response mc = Http.post(MINECRAFT + "/authentication/login_with_xbox", "application/json",
                                     new JSONObject().put("identityToken", "XBL3.0 x=" + uhs + ";" + xstsJson.getString("Token")).toString());
        if (!mc.ok()) {
            throw new LoginException("Сервер Minecraft не принял вход (HTTP " + mc.status + ")");
        }
        JSONObject mcJson = new JSONObject(mc.body);
        minecraftToken = mcJson.getString("access_token");
        minecraftTokenExpires = System.currentTimeMillis() + mcJson.optLong("expires_in", 86400) * 1000L;
        loadProfile();
    }

    /** the Minecraft: Java Edition profile; an account without the game has none */
    void loadProfile() throws Exception {
        Http.Response r = Http.get(MINECRAFT + "/minecraft/profile", "Authorization", "Bearer " + minecraftToken);
        if (r.status == 404) {
            ownsJava = false;
            name = "";
            uuid = "";
            skinUrl = "";
            return;
        }
        if (!r.ok()) {
            throw new java.io.IOException("профиль Minecraft: HTTP " + r.status);
        }
        JSONObject o = new JSONObject(r.body);
        ownsJava = true;
        name = o.optString("name");
        uuid = o.optString("id");
        skinUrl = "";
        JSONArray skins = o.optJSONArray("skins");
        for (int i = 0; skins != null && i < skins.length(); i++) {
            JSONObject skin = skins.getJSONObject(i);
            if (skinUrl.isEmpty() || "ACTIVE".equals(skin.optString("state"))) {
                skinUrl = skin.optString("url");
            }
        }
    }

    /** the start of the app: renews everything from the saved refresh token */
    void refresh() throws Exception {
        finish(refreshMicrosoft());
    }

    static String xstsError(long code) {
        if (code == 2148916233L) {
            return "У этого аккаунта Microsoft нет профиля Xbox. Открой xbox.com, войди и создай профиль, потом попробуй снова.";
        }
        if (code == 2148916235L) {
            return "Xbox Live недоступен в твоей стране.";
        }
        if (code == 2148916236L || code == 2148916237L) {
            return "Аккаунту нужно подтвердить возраст на xbox.com.";
        }
        if (code == 2148916238L) {
            return "Это детский аккаунт: взрослый должен добавить его в семью Microsoft (account.microsoft.com/family).";
        }
        return "Xbox Live не пустил в Minecraft (код " + code + ").";
    }
}
