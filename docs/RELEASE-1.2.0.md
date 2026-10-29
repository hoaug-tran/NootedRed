# NootedRed v1.2.0

We are proud to present **NootedRed v1.2.0**, a monumental milestone release delivering **dynamic multi-version pattern resolution** for future-proof macOS compatibility, **complete hardware video decode & encode capabilities** across all AMD Vega/GCN APUs, and crucial fixes for **hardware-accelerated screen capture and remote desktop rendering artifacts**.

---

### What's New & Resolved in v1.2.0

#### 1. Dynamic Multi-Version Pattern Resolution (Autonomous macOS Build Compatibility)
- **Elimination of Static Page Tables**: Replaced rigid, fragile 4 KiB static binary page tables and hardcoded dyld cache offsets for `AppleGVA` and `AMDRadeonVADriver2` with a robust, multi-layer dynamic signature resolver.
- **Future-Proof Build Independence**: NootedRed now dynamically locates and adapts target kernel/driver interfaces at runtime. Future minor macOS incremental builds and security updates will be adapted automatically without requiring manual offset extraction, driver harvesting, or immediate kext updates.
- **Iterative Multi-Site Resolver**: Upgraded the signature scanning engine to iteratively match and patch multiple duplicate function sites across driver pages (e.g., status strides, video processor dispatchers, and Blt engines).
- **Process Exclusion Defense**: Hardened the dynamic shared-cache scanner to strictly bypass `WindowServer` and `loginwindow`, eliminating unnecessary page touches and preserving system compositor integrity.

#### 2. Full Hardware Video Codec Restoration (Vega 8 / GCN APUs)
- **All 5 Core Hardware Codecs Fully Operational**: Restored and validated end-to-end hardware acceleration across all supported Vega/GCN APUs (Raven Ridge, Picasso, Renoir, Lucienne, Cezanne, Barcelo):
  1. **AVC / H.264 Hardware Decode**
  2. **AVC / H.264 Hardware Encode**
  3. **8-bit HEVC (Main) Hardware Decode**
  4. **8-bit HEVC (Main) Hardware Encode**
  5. **10-bit HEVC (Profile 2 / Main 10) Hardware Decode**
- **Enabled Out-of-the-Box**: Both `GLFix` (Chromium/Electron rendering) and `VDA` (VideoToolbox hardware video acceleration) are now **enabled by default**, providing a seamless out-of-the-box experience without requiring manual boot arguments.
- **Ventura APU Expansion**: Removed the legacy `0x1638` Cezanne restriction on macOS Ventura (13.x), extending VCN 2.x acceleration to all Vega APUs running Darwin 22.

#### 3. Resolution of Screen Recording & Remote Desktop Artifacts
- **Encoder Pitch & Stride Realignment**: Calibrated hardware encode status strides (`kVDACtx_StatusStride_Modern`, `0x30` vs `0xA4`), encode feedback strides, and source surface pitch calculations in `VDASignatures.inc`.
- **Elimination of Red Stripes & Tearing**: Completely resolved the notorious horizontal red banding, compression artifacts, and frame tearing experienced when using NetEase UU Remote (网易UU远程), macOS native screen capture (`Cmd+Shift+5`), QuickTime Player recording, and OBS Studio.
- **Streamlined Teardown Synchronization**: Eliminated synchronous blocking teardown drain loops (`drainVCN`), resolving channel deadlocks, client stop timeouts, and GPU watchdog panics when switching video tracks or terminating recording sessions.

---

### Boot Arguments & Configuration Flags

> [!IMPORTANT]
> **Out-of-the-Box Operation:**
> Starting with v1.2.0, **GLFix, VDA hardware video acceleration (AVC + HEVC), SubmitGuard, and LegacyBlend are all ENABLED by default**. Most users do not need to specify any boot arguments!

If you need to diagnose or disable specific features, use the following opt-out boot arguments in your OpenCore `config.plist`:

| Boot Argument | Function | Default | Description / Diagnostic Use |
| :--- | :--- | :--- | :--- |
| **`-NRedNoVDA`** | Completely disable all hardware video acceleration | Disabled | Diagnostic switch. Reverts video playback and encoding to CPU software paths. |
| **`-NRedNoHEVC`** | Restrict hardware acceleration to AVC/H.264 only | Disabled | Disables HEVC decode and encode while retaining AVC/H.264 acceleration. Use if HEVC video causes instability. |
| **`-NRedNoGLFix`** | Disable user-space OpenGL/Metal dual-source patches | Disabled | Diagnostic switch. Turns off Chromium/Electron/CEF application workarounds. |
| **`-NRedNoSubmitGuard`**| Disable Metal command queue tail-filler protection | Disabled | Diagnostic switch for Metal command queue troubleshooting. |
| **`-NRedNoLegacyBlend`** | Disable Metal DualQuad LegacyBlend fix | Disabled | Diagnostic switch for UI dropdown blend workarounds. |
| **`-NRedDebug`** | Enable verbose kernel logging (`dmesg` / log stream) | Disabled | Useful for reporting issues and submitting compatibility logs. |
| **`-NRedOff`** | Completely disable all NootedRed drivers and hooks | Disabled | Recovery / vanilla baseline boot. |

---

### Important Notes & Prerequisites

1. **AMFI & Code Signing Requirements (macOS 13+)**:
   - Because NootedRed dynamically resolves and patches user-space AMD graphics/media driver pages in memory, macOS AMFI (Apple Mobile File Integrity) enforces code signature checks on system processes.
   - **`AMFIPass.kext` is strongly recommended** in your OpenCore EFI (along with `-amfipassbeta` in `boot-args` on newer macOS versions like Sonoma 14.8+ / Sequoia 15.x) to avoid `WindowServer` code signature invalidation.
   - Alternatively, add `amfi=0x80` to `boot-args`.
2. **Prerequisites & Incompatibilities**:
   - **Lilu.kext** (>= 1.7.0) is strictly required and must load before `NootedRed.kext`.
   - **DO NOT** use `WhateverGreen.kext` or `BFixup.kext` alongside `NootedRed.kext` (they will conflict).
3. **Supported Hardware**:
   - AMD APUs with **Vega/GCN architecture** (Raven Ridge, Picasso, Renoir, Lucienne, Cezanne, Barcelo — Ryzen 2000 through 5000 / 7030 series).
   - RDNA-based APUs (6000 / 7040 / 8000 series with Radeon 680M/780M) are **NOT supported**.
4. **DRM & Streaming Services**:
   - FairPlay 1.x hardware DRM (e.g. Netflix in Safari, Apple TV+) has known architectural constraints on AMD Hackintoshes. For streaming media, use Chromium-based browsers (Chrome, Edge, Firefox) which leverage Widevine software DRM.

---

### Package Hashes (SHA-256)
- **`NootedRed-1.2.0-RELEASE.zip`**: `105d33cf37dbd003a3ff1628a2c6f75f29905baa66829c478113281fcb546bee`
