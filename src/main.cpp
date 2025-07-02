#include <Arduino.h>


extern void radio_setup();
extern void term_handle();
extern void test_term_handle();

void setup() {
  Serial.begin(115200);
  delay(10000);//delay(10000);  //10 sec для отладки !
  Serial.println("----------------Start Info-----------------");
  Serial.printf("Total heap:\t%d \r\n", ESP.getHeapSize());
  Serial.printf("Free heap:\t%d \r\n", ESP.getFreeHeap());
  Serial.println("I2C_SDA= "+String(SDA));
  Serial.println("I2C_SCL= "+String(SCL));
  Serial.println("-------------------------------------------");

  radio_setup();

}

void loop() {

  term_handle();
  //test_term_handle();

}

