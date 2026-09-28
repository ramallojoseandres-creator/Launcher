#include "hal/bright/bright.h"
#include "hal/device.h"
#include "idf/launcher_platform.h"
#include "powerSave.h"
#include <Arduino.h>
#include <interface.h>

// ESP32-S3 N16R8 + ILI9341 2.8" SPI.
// Con HAS_TOUCH: tactil XPT2046 en pines propios (barra inferior Esc/Prev/Next).
// Con HAS_3_BUTTON: 3 botones a GND con pull-up interno.

#if defined(HAS_TOUCH)
#include "hal/inputs/touch.h"
static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // Misma tabla que el modulo generico 2.8" (CYD-2432S028)
    // rotacion:        0      1      2      3
    bool swapXY[4] = {true, false, true, false};
    bool mirrorX[4] = {true, false, false, true};
    bool mirrorY[4] = {false, false, true, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}
#else
#include "hal/inputs/buttons.h"
static DeviceButtons buttonsCfg() {
    DeviceButtons cfg{BTN_PREV, BTN_NEXT, BTN_SEL};
    return cfg;
}
#endif

void _setup_gpio() {
    // Todos los CS en alto para que el bus arranque limpio
    launcherGpioOutput(TFT_CS);
    launcherGpioWrite(TFT_CS, HIGH);
    launcherGpioOutput(SDCARD_CS);
    launcherGpioWrite(SDCARD_CS, HIGH);
#if defined(HAS_TOUCH)
    launcherGpioOutput(CYD28_TouchR_CS);
    launcherGpioWrite(CYD28_TouchR_CS, HIGH);
#else
    hal_buttons_init(buttonsCfg(), 3);
#endif
}

void _post_setup_gpio() {
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, bright);
#if defined(HAS_TOUCH)
    if (!hal_touch_init(touchCfg(), 0x5D, false)) {
        launcherConsolePrintf("%s\n", String("Touch IC not Started").c_str());
    } else launcherConsolePrintf("%s\n", String("Touch IC Started").c_str());
#endif
}

void _late_setup_gpio() {}

int getBattery() { return 0; }

void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

void InputHandler(void) {
#if defined(HAS_TOUCH)
    static long tm = launcherMillis();
    if (launcherMillis() - tm > 250 || LongPress) {
        LTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            tm = launcherMillis();
            if (!hal_touch_apply(t)) return;
        }
    }
#else
    hal_buttons_poll_3(buttonsCfg());
#endif
}

void powerOff() {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_deep_sleep_start();
}

void reboot() { ESP.restart(); }
