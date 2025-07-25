/*
  Based on the SI4735_01_POC example by PU2CLR,
  adapted for ESP32C3, 2025.
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
extern void fill_menu_string();
void change_freq_handle();

//касаемо SSB
const uint16_t size_content = sizeof ssb_patch_content; // see ssb_patch_content in patch_full.h or patch_init.h
bool ssbLoaded = false; //флаг SSB
bool bfoOn = false;
bool disableAgc = true;
const char *bandwidthSSB[] = {"1.2", "2.2", "3.0", "4.0", "0.5", "1.0"};//полоса инф. для печати
uint8_t bwIdxSSB = 2; //полоса пропускания сигнала

//текущие параметры
uint16_t currentFrequency;
uint16_t previousFrequency;
int currSNR=0;
int currRSSI=0;
int currVol=0;
const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};
uint8_t bandwidthIdx = 1; //4 kHz для SW


const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
uint8_t currentMode = 0; //модуляция

SI4735 rx;

//обновляем информацию о состоянии радио, заполняем disp1..disp3 для дисплея
//вызывается при изменении частоты
void showStatus()
{
  disp1=""; disp2=""; disp3=""; //для печати на дисплей
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
  fill_menu_string();
}

//инициализация микросхемы радио
void radio_setup()
{
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  //Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL); //делается в DISP.cpp !!!

  Serial.println("SI4735 ESP32C3 Serial Monitor Demo");

  //Автоматически определяем адрес радио i2c
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

  bandIdx=0; //индекс диапазона 0-FM
  useBand(); //включить диапазон из списка согласно согласно номеру: bandIdx
  //rx.setFM(8400, 10800, 9860, 10);
  delay(500);
  currentFrequency = previousFrequency = rx.getFrequency();
  rx.setVolume(45);
  rx.setBandwidth(bandwidthIdx, 1); //плдлса 4 kHz
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
      delay(50/*300*/); //время для получения правильного SNR, 
                        //т.к в цикле идет помеха при обновлении дисплея..
      showStatus(); //печатаем статус , если было изменение частоты
      disp_refresh();
    }
}

/*
   This function loads the contents of the ssb_patch_content array into the CI (Si4735)
   and starts the radio on SSB mode.
   Это делается каждый раз при переключении на SSB !
*/
void loadSSB()
{
  //display.setCursor(0, 2);
  Serial.println("-->Switching to SSB..");

  rx.reset();
  rx.queryLibraryId(); // Is it really necessary here? I will check it.
  rx.patchPowerUp();
  delay(50);
  rx.setI2CFastMode(); // Recommended
  // si4735.setI2CFastModeCustom(500000); // It is a test and may crash.
  rx.downloadPatch(ssb_patch_content, size_content);
  rx.setI2CStandardMode(); // goes back to default (100kHz)
  //cleanBfoRdsInfo(); //очищаем строку дисплея

  // delay(50);
  // Parameters
  // AUDIOBW - SSB Audio bandwidth; 0 = 1.2kHz (default); 1=2.2kHz; 2=3kHz; 3=4kHz; 4=500Hz; 5=1kHz;
  // SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
  // AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
  // AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
  // SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
  // DSP_AFCDIS - DSP AFC Disable or enable; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.
  rx.setSSBConfig(bwIdxSSB, 1, 0, 0, 0, 1);
  delay(25);
  ssbLoaded = true;
  //display.clear();
}



void bandwidth_up() {
        if (!rx.isCurrentTuneFM()) //полоса только для AM !
        {
          if (bandwidthIdx < 6) {bandwidthIdx++;}//[0 1 2 3 4 5 6]
          else {bandwidthIdx = 0;}
          rx.setBandwidth(bandwidthIdx, 1);
          Serial.print("AM Filter: ");
          Serial.print(bandwidth[bandwidthIdx]);
          Serial.println(" kHz");         
          disp4 = "BW: "+String(bandwidth[bandwidthIdx])+"kHz";
          disp_refresh();
        }
}
void bandwidth_down() {
          if (!rx.isCurrentTuneFM()) //полоса только для AM !
        {
          if (bandwidthIdx > 0) {bandwidthIdx--;}//[0 1 2 3 4 5 6]
          else {bandwidthIdx = 6;}
          rx.setBandwidth(bandwidthIdx, 1);
          Serial.print("AM Filter: ");
          Serial.print(bandwidth[bandwidthIdx]);
          Serial.println(" kHz");         
          disp4 = "BW: "+String(bandwidth[bandwidthIdx])+"kHz";
          disp_refresh();
        }
}
void volume_up() {
        rx.volumeUp(); //звук+
        currVol = rx.getCurrentVolume();
        Serial.println("Vol[0-63]="+String(currVol));
        disp4 = "Vol: "+String(currVol);
        disp_refresh();
}
void volume_down() {
        rx.volumeDown();
        currVol = rx.getCurrentVolume();
        Serial.println("Vol[0-63]="+String(currVol));
        disp4 = "Vol: "+String(currVol);
        disp_refresh();
}
void ssb_on(){
  //...
  ssbLoaded = true;
  disp4 = "SSB: on";
  disp_refresh();
}
void ssb_off(){
  //...
  ssbLoaded = false;
  disp4 = "SSB: off";
  disp_refresh();
}
