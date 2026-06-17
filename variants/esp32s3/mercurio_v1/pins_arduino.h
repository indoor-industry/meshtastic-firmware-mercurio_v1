#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// UART0 physical pins - Serial maps to USB CDC when ARDUINO_USB_CDC_ON_BOOT=1
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// I2C - MCP23017 I/O expander bus
static const uint8_t SDA = 47;
static const uint8_t SCL = 48;

// Default SPI (SPI2_HOST) - ILI9341 display and microSD
static const uint8_t MISO = 12;
static const uint8_t MOSI = 13;
static const uint8_t SCK  = 14;
static const uint8_t SS   = 18; // TFT_CS

#endif /* Pins_Arduino_h */
