#include <Arduino.h>
#include <SI4735.h>

#define CLK_PIN 6 //конденсатор 0.1 мкф на землю ОБЯЗАТЕЛЬНО !!!
#define DT_PIN 7
//#define SW_PIN 8

//extern volatile bool tempfail; //флаг для блокировки реле по резкому падению температуры
//extern bool overheat; //флаг для блокировки реле по перегреву
//extern boolean flag_apn;
extern SI4735 rx;

volatile int counter = 0;
int lastStateCLK;
volatile int encoderFlag=0;//0-стоим на месте, 1-вправо, -1=влево

//Обработка прерывания
void IRAM_ATTR rotary_encoder() {
    int currentStateCLK = digitalRead(CLK_PIN);
    if (currentStateCLK != lastStateCLK) {
        if (digitalRead(DT_PIN) != currentStateCLK) {
            counter++;
            encoderFlag=1; //флаг вращали вправо
        } else {
            counter--;
            encoderFlag=-1; //флаг вращали влево
        }
        //Serial.print("Position: ");
        //Serial.println(counter);
    }
    lastStateCLK = currentStateCLK;
}

//начальные установки для энкодера (Interrupt..)
void encoder_setup() {
    pinMode(CLK_PIN, INPUT);
    pinMode(DT_PIN, INPUT);
    //pinMode(SW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLK_PIN), rotary_encoder, CHANGE);
}


//обрабатываем события поворот влево/вправо
void encoder_handle() {
  // Check if the encoder has moved.
  if (encoderFlag != 0)
  {
    //if (bfoOn)
    //{
    //  currentBFO = (encoderCount == 1) ? (currentBFO + currentBFOStep) : (currentBFO - currentBFOStep);
    //}
    //else
    //{
      if (encoderFlag == 1) //флаг вращали вправо
        rx.frequencyUp();
      else                  //флаг вращали влево
        rx.frequencyDown(); 

      // Show the current frequency only if it has changed
      //delay(30);
      //currentFrequency = si4735.getFrequency();
    //}
    encoderFlag = 0; //сброс флага вправо/влево
  }




    /*
    if (digitalRead(SW_PIN) == LOW) {
        //Serial.println("Button Pressed");
        if(counter != 0){counter=0;}
        tempfail = false;
        overheat = false;
    }
    */
}


int encoder_value(){
    return counter;
}