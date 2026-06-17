#include "variant.h"
#include <Arduino.h>
#include <Wire.h>

// MCP23017 register addresses (BANK=0 mode, factory default)
#define MCP_ADDR   0x20
#define MCP_IODIRA 0x00  // port A direction: 0=output, 1=input
#define MCP_IODIRB 0x01  // port B direction
#define MCP_GPPUA  0x0C  // port A pull-ups
#define MCP_GPPUB  0x0D  // port B pull-ups
#define MCP_OLATA  0x14  // port A output latch
#define MCP_OLATB  0x15  // port B output latch
#define MCP_IOCON  0x0A  // configuration register

static void mcp_write(uint8_t reg, uint8_t val)
{
    Wire.beginTransmission(MCP_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// Called by Meshtastic very early in setup(), before SPI/I2C init.
void earlyInitVariant()
{
    // Deassert all SPI CS lines before any SPI bus starts to prevent
    // bus contention when multiple slaves share the same bus.
    pinMode(LORA_CS, OUTPUT);  digitalWrite(LORA_CS, HIGH);
    pinMode(6, OUTPUT);        digitalWrite(6, HIGH);   // XPT2046 touch CS
    pinMode(TFT_CS, OUTPUT);   digitalWrite(TFT_CS, HIGH);
    pinMode(42, OUTPUT);       digitalWrite(42, HIGH);  // microSD CS

    // Hardware-reset MCP23017: GPIO40 is active-LOW RST.
    // After reset the chip comes up with all pins as high-impedance inputs.
    pinMode(40, OUTPUT);
    digitalWrite(40, LOW);
    delay(10);
    digitalWrite(40, HIGH);
    delay(10);

    // Configure MCP23017 via I2C.
    // Wire.begin() here is safe: Meshtastic will call it again later
    // with the same pins, which just re-arms the bus without side effects.
    Wire.begin(I2C_SDA, I2C_SCL);

    // IOCON: mirror INTA/INTB so a single interrupt line serves both ports
    mcp_write(MCP_IOCON, 0x40);

    // IODIRA: 0=output, 1=input
    //   GPA0 (GPS_RST)  = output   -> 0
    //   GPA1 (BAT_STAT1)= input    -> 1
    //   GPA2 (DISP_RST) = output   -> 0
    //   GPA3 (GPS_WKUP) = output   -> 0
    //   GPA4 (GPS_1PPS) = input    -> 1
    //   GPA5 (AUDIO_EN) = output   -> 0
    //   GPA6 (SW1)      = input    -> 1
    //   GPA7 (unused)   = output   -> 0
    // 0b01010010 = 0x52
    mcp_write(MCP_IODIRA, 0x52);

    // IODIRB:
    //   GPB0 (SW2)       = input   -> 1
    //   GPB1 (LORA_DIO2) = input   -> 1
    //   GPB2 (LORA_DIO3) = input   -> 1
    //   GPB3 (unused)    = output  -> 0
    //   GPB4 (TOUCH_IRQ) = input   -> 1
    //   GPB5 (BAT_STAT2) = input   -> 1
    //   GPB6 (SD_DET)    = input   -> 1
    //   GPB7 (unused)    = output  -> 0
    // 0b01110111 = 0x77
    mcp_write(MCP_IODIRB, 0x77);

    // Pull-ups on input pins (active-LOW buttons, status inputs)
    mcp_write(MCP_GPPUA, 0x42); // GPA1, GPA6
    mcp_write(MCP_GPPUB, 0x71); // GPB0, GPB4, GPB5, GPB6

    // Initial output state:
    //   GPA0 (GPS_RST)  = HIGH  (active-LOW reset; hold out of reset)
    //   GPA2 (DISP_RST) = LOW   (will be pulsed HIGH below)
    //   GPA3 (GPS_WKUP) = HIGH  (keep GPS awake)
    //   GPA5 (AUDIO_EN) = LOW   (amplifier off at boot)
    // 0b00001001 = 0x09
    mcp_write(MCP_OLATA, 0x09);
    delay(10);

    // Pulse ILI9341 reset HIGH to bring it out of reset.
    // GPA2 (DISP_RST) = HIGH -> 0b00001101 = 0x0D
    mcp_write(MCP_OLATA, 0x0D);
    delay(120); // ILI9341 datasheet: wait >=120 ms after RST goes HIGH
}
