#pragma once

#include <stdint.h> // uint8_t for mercurio_readButtons()'s declaration below

// =====================================================================
// Mercurio v1 - ESP32-S3-N16R8 custom board
// Verified against LoRa-device.net (KiCad netlist, 2026-05-13)
//
// Peripherals:
//   LoRa  : SX1262 (RA-01SH-P) on SPI-A (SPI3_HOST via SPI1/HSPI)
//   TFT   : ILI9341 240x320 on SPI-B (SPI2_HOST)
//   Touch : XPT2046 on SPI-A (interrupt via MCP23017 GPB4)
//   SD    : microSD on SPI-B (CS=GPIO42)
//   GPS   : Quectel L76KB-A58 on Serial1 (GPIO43/44)
//   I2C   : MCP23017 @ 0x20 on GPIO47/48
//   Audio : MAX98357A + ICS-43432 on I2S (GPIO 1,4,8,21)
//   ADC   : Battery divider 47k/47k on GPIO9
// =====================================================================

// ----- LoRa SX1262 (SPI-A, SPI3_HOST via SPI1/HSPI) -----------------
#define USE_SX1262

// Tell Meshtastic to use SPI1 (HSPI = SPI3_HOST) for LoRa so the
// ILI9341 display can use SPI2_HOST without conflict.
#define HW_SPI1_DEVICE

#define LORA_SCK  38
#define LORA_MISO 41
#define LORA_MOSI 39
#define LORA_CS    2

#define SX126X_CS    LORA_CS
#define SX126X_DIO1  5    // IRQ (DIO1 on SX1262)
#define SX126X_BUSY  7    // BUSY
#define SX126X_RESET 17

// RFEN (GPIO16): drive HIGH to enable RA-01SH-P RF front-end.
// Meshtastic's SX126xInterface sets this HIGH once in init() and leaves it.
#define SX126X_ANT_SW 16

// RA-01SH-P has an internal TCXO driven by SX1262 DIO3 (not routed to
// a free GPIO; DIO2/DIO3 are read-only via MCP23017 for diagnostics).
// This voltage matches the RadioLib default used in the test firmware.
#define SX126X_DIO3_TCXO_VOLTAGE 1.6

// DIO2 is NOT wired to a free GPIO, so do NOT define SX126X_DIO2_AS_RF_SWITCH.
// The RA-01SH-P handles TX/RX switching internally.
#define LORA_DIO0  RADIOLIB_NC // not connected
#define LORA_RESET SX126X_RESET
#define LORA_DIO1  SX126X_DIO1

// ----- ILI9341 TFT display (SPI-B, SPI2_HOST) -----------------------
#define ILI9341_DRIVER
#define ILI9341_SPI_HOST SPI2_HOST

#define TFT_SCLK  14
#define TFT_MOSI  13
#define TFT_MISO  12
#define TFT_CS    18
#define TFT_DC    15
// Hardware reset is via MCP23017 GPA2, pulsed in earlyInitVariant().
// LovyanGFX handles -1 as "no software reset pin".
#define TFT_RST  -1
#define TFT_BL    11  // backlight, active HIGH, PWM-capable
#define TFT_BUSY -1   // not connected on ILI9341

#define TFT_WIDTH          240
#define TFT_HEIGHT         320
#define TFT_OFFSET_X         0
#define TFT_OFFSET_Y         0
#define TFT_OFFSET_ROTATION  0

#define USE_TFTDISPLAY             1
#define SCREEN_TRANSITION_FRAMERATE 5
#define BRIGHTNESS_DEFAULT         100

// LovyanGFX SPI bus frequencies
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000

// ----- GPS: Quectel L76KB-A58 (Serial1 / UART1) ---------------------
// RST and WKUP are via MCP23017 and handled in earlyInitVariant().
#define HAS_GPS 1
#define GPS_RX_PIN 44
#define GPS_TX_PIN 43

// ----- Battery ADC (GPIO9 = ADC1_CH8, 47k/47k voltage divider) ------
#define BATTERY_PIN    9
#define ADC_CHANNEL    ADC_CHANNEL_8
#define ADC_MULTIPLIER 2.1  // 2.0 hardware ratio + ~5% ADC correction

// ----- I2C (MCP23017 I/O expander @ 0x20) ---------------------------
#define I2C_SDA 47
#define I2C_SCL 48

// ----- User input ----------------------------------------------------
// BOOT button (GPIO0) is the only directly-wired button.
// SW1 (MCP GPA6) and SW2 (MCP GPB0) are behind the I/O expander
// and require custom input handling (not wired here).
#define BUTTON_PIN          0
#define BUTTON_ACTIVE_LOW   true
#define BUTTON_ACTIVE_PULLUP true

// ----- XPT2046 resistive touch (SPI-A, CS=GPIO6) --------------------
// IRQ is MCP23017 GPB4 (not a free GPIO), so we poll without interrupt.
// The XPT2046 shares SPI3_HOST/HSPI with LoRa; reads go through SPI1
// which participates in Arduino's SPI mutex (safe with RadioLib).
#define HAS_TOUCHSCREEN 1
#define TOUCH_CS 6

// Raw 12-bit ADC → screen pixel mapping. Tune after first flash:
//   axes swapped  → toggle TOUCH_SWAP_XY
//   X inverted    → toggle TOUCH_INVERT_X
//   Y inverted    → toggle TOUCH_INVERT_Y
//   scale wrong   → adjust MIN/MAX
#define TOUCH_X_MIN      300
#define TOUCH_X_MAX     3900
#define TOUCH_Y_MIN      400
#define TOUCH_Y_MAX     3900
#define TOUCH_SWAP_XY      1
#define TOUCH_INVERT_X     0
#define TOUCH_INVERT_Y     1

// ----- Audio: MAX98357A speaker amp (I2S0) ---------------------------
// Mic (ICS-43432, DIN=GPIO8) isn't wired up yet - no generic mic-input
// pathway exists in mainline Meshtastic for SX1262 boards (the only
// existing voice module, AudioModule, is hard-gated to USE_SX1280).
// AUDIO_EN (MCP23017 GPA5, amp shutdown/enable) is toggled by
// mercurio_setAudioEnable() in variant.cpp, called from AudioThread.h.
// Pins verified working in test_firmware/src/audio_i2s.cpp.
#define HAS_I2S
#define DAC_I2S_BCK  4
#define DAC_I2S_WS   1
#define DAC_I2S_DOUT 21
// AudioThread.h's initOutput() unconditionally calls the 4-arg SetPinout()
// overload, so this needs a value even though MAX98357A doesn't use MCLK.
#define DAC_I2S_MCLK -1

// Implemented in variant.cpp; called from src/AudioThread.h.
void mercurio_setAudioEnable(bool on);

// ----- Side buttons SW1 (MCP GPA6) / SW2 (MCP GPB0) -> volume ---------
// Not on free GPIOs, so they're polled via I2C (see AudioThread.h) rather
// than a GPIO interrupt. Implemented in variant.cpp.
uint8_t mercurio_readButtons();

// ----- microSD (SPI-B, shares the ILI9341 bus) ------------------------
// Shares the display's hardware SPI bus, same as T-Deck/tlora-pager
// (default SdFat driver, Arduino SPIClass). Card-detect is MCP23017
// GPB6, input+pullup in earlyInitVariant().
#define SDCARD_CS 42
#define SPI_SCK   14
#define SPI_MOSI  13
#define SPI_MISO  12

// ----- PSRAM ---------------------------------------------------------
#define BOARD_HAS_PSRAM
