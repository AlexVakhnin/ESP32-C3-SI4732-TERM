#include <Arduino.h>
#include <SI4735.h>

#define CLK_PIN 6 //конденсатор 0.1 мкф на землю ОБЯЗАТЕЛЬНО !!!
#define DT_PIN 7

extern SI4735 rx;

volatile int counter = 0;
volatile bool lastStateCLK = true;
volatile int encoderFlag=0; //0-стоим на месте, 1-вправо, -1=влево

//Обработка прерывания
void IRAM_ATTR rotary_encoder() {
    bool currentStateCLK = digitalRead(CLK_PIN);
    if (currentStateCLK != lastStateCLK) {
        if (digitalRead(DT_PIN) != currentStateCLK) {
            counter++;
            encoderFlag=1; //флаг вращали вправо
        } else {
            counter--;
            encoderFlag=-1; //флаг вращали влево
        }
        //Serial.println("counter: "+String(counter)); //DEBUG
    }
    lastStateCLK = currentStateCLK;
//Serial.println("currentStateCLK: "+String(currentStateCLK)); //DEBUG
}

//начальные установки для энкодера (Interrupt..)
void encoder_setup() {
    pinMode(CLK_PIN, INPUT);
    pinMode(DT_PIN, INPUT_PULLUP);
    lastStateCLK = digitalRead(CLK_PIN); //читаем предыдущее состояние, иначе первый шаг может быть пустой..
    attachInterrupt(digitalPinToInterrupt(CLK_PIN), rotary_encoder, CHANGE);
}


//обрабатываем события поворот влево/вправо
void encoder_handle() {
  // Check if the encoder has moved.
  if (encoderFlag != 0)
  {
      if (encoderFlag == 1) //флаг вращали вправо
        rx.frequencyUp();
      else                  //флаг вращали влево
        rx.frequencyDown(); 

    encoderFlag = 0; //сброс флага вправо/влево
  }
}

int encoder_value(){
    return counter;
}