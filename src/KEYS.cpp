#include <Arduino.h>
//#include <SI4735.h>

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
#define BFO 4

//extern SI4735 rx;
extern String disp4;
extern int currVol;
extern int currentBFO;
extern void bandUp();
extern void bandDown();
extern void bandwidth_up();
extern void bandwidth_down();
extern void volume_up();
extern void volume_down();
extern void ssb_on();
extern void ssb_off();
extern void bfo_up();
extern void bfo_down();

extern void disp_refresh();
extern const char *bandwidth[];
extern uint8_t bandwidthIdx;
extern bool ssbLoaded;

const char *menu[] = {"Vol:", "BW:", "Band:", "SSB:", "BFO:"};
const int lastMenu = 5-1; //количество в списке меню
uint8_t menuIdx = 0;

unsigned long keyLastTime = 0; //для вычисления интервалов опроса
bool kl_old = true; bool kl_old_old = true; //нач.значения кнопок
bool kr_old = true; bool kr_old_old = true;
bool ku_old = true; bool ku_old_old = true;
bool ks_old = true; bool ks_old_old = true;

void keys_init(){
  pinMode(KEY_LEFT, INPUT_PULLUP); //входы с резисторами на +3.3
  pinMode(KEY_RIGHT, INPUT_PULLUP);
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_SW, INPUT); //кнопка энкодера
}

//обновление информации в нижней строке дисплея (меню)
void fill_menu_string(){
    if(menuIdx==VOLUME){
        disp4 = "Vol: "+String(currVol);
    }
    else if(menuIdx==BANDWIDTH){
        disp4 = "BW: "+String(bandwidth[bandwidthIdx])+"kHz";
    }
    else if(menuIdx==BAND){
        disp4 = "Band: +/-";
    }
    else if(menuIdx==SSB){
        String ssbonoff = (ssbLoaded) ? ("on") : ("off");
        disp4 = "SSB: "+ssbonoff;
    }
    else if(menuIdx==BFO){
        disp4 = "BFO:";
        if(currentBFO > 0) disp4+="+"+String(currentBFO);
        else disp4+=String(currentBFO);
    }
}

void menu_rotate(){
    if (menuIdx < lastMenu){ menuIdx++; }
    else { menuIdx = 0; }

    String smenu = menu[menuIdx];
    Serial.print("MENU->");Serial.println(smenu);
    fill_menu_string();
    disp_refresh();
}


//события от нажатий key-right
void event_kr_on(){
    if(menuIdx==VOLUME) volume_up();
    if(menuIdx==BANDWIDTH) bandwidth_down();
    if(menuIdx==BAND) bandUp();
    if(menuIdx==SSB) ssb_on();
    if(menuIdx==BFO) bfo_up();
}
//события от нажатий key-left
void event_kl_on(){
    if(menuIdx==VOLUME) volume_down();
    if(menuIdx==BANDWIDTH) bandwidth_up();
    if(menuIdx==BAND) bandDown();
    if(menuIdx==SSB) ssb_off();
    if(menuIdx==BFO) bfo_down();
}
void event_ku_on(){ //кнопка вращения МЕНЮ
    menu_rotate();
}
void event_ks_on(){ //событие кнопки энкодера
    bandUp(); //изменить диапазон
}

//обработка нажатий кнопок, вызывается из LOOP()
void keys_handle(){
    unsigned long keyCurrentTime = millis();
    if( keyCurrentTime - keyLastTime >= KEY_PERIOD or keyLastTime > keyCurrentTime ) {
        keyLastTime = keyCurrentTime;
        //читаем текущие состояния (они инверсные)
        bool kl = !digitalRead(KEY_LEFT);
        bool kr = !digitalRead(KEY_RIGHT);
        bool ku = !digitalRead(KEY_UP);
        bool ks = !digitalRead(KEY_SW);
        //ловим передние фронты ___/--
        if (kl==true and kl_old==false and kl_old_old==false) event_kl_on();//event key left
        if (kr==true and kr_old==false and kr_old_old==false) event_kr_on();//event key right
        if (ku==true and ku_old==false and ku_old_old==false) event_ku_on();//event key up
        if (ks==true and ks_old==false and ks_old_old==false) event_ks_on();//event key SW
        //заполняем предыдущие состояния
        kl_old_old=kl_old; kl_old=kl;
        kr_old_old=kr_old; kr_old=kr;
        ku_old_old=ku_old; ku_old=ku;
        ks_old_old=ks_old; ks_old=ks;
    }
}