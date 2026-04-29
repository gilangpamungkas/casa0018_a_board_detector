package com.pavesafe

import android.Manifest
import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.WindowManager
import android.webkit.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import java.net.HttpURLConnection
import java.net.URL
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity() {

    private lateinit var webView: WebView
    private val PERMISSION_REQUEST = 100

    // ── Arduino IP on phone hotspot ───────────────────────────────────────────
    // Check Serial Monitor after Arduino connects to "Gilang" hotspot
    private val ARDUINO_IP = "10.143.211.222"
    private var lastSignal = ""

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Full screen
        WindowCompat.setDecorFitsSystemWindows(window, false)
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        webView = WebView(this)
        setContentView(webView)

        webView.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true
            mediaPlaybackRequiresUserGesture = false
            allowFileAccess = true
            allowContentAccess = true
            allowUniversalAccessFromFileURLs = true
            mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
            setRenderPriority(WebSettings.RenderPriority.HIGH)
            cacheMode = WebSettings.LOAD_DEFAULT
        }

        // JavaScript bridge — app calls window.Arduino.send('C')
        // Native Kotlin makes the HTTP request — bypasses WebView restrictions
        webView.addJavascriptInterface(ArduinoBridge(), "Arduino")

        webView.webChromeClient = object : WebChromeClient() {
            override fun onPermissionRequest(request: PermissionRequest) {
                runOnUiThread { request.grant(request.resources) }
            }
            override fun onConsoleMessage(message: ConsoleMessage): Boolean {
                android.util.Log.d("PaveSafe_JS",
                    "${message.message()} (${message.sourceId()}:${message.lineNumber()})")
                return true
            }
        }

        webView.webViewClient = WebViewClient()
        checkPermissions()
    }

    // ── Native Arduino HTTP bridge ─────────────────────────────────────────────
    inner class ArduinoBridge {
        @JavascriptInterface
        fun send(signal: String) {
            if (signal == lastSignal) return
            lastSignal = signal
            thread {
                try {
                    val url = URL("http://$ARDUINO_IP/$signal")
                    val conn = url.openConnection() as HttpURLConnection
                    conn.requestMethod = "GET"
                    conn.connectTimeout = 2000
                    conn.readTimeout   = 2000
                    conn.connect()
                    val code = conn.responseCode
                    android.util.Log.d("PaveSafe", "Arduino $signal → HTTP $code")
                    conn.disconnect()
                } catch (e: Exception) {
                    android.util.Log.e("PaveSafe", "Arduino error: ${e.message}")
                    lastSignal = "" // allow retry next signal
                }
            }
        }
    }

    private fun checkPermissions() {
        val permissions = mutableListOf<String>()
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            != PackageManager.PERMISSION_GRANTED) {
            permissions.add(Manifest.permission.CAMERA)
        }
        if (permissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                this, permissions.toTypedArray(), PERMISSION_REQUEST)
        } else {
            loadApp()
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        loadApp()
    }

    private fun loadApp() {
        webView.loadUrl("file:///android_asset/pavesafe.html")
    }

    override fun onBackPressed() {
        if (webView.canGoBack()) webView.goBack() else super.onBackPressed()
    }

    override fun onResume()  { super.onResume();  webView.onResume() }
    override fun onPause()   { super.onPause();   webView.onPause()  }
    override fun onDestroy() { webView.destroy(); super.onDestroy()  }
}