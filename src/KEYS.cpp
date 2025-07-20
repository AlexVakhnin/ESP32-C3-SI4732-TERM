#include <Arduino.h>

#define KEY_LEFT 0
#define KEY_RIGHT 1
#define KEY_PERIOD 50  //150 ms

unsigned long keyCurrentTime = 0;  //для вычисления интервалов опроса
unsigned long keyLastTime = 0;
bool kl_old = true;bool kl_old_old = true; //нач.значения
bool kr_old = true;bool kr_old_old = true;

void keys_init(){
  pinMode(KEY_LEFT, INPUT_PULLUP);
  pinMode(KEY_RIGHT, INPUT_PULLUP);
}

bool kl_state(){
    return !digitalRead(KEY_LEFT);
}
bool kr_state(){
    return !digitalRead(KEY_RIGHT);
}

void event_kl_on(){
    Serial.println("Key-Left-ON!");
}
void event_kr_on(){
    Serial.println("Key-right-ON!");
}

void keys_handle(){
    keyCurrentTime = millis();
    if( keyCurrentTime - keyLastTime >= KEY_PERIOD or keyLastTime > keyCurrentTime ) {
        //Serial.println("--Skanning Keys..");
        bool kl = kl_state(); bool kr = kr_state(); //текущее состояние
        //ловим передние фронты __/--
        if (kl==true and kl_old==false and kl_old_old==false) event_kl_on();//event key left
        if (kr==true and kr_old==false and kr_old_old==false) event_kr_on();//event key right
        kl_old_old=kl_old;kl_old=kl; //заполняем предыдущие состояния
        kr_old_old=kr_old;kr_old=kr;
        keyLastTime = keyCurrentTime;
    }
}