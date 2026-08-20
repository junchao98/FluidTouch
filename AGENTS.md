# FluidTouch — Agent 指南

ESP32 触屏 CNC 控制器（FluidNC WebSocket 客户端），LVGL 9.5 UI，PlatformIO 构建。
本仓库是本地工作副本：在上游 jeyeager65/FluidTouch 基础上加了 M5Stack Tab5（ESP32-P4）移植，本地提交直接落在 main 分支。

## 常用命令

```bash
pio run -e m5tab5                              # 构建 Tab5
pio run -e m5tab5 -t upload --upload-port /dev/ttyACM0   # 烧写（设备固定在 /dev/ttyACM0）
pio device monitor -b 115200                   # 串口监视
```

- 共 4 个环境，**改动必须保证全部编译通过**：`m5tab5` + `elecrow-crowpanel-7-basic` / `-advance-v12` / `-advance-v13`。CI 和 Web 安装器只覆盖 CrowPanel 三个（Tab5 是本地专属，勿加入 CI/release 流程）。
- **不要删除 .pio 构建目录** — 只做增量编译。
- 无测试套件/lint；验证 = 四环境编译通过 + 真机测试。
- 串口抓取技巧（Tab5 USB-CDC 直接 RTS/DTR 复位不可靠）：`esptool.py --chip esp32p4 --port /dev/ttyACM0 --before default_reset run` 后立即以 115200 读端口。
- 截图调试：WiFi IP 是动态的，从串口日志获取，浏览器访问 `http://<ip>/screenshot.bmp`。

## 硬件门控

- 条件编译宏：`HARDWARE_TAB5` / `HARDWARE_BASIC` / `HARDWARE_ADVANCE`（+`HARDWARE_ADVANCE_V12/V13`）；P4 专属 LVGL 项另用 `CONFIG_IDF_TARGET_ESP32P4` 门控。
- Tab5 环境用 pioarduino 平台包，依赖 pre 脚本 `scripts/fix_riscv_asm_flags.py`（勿删）。
- `scripts/generate_defaults.py` 构建时把 `hardware_defaults.ini` 烘焙为 `include/generated/hardware_defaults.h`（首启动出厂默认）。

## Tab5 显示管线（勿回退）

- `src/core/display_driver.cpp`：LVGL 双 64 字节对齐 PSRAM 全屏缓冲 → `ppa_flush()` 用 PPA SRM DMA（旋转 90/270 + byte_swap）直写 DSI framebuffer，含 cache 同步。已验证全屏重绘 884ms→103ms；改动后必须在真机验证颜色与性能，PPA 失败时回退 `pushImageDMA` 路径。
- 截图字节序陷阱：Tab5 `readRect` 返回原生序，`screenshot_server.cpp` 的 `rgb565_to_rgb888` **不做**字节交换；CrowPanel（LovyanGFX）路径才需要交换。勿"修复"成双重交换。
- LVGL 性能配置集中在 `include/lv_conf.h`（`LV_USE_OS=FREERTOS`、`LV_DRAW_SW_DRAW_UNIT_CNT=2`、`LV_USE_PPA`）。

## 编码约定

- 颜色一律用 `UITheme::` 常量，禁止裸 `lv_color_hex()`。
- 触摸事件用 `LV_EVENT_CLICKED`；标签更新必须 delta check（值变化才 `set_text`）。
- >10KB 缓冲用 `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`。
- UI 类模式：静态 `create(parent)` 工厂 + 静态成员指针；文件 `ui_tab_*.h/cpp` ↔ 类名 `UITab*`。
- UI 布局需同时适配 800×480（CrowPanel）与 1280×720（Tab5），几何量用 UI_SCALE。

## 用户约定

- 提交信息：一句话即可（英文）。
- 启动必须显示手动机床选择界面 — 勿重新引入 `FLUIDTOUCH_AUTO_CONNECT` 自动连接调试标志。

## 深入参考

- `.github/copilot-instructions.md` — 详细架构/模块/陷阱清单（注意其中 LVGL 版本与环境名比 platformio.ini 旧，冲突时以 platformio.ini 为准）。
- `docs/development.md` — 构建/调试/上游分支流程（上游在 dev 分支开发、PR 到 dev）。
