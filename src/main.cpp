/*
  Based on PU2CLR sources,
  adapted for ESP32C3, 2025.
*/


#include <Arduino.h>


extern void radio_setup();
extern void term_handle();
extern void disp_setup();
extern void keys_init();
extern void keys_handle();
extern void encoder_setup();
extern void encoder_handle();
extern void change_freq_handle();

void setup() {
  Serial.begin(115200);
  delay(5000);//delay(10000);  //10 sec для отладки !
  Serial.println("----------------Start Info-----------------");
  Serial.printf("Total heap:\t%d \r\n", ESP.getHeapSize());
  Serial.printf("Free heap:\t%d \r\n", ESP.getFreeHeap());
  Serial.println("-------------------------------------------");

  disp_setup(); //1 (init i2c pins)
  keys_init();
  encoder_setup();
  radio_setup(); //2
}

void loop() {
  term_handle(); //события от клавиатуры терминала T=100
  keys_handle(); //события от кнопок T=50
  encoder_handle(); //события от поворота энкодера T=0 (по прерыванию)
  change_freq_handle(); //реакция на изменение частоты приемника T=0 + delay(30)
}

