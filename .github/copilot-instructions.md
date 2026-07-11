# Copilot Instructions for `esp32s31`

## Build, test, and lint commands

- Use an ESP-IDF environment that supports `esp32s31` (preview target in current setup).
- First-time target setup:
  - `idf.py --preview set-target esp32s31`
- Reconfigure after dependency or Kconfig changes:
  - `idf.py --preview reconfigure`
- Build:
  - `idf.py --preview build`
- Flash + monitor:
  - `idf.py --preview -p <PORT> flash monitor`

Test commands:
- No repository-defined test suite or single-test target is currently committed (no `test/` app or test runner config found).

Lint commands:
- No repository-defined lint tooling/config is currently committed (no project lint command/config found).

## High-level architecture

- `main/main.c` is the app entry flow:
  1. `esp_board_manager_init()`
  2. `app_network_start_and_wait()`
  3. UI entry (`rem_page_show()` or `lvgl_test_page_show()` gated by `CONFIG_APP_REM_ENABLE`)
  4. `application_start()`

- `main/application.c` is the runtime orchestrator:
  - Creates `application_task` for audio/chat/MCP startup.
  - Creates `button_task` to poll BOOT button (`GPIO0`) and toggle audio test start/stop.
  - Registers chat protocol callbacks and ESP event handlers, then starts the chat session.

- Board and peripheral wiring is generated, not handwritten:
  - `components/gen_bmgr_codes/*.c` contains auto-generated board info, peripheral descriptors, and device descriptors.
  - `components/gen_bmgr_codes/CMakeLists.txt` includes board sources from `managed_components/espressif__esp_boards/esp32_s31_korvo_1` and sets `WHOLE_ARCHIVE`.

- Dependency ownership:
  - `main/idf_component.yml` declares app-level dependencies (`espressif/esp_board_manager`, `espressif/network_provisioning`).
  - `components/gen_bmgr_codes/idf_component.yml` declares generated board-specific dependencies (for example, GT1151 touch).

## Key conventions in this repository

- Treat `components/gen_bmgr_codes/*` as generated artifacts:
  - Files are marked `DO NOT MODIFY THIS FILE MANUALLY`.
  - Change board/peripheral config through esp_board_manager inputs and regenerate instead of patching generated code.

- Keep feature-flag behavior synchronized across files:
  - `CONFIG_APP_REM_ENABLE` gates both initial UI selection in `main/main.c` and REM state updates in `main/application.c`.

- Provisioning behavior is Kconfig-driven:
  - `main/Kconfig.projbuild` defines the `APP_NETWORK_MODE` choice and `APP_PROV_*` options.
  - New networking/provisioning logic should follow these config switches rather than hardcoded mode selection.

- Follow current app error/logging style:
  - Use `esp_err_t` returns, `ESP_ERROR_CHECK` for must-succeed paths, and `ESP_LOGI/W/E` for diagnostics.
