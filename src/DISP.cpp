#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define I2C_CLOCK 4
#define I2C_DATA 3
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void disp_refresh(); //объявление

String disp1="1"; //дсплей - строка 1
String disp2="2"; //дсплей - строка 2
String disp3="3"; //дсплей - строка 3
String disp4="4"; //дсплей - строка 4

//инициализация дисплея и интерфейса i2c
void disp_setup(){

  // setup I2C pins explicitly for ESP32C3
  Wire.begin(I2C_DATA, I2C_CLOCK);

  // Setup display SSD1306
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
 // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  display.setTextColor(WHITE);
  delay(500);
}

//обновление информации на дисплее согласно переменным disp1..disp4
void disp_refresh(){
  display.clearDisplay();
  display.setTextSize(2);

  display.setCursor(0, 0);
  display.println(disp1.c_str());

  display.setCursor(0, 16);
  display.println(disp2.c_str());

  display.setCursor(0, 32);
  display.println(disp3.c_str());

  display.setCursor(0, 48);
  display.println(disp4.c_str());

  display.display();
}