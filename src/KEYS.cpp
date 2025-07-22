#include <Arduino.h>
#include <SI4735.h>

#define KEY_LEFT 0
#define KEY_RIGHT 1
#define KEY_UP 5
#define KEY_PERIOD 50//100  //150 ms

extern SI4735 rx;
extern void bandUp();
extern void disp_refresh();

//unsigned long keyCurrentTime = 0;  //для вычисления интервалов опроса
unsigned long keyLastTime = 0;
bool kl_old = true; bool kl_old_old = true; //нач.значения
bool kr_old = true; bool kr_old_old = true;
bool ku_old = true; bool ku_old_old = true;

void keys_init(){
  pinMode(KEY_LEFT, INPUT_PULLUP); //входы с резисторами на +3.3
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  pinMode(KEY_UP, INPUT_PULLUP);
}

bool kl_state(){
    return !digitalRead(KEY_LEFT); //"лог. 1" - нажатие
}
bool kr_state(){
    return !digitalRead(KEY_RIGHT);
}
bool ku_state(){
    return !digitalRead(KEY_UP);
}

//события от нажатий
void event_kl_on(){
    //Serial.println("Key-Left-ON!");
    rx.frequencyDown(); //уменьшить f на величину шага
}
void event_kr_on(){
    //Serial.println("Key-right-ON!");
    rx.frequencyUp(); //увеличить f на величину шага
}
void event_ku_on(){
    //Serial.println("Key-up-ON!");
    bandUp(); //изменить диапазон
    //val += touchRead(pin);
}

//обработка нажатий кнопок, вызывается из LOOP()
void keys_handle(){
    unsigned long keyCurrentTime = millis();
    if( keyCurrentTime - keyLastTime >= KEY_PERIOD or keyLastTime > keyCurrentTime ) {
        keyLastTime = keyCurrentTime;
        //Serial.println("--Skanning Keys..");
        bool kl = kl_state(); bool kr = kr_state(); bool ku = ku_state(); //читаем текущие состояния
        //ловим передние фронты __/--
        if (kl==true and kl_old==false and kl_old_old==false) event_kl_on();//event key left
        if (kr==true and kr_old==false and kr_old_old==false) event_kr_on();//event key right
        if (ku==true and ku_old==false and ku_old_old==false) event_ku_on();//event key up
        kl_old_old=kl_old; kl_old=kl; //заполняем предыдущие состояния
        kr_old_old=kr_old; kr_old=kr;
        ku_old_old=ku_old; ku_old=ku;
    }
}