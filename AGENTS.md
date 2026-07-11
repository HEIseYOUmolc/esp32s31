# Repository Guidelines

## Project Structure & Module Organization

This is an ESP-IDF project (`esp32_s31_chat`) targeting **ESP32-S3 (ESP-BOX-3)** hardware. The project implements a voice chat application ("小智" / Xiaozhi smart assistant) with LVGL display, audio codec, and network provisioning.

```
esp32s31/
├── CMakeLists.txt                   # Project root — cmake_minimum_required(3.16)
├── main/
│   ├── CMakeLists.txt               # Component registration with WHOLE_ARCHIVE
│   ├── idf_component.yml            # Dependencies: esp_xiaozhi, av_processor, fonts
│   ├── Kconfig.projbuild            # Transport selection (MQTT/WebSocket/Auto)
│   ├── main.c                       # app_main() — NVS init, net connect, chat start
│   ├── esp_xiaozhi_chat_app.c/.h    # Chat application core
│   └── esp_xiaozhi_chat_display.c/.h# LVGL display & UI
├── components/
│   ├── esp_board_manager_adapter/   # Board support via esp_board_manager
│   └── gen_bmgr_codes/              # Auto-generated board config code
├── managed_components/              # Auto-downloaded dependencies (do not edit)
├── partitions.csv                   # Custom partition table (6MB OTA slots)
├── sdkconfig.defaults               # Kconfig defaults
└── dependencies.lock                # Component manager lockfile
```

## Build, Test, and Development Commands

```bash
# Activate IDF environment (v6.0.1)
C:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe -m pip install -q idf-component-manager
& "C:\Program Files\Espressif\idf_cmd_init\idf_cmd_init.bat"

# Build the project
python -m idf_py build

# Full clean and rebuild
python -m idf_py fullclean && python -m idf_py build

# Flash and monitor (with target ESP-BOX-3)
python -m idf_py -p COM? flash monitor

# Open menuconfig
python -m idf_py menuconfig

# Update dependencies
python -m idf_py update-dependencies
```

## Coding Style & Naming Conventions

This project follows the **ESP-IDF project coding standards** defined in the project-level AGENTS.md conventions. Key rules:

- **Functions**: `snake_case` — return `esp_err_t` for all fallible operations.
- **Variables**: `snake_case`; static variables prefixed with `s_`.
- **Macros/Constants**: `UPPER_SNAKE_CASE`.
- **TAG**: Every `.c` file must declare `static const char *TAG = "module_name";`.
- **Headers**: Order — C standard lib → FreeRTOS → ESP-IDF core → sdkconfig → project headers.
- **Struct init**: Use designated initializers (`.field = value`), never line-by-line assignment.
- **Error handling**: `ESP_ERROR_CHECK()` for unrecoverable init failures; `ESP_RETURN_ON_ERROR()` / `ESP_LOGE()` for recoverable paths.
- **Logging**: Use `ESP_LOGI` / `ESP_LOGE` / `ESP_LOGW` — never `printf()` outside trivial blink demos.
- **SPDX header**: Every source file must start with the Apache-2.0 copyright header.

## Testing Guidelines

- This project is an embedded firmware application with no automated test suite.
- Verification is done through **build validation** (`python -m idf_py build`) and **on-target testing** via `flash monitor`.
- When adding features, always verify the build compiles cleanly before claiming completion.
- Use `dependencies.lock` to pin component versions — run `python -m idf_py update-dependencies` after adding new `idf_component.yml` entries.

## Commit & Pull Request Guidelines

- **Commit messages**: Use the conventional format — `type(scope): short description` (e.g., `fix(display): correct LVGL touch calibration`, `feat(audio): add wake-word volume control`).
- Keep the subject line under 72 characters; use the body for rationale and any breaking changes.
- **Pull requests**: Link to the related issue if applicable. Describe the change, testing performed, and any hardware-specific notes (e.g., "tested on ESP-BOX-3 with ESP-IDF v6.0.1").
- Do **not** commit `build/`, `managed_components/`, or `sdkconfig.old` — the `.gitignore` should already exclude these.

## Security & Configuration Tips

- **Partition table**: Custom layout in `partitions.csv` — two 6MB OTA slots. Modify with care.
- **WiFi memory**: `sdkconfig.defaults` reduces WiFi buffer counts and disables enterprise support to save RAM.
- **UART ISR**: `CONFIG_UART_ISR_IN_IRAM=y` prevents FIFO overflow on ML307 modules.
- **Board selection**: The project targets **ESP-BOX-3** (`esp32_s31_korvo_1` board). To switch boards, update `sdkconfig.defaults` and regenerate via `python -m idf_py reconfigure`.
- **managed_components/**: Never edit these files directly — they are managed by the component manager and will be overwritten on `update-dependencies`. Use `override_path` in `idf_component.yml` to patch.

## Agent-Specific Instructions

When modifying this repository, always check `main/idf_component.yml` for the current dependency tree. The project uses:

- `78/xiaozhi-fonts` ~1.3.2 (LVGL compressed CJK fonts)
- `espressif/esp_xiaozhi` ^0.1.1 (chat SDK)
- `espressif/network_provisioning` ^1.2.4
- `jason-mao/av_processor` ^0.7.0 (audio/video processing)
- `protocol_examples_common` from IDF examples (path reference)

If adding new components, declare them in `main/idf_component.yml` with semantic version ranges (`^major.minor.patch`).


## Changelog


### 2026-07-12 — LVGL RGB display crash fix

Fixed assertion `disp_cfg->io_handle != NULL` crash when enabling LVGL on `esp32_s31_korvo_1` (RGB LCD).

**Root cause**: `esp_board_manager_adapter.c` called `lvgl_port_add_disp()` which requires `io_handle != NULL`. RGB LCDs have no separate IO handle (`io_handle` is NULL), unlike SPI displays.

**Fix** (`components/esp_board_manager_adapter/esp_board_manager_adapter.c`): Detect RGB sub_type and use `lvgl_port_add_disp_rgb()` instead, which supports RGB panels without an IO handle.
### 2026-07-11 — av_processor v0.7.0 API migration

Migrated `esp_xiaozhi_chat_app.c` from `jason-mao/av_processor` v0.6.x to v0.7.0, which introduced an explicit handle-based audio instance model.

**Changes applied to `main/esp_xiaozhi_chat_app.c`:**

- **`audio_manager_config_t`**: Replaced removed fields `.play_dev`, `.rec_dev`, `.board_sample_rate`, `.board_bits`, `.board_channels` with `.play_io.write_cb` / `.rec_io.read_cb` callbacks bridging to `esp_codec_dev`. Added `audio_manager_config_set_play_io_format()` / `set_rec_io_format()` calls.
- **Bridge callbacks**: Added `board_audio_write_cb()` and `board_audio_read_cb()` to connect `audio_io_ops_t` with `esp_codec_dev_write()` / `esp_codec_dev_read()`.
- **Static handles**: Added `s_recorder`, `s_feeder`, `s_playback` (file-scope).
- **Open/close functions**: All `*_open()` calls now take output handle parameter; all `*_close()` calls now require handle input.
- **`audio_feeder_run()`** → `audio_feeder_start(handle)`.
- **`audio_playback_config_t`** → `audio_play_config_t`, `DEFAULT_AUDIO_PLAYBACK_CONFIG()` → `DEFAULT_AUDIO_PLAY_CONFIG()`.
- **Recorder event callback**: Added `audio_recorder_handle_t recorder` first parameter.
- **`config.play_dev` / `config.rec_dev`** references → `bsp_info.play_dev` / `bsp_info.rec_dev` with explicit cast.


- **`afe_config.ai_mode_wakeup = true`** → `afe_config.mode.afe.algo_mask |= AV_PROCESSOR_AFE_FLAG_AI_MODE_WAKEUP` (field removed, now set via algo_mask flag).
**Reference**: Official `audio_afe` example in `managed_components/jason-mao__av_processor/examples/audio_afe/` and `docs/COMPONENT_GUIDE.md`.
