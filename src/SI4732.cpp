/*
  Simple example of using the PU2CLR SI4735 Arduino Library on ESP32C3.
  This sketch turns the ESP32C3 and the SI4735 into a small FM/AM receiver
  controlled via the Serial Monitor.

  Based on the SI4735_01_POC example of this library.
  By PU2CLR, adapted for ESP32C3, 2025.

  This code was genereted Codex (Via ChatGPT) at 2025-06-10.  
*/

#include <SI4735.h>
#include <Wire.h>
#include <patch_full.h>    // SSB patch for whole SSBRX full download


// Pin definitions for ESP32C3
#define RESET_PIN    2  // GPIO8 connected to RST pin of SI4735
#define ESP32_I2C_SDA 3 // GPIO4 connected to SDIO (pin 18)
#define ESP32_I2C_SCL 4 // GPIO5 connected to SCLK (pin 17)

#define AM_FUNCTION 1
#define FM_FUNCTION 0

extern int bandIdx;
extern void useBand();
extern String band_name();
extern String band_name_d();
extern void bandUp();
extern void bandDown();
extern void disp_refresh();
extern String disp1;
extern String disp2;
extern String disp3;
extern String disp4;

uint16_t currentFrequency;
uint16_t previousFrequency;
int currSNR=0;
int currRSSI=0;
int currVol=0;
uint8_t bandwidthIdx = 0;
const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};

SI4735 rx;


void showHelp()
{
  Serial.println("Commands: F=FM, A=AM, U/D=Freq +/-, S/s=Seek, +=Vol+, -=Vol-, B=BW, 0=Status, ?=Help");
  Serial.println("==================================================");
}

//*********************************************************************
//СТАТУС ПЕЧАТЬ
void showStatus()
{
  disp1=""; disp2=""; disp3=""; disp4=""; //для печати на дисплей
  previousFrequency = currentFrequency = rx.getFrequency();
  currVol = rx.getCurrentVolume();
  rx.getCurrentReceivedSignalQuality();
  currSNR = rx.getCurrentSNR();
  currRSSI = rx.getCurrentRSSI();
  Serial.print("Tuned to "+band_name()+" ");
  disp2=band_name_d()+"   ";
  if (rx.isCurrentTuneFM()) //если диапазон FM
  {
    Serial.print(String(currentFrequency / 100.0, 2));
    Serial.print(" MHz ");
    Serial.print((rx.getCurrentPilot()) ? "STEREO" : "MONO");
      disp1=String(currentFrequency / 100.0, 2)/*+" MHz "*/; //для дисплея
      disp2+="MHz";
  }
  else  //если диапазон AM
  {
    Serial.print(currentFrequency);
    Serial.print(" kHz");
      disp1=String(currentFrequency)/*+" kHz"*/; //для дисплея
      disp2+="kHz";
  }
  Serial.print(" [SNR:"); //отношение сигнал/шум
  Serial.print(String(currSNR));
  Serial.print("dB Signal:"); //уровень радиосигнала
  Serial.print(String(currRSSI));
  Serial.println("dBuV]");
  disp3 = "SNR: "+String(currSNR)+"/"+String(currRSSI);
  disp4 = "Vol: "+String(currVol);
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

//***************************************************************************
//ИНИЦИАЛИЗАЦИЯ
void radio_setup()
{
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  // setup I2C pins explicitly for ESP32C3
  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);

  Serial.println("SI4735 ESP32C3 Serial Monitor Demo");
  showHelp();

  // Look for the Si47XX I2C bus address
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {
    Serial.println("Si473X not found!");
    Serial.flush();
    while (1);
  } else {
    Serial.print("The SI473X I2C address is 0x");
    Serial.println(si4735Addr, HEX);
  }

  delay(500);
  rx.setup(RESET_PIN, FM_FUNCTION);

  bandIdx=0; //0-FM
  useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
  //rx.setFM(8400, 10800, 9860, 10);
  delay(500);
  currentFrequency = previousFrequency = rx.getFrequency();
  rx.setVolume(50);
  showStatus();

}

//***************************************************************************
//ТЕРМИНАЛ
void term_handle()
{
  if (Serial.available() > 0)
  {
    char key = Serial.read(); //читаем символ с клавиатуры
    //Serial.println("Key="+String(key));
    switch (key)
    {
      case 'q':
        disp_refresh();
        Serial.println("Test Display1..");
        //bandIdx=7; //41-AM
        //useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
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
        break;
      case '?':
        showHelp();
        break;
      default:
        break;
    }
  }

  delay(100);
  currentFrequency = rx.getCurrentFrequency(); //запрос текущей частоты
  if (currentFrequency != previousFrequency)
  {
    previousFrequency = currentFrequency;
    showStatus(); //печатаем статус , если было изменение частоты
    disp_refresh();
    delay(300);
  }
}

