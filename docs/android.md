# Prerequisites

Install these before starting:

1. **Android Studio** — https://developer.android.com/studio
2. **Android NDK r26+** — install via Android Studio SDK Manager
3. **Android SDK Platform 32+** — via SDK Manager
4. **Meta Quest ADB drivers** — enable USB debugging on headset
5. **Meta Quest OpenXR SDK** — https://developer.oculus.com/downloads/

You also need a **Meta Developer account** to sideload:
- https://developer.oculus.com/
- Enable Developer Mode on your Quest headset

---

# Build and sideload

## Build the APK

From Android Studio, or command line:

```bash
cd android
./gradlew assembleDebug
```

The APK will be at:
```
android/app/build/outputs/apk/debug/app-debug.apk
```

## Install via ADB

```bash
adb install app-debug.apk
```

Or using Meta's `adb`:

```bash
# Enable developer mode on Quest first
adb devices
adb install -r app-debug.apk
adb shell am start -n com.chiselengine.app/android.app.NativeActivity
```

## Install via Meta Quest Developer Hub (MQDH)

Alternatively, use the Meta Quest Developer Hub app on your PC for easier sideloading.

---

# Signing for distribution

For sideloading, debug signing works. For distribution beyond your own device, you need:

```bash
keytool -genkey -v -keystore chisel.keystore -alias chisel -keyalg RSA -keysize 2048 -validity 10000
```

Then configure in `build.gradle`:

```groovy
android {
    signingConfigs {
        release {
            storeFile file('chisel.keystore')
            storePassword 'your-password'
            keyAlias 'chisel'
            keyPassword 'your-password'
        }
    }
    buildTypes {
        release {
            signingConfig signingConfigs.release
        }
    }
}
```

---

# Realistic effort estimate

| Task | Effort |
|---|---|
| Android project scaffolding | 1–2 days |
| Replace GLFW with NativeActivity + EGL | 2–3 days |
| Adapt shaders to GLSL ES | 2–3 days |
| Adapt XRManager for Meta OpenXR | 2–3 days |
| Adapt file loading for Android assets | 1–2 days |
| Adapt RenderSystem (GL ES differences) | 3–5 days |
| Adapt audio (miniaudio already supports Android) | 0.5 day |
| Testing on Quest hardware | Ongoing |

**Total: roughly 2–3 weeks of focused work** for a basic working port.

---

# Recommended approach

Given the scope, I'd suggest doing this incrementally:

1. **Phase 1**: Get a blank NativeActivity rendering a colored triangle on Quest via OpenGL ES. Verify ADB sideloading works.

2. **Phase 2**: Get OpenXR session creation working on Quest. Render a stereo colored background.

3. **Phase 3**: Port your `RenderSystem` shaders to GLSL ES. Render a simple mesh.

4. **Phase 4**: Port `GLBLoader`, `Scene`, and `Node` hierarchy.

5. **Phase 5**: Port `LuaRuntime` and `game.lua`.

6. **Phase 6**: Port `PhysicsSystem` (Jolt already supports ARM64).

7. **Phase 7**: Polish — input, audio, performance tuning for Quest's mobile GPU.

Would you like me to start with Phase 1 and create a minimal Android NativeActivity + EGL + OpenGL ES skeleton that you can build and sideload immediately?