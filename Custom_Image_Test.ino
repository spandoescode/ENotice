// mapping suggestion for ESP32, e.g. LOLIN32, see .../variants/.../pins_arduino.h for your board
// NOTE: there are variants with different pins for SPI ! CHECK SPI PINS OF YOUR BOARD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#if defined(ESP32)

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/ 16, /*BUSY=*/ 4); // arbitrary selection of (16), 4

#endif

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");

  display.init(115200); // enable diagnostic output on Serial

  Serial.println("setup done");
}

void loop() {
  showBitmapExample();
  delay(2000);
}

#include "customimage.h"
#if defined(_GxGDEW042T2_H_)
void showBitmapExample()
{

uint16_t x = (display.width() - 64) / 2;
uint16_t y = 5;

#if defined(__AVR)
  //display.drawBitmap(epd_bitmap_ass, sizeof(BitmapExample1));
#else
  display.drawBitmap(epd_bitmap_ass, sizeof(BitmapExample1));
  display.fillScreen(GxEPD_WHITE);
  display.update();
  delay(500);
#endif
}
#endif
