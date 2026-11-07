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
    private final Button signIn;
    private final Button openPage;
    private final Button copy;
    private Account.DeviceCode device;
    private volatile boolean waiting;
    private int attempt;  // a new "Войти" stops the old wait

    LoginScreen(MainActivity activity) {
        this.activity = activity;
        root = new FrameLayout(activity);
        root.setBackground(new MoonSkyDrawable(activity.getResources().getDisplayMetrics().density));

        LinearLayout card = new LinearLayout(activity);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER_HORIZONTAL);
        card.setPadding(dp(28), dp(20), dp(28), dp(20));
        card.setBackground(Ui.rounded(activity, 0xE00D0E16, 18, Ui.LINE));

        ImageView logo = new ImageView(activity);
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
        code.setVisibility(View.GONE);
        card.addView(code);

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
        root.addView(card, center);
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

    private void start() {
        final int mine = ++attempt;
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
                device = result;
                code.setText(result.userCode);
                code.setVisibility(View.VISIBLE);
                openPage.setVisibility(View.VISIBLE);
                copy.setVisibility(View.VISIBLE);
                signIn.setText("Новый код");
                signIn.setVisibility(View.VISIBLE);
                message.setText("Открой " + result.verificationUri.replace("https://", "") + " (можно на компьютере) и введи этот код. "
                                + "Я подожду и сам открою меню.");
                waitFor(result, mine);
            }
        });
    }

    /** asks Microsoft every few seconds whether the code was entered, then finishes the sign-in */
    private void waitFor(final Account.DeviceCode code, final int mine) {
        waiting = true;
        activity.background(new MainActivity.Job<Account>() {
            public Account run() throws Exception {
                Account account = new Account();
                String token = null;
                while (token == null) {
                    if (mine != attempt) {
                        return null;  // a newer code replaced this one
                    }
                    Thread.sleep(code.interval * 1000L);
                    token = account.pollDeviceCode(code);
                }
                account.finish(token);
                return account;
            }

            public void done(Account account, Exception error) {
                if (mine != attempt) {
                    return;
                }
                waiting = false;
                if (error != null) {
                    message.setText(problem(error));
                    LoginScreen.this.code.setVisibility(View.GONE);
                    openPage.setVisibility(View.GONE);
                    copy.setVisibility(View.GONE);
                    signIn.setText("Войти через Microsoft");
                    return;
                }
                if (account != null) {
                    activity.signedIn(account);
                }
            }
        });
    }

    boolean isWaiting() {
        return waiting;
    }

    private void openPage() {
        if (device == null) {
            return;
        }
        copyCode();
        try {
            activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(device.verificationUri)));
        } catch (android.content.ActivityNotFoundException e) {
            Toast.makeText(activity, "Нет браузера. Открой " + device.verificationUri + " на другом устройстве.", Toast.LENGTH_LONG).show();
        }
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
