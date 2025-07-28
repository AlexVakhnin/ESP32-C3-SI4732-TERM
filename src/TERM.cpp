/*
  Based on PU2CLR sources,
  adapted for ESP32C3, 2025.
*/

#include <Arduino.h>
#include <SI4735.h>

#define TERM_PERIOD 100  //150 ms
unsigned long termLastTime = 0;

extern SI4735 rx;
extern int bandIdx;
extern void useBand();
extern void bandUp();
extern void bandDown();
//extern int currVol;
extern void disp_refresh();
extern void showStatus();
extern void volume_up();
extern void volume_down();
extern void bandwidth_up();
extern void bandwidth_down();
//extern String disp4;
//extern const char *bandwidth[];
//extern uint8_t bandwidthIdx;


void term_setup(){


}


//ЧАСТОТА ПЕЧАТЬ (используется для поиска)
void showFrequency( uint16_t freq )
{
  if (rx.isCurrentTuneFM())
  {
    Serial.print(String(freq / 100.0, 2));
    Serial.println(" MHz");
  }
  else
  {
    Serial.print(freq);
    Serial.println(" kHz");
  }
}

void showHelp()
{
  Serial.println("Commands: F=FM, A=AM, U/D=Freq +/-, S/s=Seek, +=Vol+, -=Vol-, B=BW, 0=Status, ?=Help");
  Serial.println("==================================================");
}

//***************************************************************************
//ТЕРМИНАЛ
void term_handle()
{
  unsigned long termCurrentTime = millis();
  if( termCurrentTime - termLastTime >= TERM_PERIOD or termLastTime > termCurrentTime ) {
    termLastTime = termCurrentTime;
    if (Serial.available() > 0)
    {
      char key = Serial.read(); //читаем символ с клавиатуры
      //Serial.println("Key="+String(key));
      switch (key)
      {
      case 'q':
        //
        break;
      case 'w':
        //
        break;
      case 'e':
        //
        break;
      case 'r':
        //
        break;
      case '1':
        bandDown();
        break;
      case '2':
        bandUp();
        break;
      case '+':
        volume_up();
        break;
      case '-':
        volume_down();
        break;
      case 'a':
      case 'A':
        bandIdx=1; //AM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        break;
      case 'f':
      case 'F':
        bandIdx=0; //0-FM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        rx.setFrequency(9860); //проминь
        break;
      case 'U':
      case 'u':
        rx.frequencyUp(); //увеличить f на величину шага
        break;
      case 'D':
      case 'd':
        rx.frequencyDown(); //уменьшить f на величину шага
        break;
      case 'B':
        bandwidth_up();
        break;
      case 'b':
        bandwidth_down();
        break;
      case 'S':
        rx.seekStationProgress(showFrequency, 1);//поиск станции вверх
        break;
      case 's':
        rx.seekStationProgress(showFrequency, 0);//поиск станции вниз
        break;
      case '0':
        showStatus();
        disp_refresh();
        break;
      case '?':
        showHelp();
        break;
      default:
        break;
      }
    }
  }
}