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
//#define ESP32_I2C_SDA 3
//#define ESP32_I2C_SCL 4
//#define TERM_PERIOD 100  //150 ms

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
void change_freq_handle();

//unsigned long termCurrentTime = 0;  //для вычисления интервалов опроса
//unsigned long termLastTime = 0;

uint16_t currentFrequency;
uint16_t previousFrequency;
int currSNR=0;
int currRSSI=0;
int currVol=0;
uint8_t bandwidthIdx = 0;
const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};

SI4735 rx;

//обновляем информацию о состоянии радио, заполняем disp1..disp4 для дисплея
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


//инициализация микросхемы радио
void radio_setup()
{
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  //Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL); //делается в DISP.cpp !!!

  Serial.println("SI4735 ESP32C3 Serial Monitor Demo");
  //showHelp();

  // Look for the Si47XX I2C bus address
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {
    Serial.println("Si473X not found!");
    Serial.flush();
    while (1); //заглушка, если не нашли адрес SI4732
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
  //showStatus();
  delay(300);
  showStatus();
  disp_refresh(); //обновить экран дисплея 
}

//ловим изменение частоты - событие для обновления дисплея..
void change_freq_handle(){
  currentFrequency = rx.getCurrentFrequency(); //запрос текущей частоты
  if (currentFrequency != previousFrequency)
    {
      previousFrequency = currentFrequency;
      delay(300); //время для получения правильного SNR
      showStatus(); //печатаем статус , если было изменение частоты
      disp_refresh();
    }
}
