package com.xxy.vulkanversion;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.appbar.MaterialToolbar;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.textview.MaterialTextView;

/**
 * 主界面：提供 Vulkan 1.0 / 1.1 / 1.3 / 1.4 四个渲染按钮。
 * 渲染由 Native (C++/NDK + Vulkan) 完成，渲染结果绘制到 SurfaceView。
 */
public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    // 与 native 层约定的 Vulkan 版本代号
    public static final int VK_API_1_0 = 0;
    public static final int VK_API_1_1 = 1;
    public static final int VK_API_1_3 = 2;
    public static final int VK_API_1_4 = 3;

    static {
        System.loadLibrary("vulkanversion");
    }

    private SurfaceView surfaceView;
    private MaterialTextView infoText;
    private MaterialButton btn10;
    private MaterialButton btn11;
    private MaterialButton btn13;
    private MaterialButton btn14;
    private MaterialButton btnRefresh;
    private MaterialToolbar toolbar;

    // 当前正在渲染的版本，-1 表示尚未开始
    private int currentVersion = -1;

    // 用于在 native 线程回调 UI 线程
    private final android.os.Handler mainHandler = new android.os.Handler(
            android.os.Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView = findViewById(R.id.surface_view);
        infoText = findViewById(R.id.info_text);
        btn10 = findViewById(R.id.btn_vulkan_10);
        btn11 = findViewById(R.id.btn_vulkan_11);
        btn13 = findViewById(R.id.btn_vulkan_13);
        btn14 = findViewById(R.id.btn_vulkan_14);
        btnRefresh = findViewById(R.id.btn_refresh);
        toolbar = findViewById(R.id.toolbar);

        // 使用 MaterialToolbar 作为 ActionBar
        setSupportActionBar(toolbar);

        surfaceView.getHolder().addCallback(this);

        btn10.setOnClickListener(v -> startRendering(VK_API_1_0, "Vulkan 1.0"));
        btn11.setOnClickListener(v -> startRendering(VK_API_1_1, "Vulkan 1.1"));
        btn13.setOnClickListener(v -> startRendering(VK_API_1_3, "Vulkan 1.3"));
        btn14.setOnClickListener(v -> startRendering(VK_API_1_4, "Vulkan 1.4"));
        btnRefresh.setOnClickListener(v -> updateInfoText());

        updateInfoText();
    }

    private void updateInfoText() {
        String supported = nativeGetSupportedVulkanVersion();
        infoText.setText("设备 GPU 支持的最高 Vulkan 版本：\n" + supported);
    }

    /**
     * 开始使用指定版本渲染。
     */
    private void startRendering(int version, String label) {
        // 先查询设备支持的最高版本
        int maxSupported = nativeGetMaxSupportedVersion();

        if (version > maxSupported) {
            // GPU 不支持该版本：弹窗提示（不进入渲染）
            showUnsupportedDialog(label);
            return;
        }

        currentVersion = version;
        highlightButton(version);

        // 若 Surface 已就绪，立即开始；否则等待 surfaceCreated 回调
        if (surfaceView.getHolder().getSurface().isValid()) {
            nativeStartRender(surfaceView.getHolder().getSurface(), version);
        }
    }

    private void highlightButton(int version) {
        // 恢复所有按钮为可用态
        btn10.setChecked(false);
        btn11.setChecked(false);
        btn13.setChecked(false);
        btn14.setChecked(false);
        btn10.setEnabled(true);
        btn11.setEnabled(true);
        btn13.setEnabled(true);
        btn14.setEnabled(true);

        MaterialButton active;
        switch (version) {
            case VK_API_1_0:
                active = btn10;
                break;
            case VK_API_1_1:
                active = btn11;
                break;
            case VK_API_1_3:
                active = btn13;
                break;
            case VK_API_1_4:
                active = btn14;
                break;
            default:
                return;
        }
        // 高亮当前渲染版本
        active.setChecked(true);
    }

    private void showUnsupportedDialog(String label) {
        String supported = nativeGetSupportedVulkanVersion();
        new AlertDialog.Builder(this)
                .setTitle("GPU 不支持 " + label)
                .setMessage("当前设备 GPU 不支持 " + label + "。\n\n设备支持的最高版本为：\n"
                        + supported + "\n\n请选择更低版本进行渲染。")
                .setPositiveButton("知道了", null)
                .show();
    }

    // ---------------- Native 回调（由 C++ 渲染线程调用） ----------------

    /**
     * 渲染线程回调：报告版本检测结果 / 错误信息，主线程弹出 Toast。
     */
    private void onNativeMessage(final String message) {
        mainHandler.post(() -> Toast.makeText(MainActivity.this, message,
                    Toast.LENGTH_SHORT).show());
    }

    /**
     * 渲染线程回调：报告渲染中发生不支持/不可用的情况，弹窗提示。
     */
    private void onNativeUnsupported(final String message) {
        mainHandler.post(() -> new AlertDialog.Builder(MainActivity.this)
                .setTitle("GPU 不支持该版本")
                .setMessage(message)
                .setPositiveButton("知道了", null)
                .show());
    }

    // ---------------- SurfaceHolder.Callback ----------------

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        if (currentVersion >= 0) {
            nativeStartRender(holder.getSurface(), currentVersion);
        }
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        nativeResize(width, height);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        nativeStopRender();
    }

    @Override
    protected void onPause() {
        super.onPause();
        nativePause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (currentVersion >= 0 && surfaceView.getHolder().getSurface().isValid()) {
            nativeStartRender(surfaceView.getHolder().getSurface(), currentVersion);
        }
    }

    // ---------------- Native 方法声明 ----------------

    private native void nativeStartRender(android.view.Surface surface, int version);
    private native void nativeStopRender();
    private native void nativeResize(int width, int height);
    private native void nativePause();
    private native int nativeGetMaxSupportedVersion();
    private native String nativeGetSupportedVulkanVersion();
}
