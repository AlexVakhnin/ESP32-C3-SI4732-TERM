#include <Arduino.h>
#include <SI4735.h>

#define KEY_LEFT 0
#define KEY_RIGHT 1
#define KEY_UP 5
#define KEY_SW 8 //кнопка энкодера
#define KEY_PERIOD 50//100  //150 ms

//меню
#define VOLUME 0
#define BANDWIDTH 1
#define BAND 2
#define SSB 3

extern SI4735 rx;
extern String disp4;
extern int currVol;
extern void bandUp();
extern void disp_refresh();
extern const char *bandwidth[];
extern uint8_t bandwidthIdx;


const char *menu[] = {"Vol:", "BW:", "Band:", "SSB:"};
const int lastMenu = 3; //количество в списке меню
uint8_t menuIdx = 0;
String smenu = "Vol:";



//unsigned long keyCurrentTime = 0;  //для вычисления интервалов опроса
unsigned long keyLastTime = 0;
bool kl_old = true; bool kl_old_old = true; //нач.значения
bool kr_old = true; bool kr_old_old = true;
bool ku_old = true; bool ku_old_old = true;
bool ks_old = true; bool ks_old_old = true;

void keys_init(){
  pinMode(KEY_LEFT, INPUT_PULLUP); //входы с резисторами на +3.3
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_SW, INPUT); //кнопка энкодера
}

bool kl_state() { return !digitalRead(KEY_LEFT); } //"лог. 1" - нажатие
bool kr_state() { return !digitalRead(KEY_RIGHT); }
bool ku_state() { return !digitalRead(KEY_UP); }
bool ks_state() { return !digitalRead(KEY_SW); }

void menu_rotate(){
    if (menuIdx < lastMenu){ menuIdx++; }
    else { menuIdx = 0; }

    smenu = menu[menuIdx];
    Serial.println(smenu);
    //обновление информации на дисплее
    if(menuIdx==VOLUME){
        disp4 = "Vol: "+String(currVol);
        disp_refresh();
    }
    else if(menuIdx==BANDWIDTH){
        disp4 = "BW: "+String(bandwidth[bandwidthIdx])+"kHz";
        disp_refresh();
    }
    else if(menuIdx==BAND){

    }
    else if(menuIdx==SSB){

    }
}

//события от нажатий
void event_kl_on(){
    Serial.println("Key-up-Left!");
}
void event_kr_on(){
    Serial.println("Key-up-Right!");
}
void event_ku_on(){
    menu_rotate();
}
void event_ks_on(){ //событие кнопки энкодера
    //Serial.println("Key-SW-ON!");
    bandUp(); //изменить диапазон
}

//обработка нажатий кнопок, вызывается из LOOP()
void keys_handle(){
    unsigned long keyCurrentTime = millis();
    if( keyCurrentTime - keyLastTime >= KEY_PERIOD or keyLastTime > keyCurrentTime ) {
        keyLastTime = keyCurrentTime;
        //читаем текущие состояния
        bool kl = kl_state(); bool kr = kr_state(); bool ku = ku_state(); bool ks = ks_state();
        //ловим передние фронты ___/--
        if (kl==true and kl_old==false and kl_old_old==false) event_kl_on();//event key left
        if (kr==true and kr_old==false and kr_old_old==false) event_kr_on();//event key right
        if (ku==true and ku_old==false and ku_old_old==false) event_ku_on();//event key up
        if (ks==true and ks_old==false and ks_old_old==false) event_ks_on();//event key SW
        kl_old_old=kl_old; kl_old=kl; //заполняем предыдущие состояния
        kr_old_old=kr_old; kr_old=kr;
        ku_old_old=ku_old; ku_old=ku;
        ks_old_old=ks_old; ks_old=ks;
    }
}