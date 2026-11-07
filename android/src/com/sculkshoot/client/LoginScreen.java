package com.sculkshoot.client;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/**
 * The door to the launcher: the menu opens only after signing in with a Microsoft account. The player opens
 * microsoft.com/link (in any browser, on the phone or a computer) and types the code shown here.
 */
final class LoginScreen {
    private final MainActivity activity;
    private final FrameLayout root;
    private final TextView message;
    private final TextView code;
    private final ImageView logo;
    private final ImageView qr;
    private final LinearLayout codeRow;
    private final Button signIn;
    private final Button openPage;
    private final Button copy;
    private Account.DeviceCode device;

    LoginScreen(MainActivity activity) {
        this.activity = activity;
        root = new FrameLayout(activity);
        root.setBackground(new MoonSkyDrawable(activity.getResources().getDisplayMetrics().density));

        LinearLayout card = new LinearLayout(activity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_HORIZONTAL);
        card.setPadding(dp(28), dp(20), dp(28), dp(20));
        card.setBackground(Ui.rounded(activity, 0xE00D0E16, 18, Ui.LINE));

        logo = new ImageView(activity);
        logo.setImageResource(R.drawable.install_default);
        card.addView(logo, new LinearLayout.LayoutParams(dp(64), dp(64)));
        TextView title = Ui.label(activity, "Lullaby Launcher", 22, Ui.ACCENT, true);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, dp(8), 0, dp(4));
        card.addView(title);
        message = Ui.label(activity, "Чтобы открыть меню, войди в аккаунт Microsoft, на котором куплена Minecraft: Java Edition.",
                           14, Ui.TEXT, false);
        message.setGravity(Gravity.CENTER);
        card.addView(message, new LinearLayout.LayoutParams(dp(420), ViewGroup.LayoutParams.WRAP_CONTENT));
        code = Ui.label(activity, "", 34, Ui.ACCENT, true);
        code.setGravity(Gravity.CENTER);
        code.setLetterSpacing(0.15f);
        code.setPadding(0, dp(10), 0, dp(4));
        // like Prism Launcher: a QR code for another phone's camera next to the code itself
        codeRow = new LinearLayout(activity);
        codeRow.setGravity(Gravity.CENTER_VERTICAL);
        codeRow.setVisibility(View.GONE);
        qr = new ImageView(activity);
        qr.setBackground(Ui.rounded(activity, 0xFFFFFFFF, 10, 0xFFFFFFFF));
        qr.setPadding(dp(6), dp(6), dp(6), dp(6));
        codeRow.addView(qr, new LinearLayout.LayoutParams(dp(150), dp(150)));
        LinearLayout codeColumn = new LinearLayout(activity);
        codeColumn.setOrientation(LinearLayout.VERTICAL);
        codeColumn.setGravity(Gravity.CENTER_HORIZONTAL);
        codeColumn.setPadding(dp(18), 0, 0, 0);
        TextView codeTitle = Ui.label(activity, "Код для входа", 13, Ui.HINT, false);
        codeTitle.setGravity(Gravity.CENTER);
        codeColumn.addView(codeTitle);
        codeColumn.addView(code);
        TextView scan = Ui.label(activity, "Наведи камеру другого телефона на QR-код —\nкод подставится сам", 12, Ui.HINT, false);
        scan.setGravity(Gravity.CENTER);
        codeColumn.addView(scan);
        codeRow.addView(codeColumn);
        LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        rowParams.topMargin = dp(10);
        card.addView(codeRow, rowParams);

        LinearLayout buttons = new LinearLayout(activity);
        buttons.setGravity(Gravity.CENTER);
        signIn = Ui.button(activity, "Войти через Microsoft", true);
        signIn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                start();
            }
        });
        openPage = Ui.button(activity, "Открыть microsoft.com/link", true);
        openPage.setVisibility(View.GONE);
        openPage.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                openPage();
            }
        });
        copy = Ui.button(activity, "Скопировать код", false);
        copy.setVisibility(View.GONE);
        copy.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                copyCode();
            }
        });
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        p.setMargins(dp(5), 0, dp(5), 0);
        buttons.addView(signIn, p);
        buttons.addView(openPage, new LinearLayout.LayoutParams(p));
        buttons.addView(copy, new LinearLayout.LayoutParams(p));
        LinearLayout.LayoutParams row = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        row.topMargin = dp(14);
        row.bottomMargin = dp(4);
        card.addView(buttons, row);

        FrameLayout.LayoutParams center = new FrameLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT,
                                                                       Gravity.CENTER);
        // small phones in landscape are low: the card scrolls instead of being cut off
        android.widget.ScrollView scroll = new android.widget.ScrollView(activity);
        scroll.setFillViewport(true);
        FrameLayout holder = new FrameLayout(activity);
        holder.setPadding(dp(8), dp(8), dp(8), dp(8));
        holder.addView(card, center);
        scroll.addView(holder, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        root.addView(scroll, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    View view() {
        return root;
    }

    /** shown instead of the code when the saved sign-in no longer works */
    void setMessage(String text) {
        message.setText(text);
    }

    private int dp(float v) {
        return Ui.dp(activity, v);
    }

    /* The wait for the code runs on its own thread, not tied to this screen: Android may recreate the window
       while the player is in the browser, and the result must still reach whatever window is open then. */
    private static int attemptCounter;
    private static volatile int currentAttempt;
    private static volatile Thread poller;

    private void start() {
        signIn.setEnabled(false);
        message.setText("Получаю код у Microsoft...");
        activity.background(new MainActivity.Job<Account.DeviceCode>() {
            public Account.DeviceCode run() throws Exception {
                return Account.startDeviceCode();
            }

            public void done(Account.DeviceCode result, Exception error) {
                signIn.setEnabled(true);
                if (error != null) {
                    message.setText(problem(error));
                    return;
                }
                Account.savePending(activity, result);
                showCode(result);
                startPolling(activity.getApplicationContext(), result, true);
            }
        });
    }

    /** when Lullaby opens again with a code still waiting: show it and keep waiting */
    void resumePending() {
        Account.DeviceCode pending = Account.loadPending(activity);
        if (pending != null) {
            showCode(pending);
            startPolling(activity.getApplicationContext(), pending, false);
        }
    }

    private void showCode(Account.DeviceCode result) {
        device = result;
        code.setText(result.userCode);
        code.setVisibility(View.VISIBLE);
        qr.setImageBitmap(qrBitmap(linkWithCode(result)));
        codeRow.setVisibility(View.VISIBLE);
        logo.setVisibility(View.GONE);
        openPage.setVisibility(View.VISIBLE);
        copy.setVisibility(View.VISIBLE);
        signIn.setText("Новый код");
        signIn.setVisibility(View.VISIBLE);
        message.setText("Отсканируй QR-код телефоном или открой " + result.verificationUri.replace("https://", "")
                        + " (можно на компьютере) и введи код. Я подожду и сам открою меню.");
    }

    /** shows the sign-in again after an error */
    void failed(String why) {
        message.setText(why);
        codeRow.setVisibility(View.GONE);
        logo.setVisibility(View.VISIBLE);
        openPage.setVisibility(View.GONE);
        copy.setVisibility(View.GONE);
        signIn.setText("Войти через Microsoft");
    }

    /**
     * Asks Microsoft every few seconds whether the code was entered, then finishes the sign-in and saves the account.
     * A new code replaces the old wait; reopening the app with the same code keeps the running one.
     */
    private static synchronized void startPolling(final android.content.Context app, final Account.DeviceCode code, boolean fresh) {
        if (!fresh && poller != null && poller.isAlive()) {
            return;
        }
        final int mine = ++attemptCounter;
        currentAttempt = mine;
        final android.os.Handler main = new android.os.Handler(android.os.Looper.getMainLooper());
        poller = new Thread(new Runnable() {
            public void run() {
                String problem = null;
                try {
                    Account account = new Account();
                    String token = null;
                    while (token == null) {
                        if (mine != currentAttempt) {
                            return;  // a newer code replaced this one
                        }
                        Thread.sleep(code.interval * 1000L);
                        token = account.pollDeviceCode(code);
                    }
                    if (mine != currentAttempt) {
                        return;
                    }
                    account.finish(token);
                    account.save(app);
                } catch (InterruptedException e) {
                    return;
                } catch (Exception e) {
                    problem = problem(e);
                }
                Account.clearPending(app);
                final String finalProblem = problem;
                main.post(new Runnable() {
                    public void run() {
                        MainActivity shown = MainActivity.resumed();
                        if (shown != null) {
                            shown.signInFinished(finalProblem);
                        } else if (finalProblem != null) {
                            lastProblem = finalProblem;  // shown when the app is opened again
                        }
                    }
                });
            }
        }, "MicrosoftSignIn");
        poller.start();
    }

    /** an error that happened while no window was open */
    static String lastProblem;

    boolean isWaiting() {
        return poller != null && poller.isAlive();
    }

    private void openPage() {
        if (device == null) {
            return;
        }
        copyCode();
        try {
            activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(linkWithCode(device))));
        } catch (android.content.ActivityNotFoundException e) {
            Toast.makeText(activity, "Нет браузера. Открой " + device.verificationUri + " на другом устройстве.", Toast.LENGTH_LONG).show();
        }
    }

    /** microsoft.com/link takes the code in the link (?otc=), so the page opens with it filled in, as in Prism Launcher */
    static String linkWithCode(Account.DeviceCode device) {
        String url = device.verificationUri;
        if (url.startsWith("https://www.microsoft.com/link") && !url.contains("?")) {
            url += "?otc=" + Uri.encode(device.userCode);
        }
        return url;
    }

    /** the QR code as a picture: dark modules on white, sharp pixels */
    static android.graphics.Bitmap qrBitmap(String text) {
        QrCode code = QrCode.encode(text);
        int border = 2;
        int n = code.size + border * 2;
        android.graphics.Bitmap small = android.graphics.Bitmap.createBitmap(n, n, android.graphics.Bitmap.Config.ARGB_8888);
        small.eraseColor(0xFFFFFFFF);
        for (int y = 0; y < code.size; y++) {
            for (int x = 0; x < code.size; x++) {
                if (code.dark(x, y)) {
                    small.setPixel(x + border, y + border, 0xFF000000);
                }
            }
        }
        int scale = Math.max(1, 360 / n);
        return android.graphics.Bitmap.createScaledBitmap(small, n * scale, n * scale, false);
    }

    private void copyCode() {
        if (device == null) {
            return;
        }
        ClipboardManager clipboard = (ClipboardManager) activity.getSystemService(Context.CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(ClipData.newPlainText("код Microsoft", device.userCode));
        Toast.makeText(activity, "Код " + device.userCode + " скопирован", Toast.LENGTH_SHORT).show();
    }

    static String problem(Exception error) {
        return error instanceof Account.LoginException ? error.getMessage() : "Нет связи с Microsoft (" + error.getMessage() + "). Проверь интернет.";
    }
}
