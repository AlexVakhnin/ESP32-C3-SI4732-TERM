#include <Arduino.h>
#include <SI4735.h>

#define TERM_PERIOD 100  //150 ms
unsigned long termLastTime = 0;

extern SI4735 rx;
extern int bandIdx;
extern void useBand();
extern void bandUp();
extern void bandDown();
extern int currVol;
extern uint8_t bandwidthIdx;
extern void disp_refresh();
extern void showStatus();
extern String disp4;
extern const char *bandwidth[];


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
        bandIdx=7; //41-AM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        //rx.setAM(6800, 7800, 7445, 5); //41m
        break;
      case 'w':
        bandIdx=10; //25-AM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        //rx.setAM(11200, 12500, 12035, 5); //25m
        break;
      case 'e':
        bandIdx=11; //22-AM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        //rx.setAM(13400, 13900, 13635, 5); //22m
        break;
      case 'r':
        bandIdx=12; //20-AM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        //rx.setAM(14000, 14500, 14200, 1); //20m
        break;
      case '1':
        bandDown();
        break;
      case '2':
        bandUp();
        break;
      case '+':
        rx.volumeUp(); //звук+
        currVol = rx.getCurrentVolume();
        Serial.println("Vol[0-63]="+String(currVol));
        disp4 = "Vol: "+String(currVol);
        disp_refresh();
        break;
      case '-':
        rx.volumeDown();
        currVol = rx.getCurrentVolume();
        Serial.println("Vol[0-63]="+String(currVol));
        disp4 = "Vol: "+String(currVol);
        disp_refresh();
        break;
      case 'a':
      case 'A':
        bandIdx=13; //19m
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        rx.setFrequency(15665);//китай
        //rx.setAM(520, 1710, 1386, 1);   //AM-(520-1700 kHz, start at 1000 kHz, step 10 kHz) диапазон СВ
        //rx.setAM(3500, 4500, 3700, 1);//80m
        break;
      case 'f':
      case 'F':
        bandIdx=0; //0-FM
        useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
        rx.setFrequency(9860); //проминь
        //rx.setFM(8400, 10800, 9860, 10); //FM (84-108 mHz, start at 106.5 MHz, step 0.1 mHz)
        break;
      case 'U':
      case 'u':
        rx.frequencyUp(); //увеличить f на величину шага
        //Serial.println("freq_up");
        break;
      case 'D':
      case 'd':
        rx.frequencyDown(); //уменьшить f на величину шага
        break;
      case 'B':
      case 'b':
        if (!rx.isCurrentTuneFM()) //полоса только для AM !
        {
          if (bandwidthIdx > 6) bandwidthIdx = 0;//[0 1 2 3 4 5 6]
          rx.setBandwidth(bandwidthIdx, 1);
          Serial.print("AM Filter: ");
          Serial.print(bandwidth[bandwidthIdx]);
          Serial.println(" kHz");
          bandwidthIdx++;
        }
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