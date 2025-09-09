#include <Arduino.h>
#include <SI4735.h>

#define CLK_PIN 6
#define DT_PIN 7
#define POLL_PERIOD 20

extern SI4735 rx;

int counter = 0;
unsigned long pollLastTime = 0; //для вычисления интервалов опроса
bool old_CLK = true;
bool old_old_CLK = true; //антидребезг
int encoderFlag=0; //0-стоим на месте, 1-вправо, -1=влево

//опрос ky-040, установка флага движения encoderFlag
void encoder_polling() {
    unsigned long pollCurrentTime = millis();
    if( pollCurrentTime - pollLastTime >= POLL_PERIOD or pollLastTime > pollCurrentTime ) {
        pollLastTime = pollCurrentTime;
        //читаем текущее значение CLK
        bool curr_CLK = digitalRead(CLK_PIN);
        if (curr_CLK != old_CLK and old_CLK==old_old_CLK) { //ловим изменение сост. CLK + антидребезг 
            if (digitalRead(DT_PIN) != curr_CLK) { //анализ DT
                counter++;
                encoderFlag=1; //флаг-вращали вправо
            } else {
                counter--;
                encoderFlag=-1; //флаг-вращали влево
            }
            //Serial.println("counter: "+String(counter)); //DEBUG
        }
        old_old_CLK =old_CLK; //заполняем предыдущие значения CLK
        old_CLK = curr_CLK;
    }
}

//начальные установки для энкодера
void encoder_setup() {
    pinMode(CLK_PIN, INPUT_PULLUP);
    pinMode(DT_PIN, INPUT_PULLUP);
    old_CLK = digitalRead(CLK_PIN); //читаем предыдущее состояние, иначе первый шаг может быть пустой..
    old_old_CLK = old_CLK; //антидребезг заполняем тоже
}


//работа с энкодером из LOOP()
//без использования прерывания, с применением антидребезга
void encoder_handle() {
    encoder_polling(); //опрос ky-040, установка флагв движения encoderFlag
  //обработка событий вращения энкодера
  if (encoderFlag != 0)
  {
      if (encoderFlag == 1) //флаг вращали вправо
        rx.frequencyUp();
      else                  //флаг вращали влево
        rx.frequencyDown(); 

    encoderFlag = 0; //сброс флага вправо/влево
  }
}
