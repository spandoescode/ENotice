// mapping suggestion for ESP32, e.g. LOLIN32, see .../variants/.../pins_arduino.h for your board
// NOTE: there are variants with different pins for SPI ! CHECK SPI PINS OF YOUR BOARD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V

// include library, include base class, make path known
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <string>
#include <iostream>

std::string bt_message = "";
std::vector<uint8_t> bt_array;
uint8_t *p_bt_array = NULL;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  45        /* Time ESP32 will go to sleep (in seconds) */

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/ 16, /*BUSY=*/ 4); // arbitrary selection of (16), 4

RTC_DATA_ATTR int bootCount = 0;
unsigned long startTime = 0;
unsigned long interval = 15000UL;
bool isConnected = false;

using namespace std;

void image(std::string h) {
  // convert string to uint_8t array
  std::vector<uint8_t> im(h.begin(), h.end());
  uint8_t *p = &im[0];
  
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(0, 0, p, 400, 300, GxEPD_BLACK);
  display.update();
}

void showFont(String message, const GFXfont* f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(7, 15);
  display.println(message);
  display.update();
  Serial.println("done2");
  delay(5000);
}

void message(String h){
  showFont(h, &FreeMonoBold9pt7b);
  Serial.println("done1");
}

void startAdvertising() {
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    isConnected = true;
    Serial.println("Connected to phone.");
  };
  void onDisconnect(BLEServer* pServer) {
    isConnected = false;
    Serial.println("Disconnecting from phone.");
  }
};

class MyCallbackHandler: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    // code here 
    std::string bt_string = pCharacteristic->getValue();

    if (bt_string == "")
    {
        bt_message = bt_string;
    } 
    else 
    {
        // Initialise temp variable
        std::string temp = bt_string;
        
        // Append new data to old data
        bt_message = bt_message + temp;   
    }

    // Print message to console 
    String printer = String(bt_message.c_str());
    Serial.println(printer);

    // Display the message/image
    message(printer);

    Serial.println("done3");
    
    /*
    if (printer[0] == '0') {
      image(bt_message);
    } else {
      String bt_message = String(bt_string.c_str());
      message(bt_message);
    }
    */
  }
};

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println("Setting up screen");
  //display.init(115200);
  //Serial.println("Screen started");
  Serial.println("Starting BLE work!");

  // name the device
  BLEDevice::init("ENotice");

  // create a server and set up the callback
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // create a service 
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // create a characteristic
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbackHandler());

  // start the service
  pService->start();

  // initialise and start advertising
  startAdvertising();

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  /*
  Next we decide what all peripherals to shut down/keep on
  By default, ESP32 will automatically power down the peripherals
  not needed by the wakeup source, but if you want to be a poweruser
  this is for you. Read in detail at the API docs
  http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
  Left the line commented as an example of how to configure peripherals.
  The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  /*
  Now that we have setup a wake cause and if needed setup the
  peripherals state in deep sleep, we can now start going to
  deep sleep.
  In the case that no wake up sources were provided but deep
  sleep was started, it will sleep forever unless hardware
  reset occurs.
  */
}

void loop(){
  //This is not going to be called
  if (!isConnected) {
    if(millis() - startTime > interval) {
      Serial.println("Going to sleep now");
      Serial.flush(); 
      esp_deep_sleep_start();
      Serial.println("This will never be printed"); 
  }
  }
}
