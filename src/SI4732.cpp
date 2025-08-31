/*
  Based on PU2CLR sources,
  adapted for ESP32C3, 2025.
*/

#include <SI4735.h>
#include <Wire.h>
#include <patch_init.h>    // SSB patch for whole SSBRX full download
//#include <patch_full.h>

// Pin definitions for ESP32C3
#define RESET_PIN    2  // GPIO8 connected to RST pin of SI4735

#define AM_FUNCTION 1
#define FM_FUNCTION 0

extern int bandIdx;
extern void useBand();
extern int bandIdx_ssb;
extern void useBand_ssb();
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
int currentBFO = 0;
uint8_t currentBFOStep = 25;

//текущие параметры
uint16_t currentFrequency;
uint16_t previousFrequency;
uint8_t currSNR=0;
uint8_t currRSSI=0;
uint8_t agcGain = 0; //внутренний коэфф. усиления АРУ, его читаем
uint8_t gainParam = 5; //параметр AGC
uint8_t currVol=0;
uint8_t currentStep = 1;
//bool flagCB = false;

const char *bandwidth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};
uint8_t bandwidthIdx = 1; //4 kHz для SW
const char *bandwidthSSB[] = {"1.2", "2.2", "3.0", "4.0", "0.5", "1.0"};//полоса инф. для печати
uint8_t bwIdxSSB = 2; //3 kHz полоса для SSB

const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
uint8_t currentMode = 0; //модуляция 1-LSB 2-USB (для показа на дисплее)

SI4735 rx;

//ПОЛНОЕ ОБНОВЛЕНИЕ ДИСПЛЕЯ (disp1..disp4)
//вызывается при изменении частоты
void showStatus()
{
  currVol = rx.getCurrentVolume();
  rx.getCurrentReceivedSignalQuality(); //для получения SNR, RSSI
  currSNR = rx.getCurrentSNR();
  currRSSI = rx.getCurrentRSSI();
  rx.getAutomaticGainControl(); //для получения параметров AGC
  agcGain = rx.getAgcGainIndex();

  Serial.print("Band: "+band_name_d()+" ");
  disp2=band_name_d()+" "; //имя диапазона для дисплея

  if (rx.isCurrentTuneFM()) //если диапазон FM
  {
    Serial.print(String(currentFrequency / 100.0, 2)+" MHz ");
    disp1=String(currentFrequency / 100.0, 2)+" MHz"; //для дисплея
    if(rx.getCurrentPilot()){
      Serial.print("STEREO");
      disp2+="ST";
    } else {
      Serial.print("MONO");
    }
  }
  else  //если диапазон SW включая SSB
  {
    Serial.print(String(currentFrequency)+" kHz");
    disp1=String(currentFrequency)+" kHz"; //для дисплея
    disp2+=String(bandModeDesc[currentMode]); //добавляем индикацию mode
    disp2+=" "+String(currentStep);
  }

  //3-я строка..
  Serial.println(" [SNR:"+String(currSNR)+"/"+String(currRSSI)+"]"); //сигнал/шум -> терминал
  disp3 = "Q:"+String(currSNR)+"/"+String(currRSSI)+"/"+String(agcGain); //quality

  fill_menu_string(); //ОБНОВЛЕНИЕ НИЖНЕЙ СТРОКИ ДИСПЛЕЯ (МЕНЮ)
}

//ловим изменение частоты (событие для обновления дисплея)
//вызывается из LOOP()
void change_freq_handle(){
  currentFrequency = rx.getCurrentFrequency(); //запрос текущей частоты
  if (currentFrequency != previousFrequency) //ловим событие изменения
    {
      previousFrequency = currentFrequency;
      delay(10); //время для получения правильного SNR, AGC(для меню AGC) 
                        //т.к если делать в цикле, то идет помеха при обновлении дисплея..
      //if(ssbLoaded) currentAGCAtt=0; //т.к. SSB при изменении частоты сам включает AGC...
      showStatus(); //обновим дисплей полностью
      disp_refresh();
    }
}

//инициализация микросхемы радио
void radio_setup()
{
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
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
  rx.setup(RESET_PIN, FM_FUNCTION); //стартуем радиоприемник
  delay(100);
  bandIdx=0; //индекс диапазона 0-FM
  useBand(); //включить диапазон из списка -> rx.setFM(8400, 10800, 9860, 10);
  delay(60);
  currentFrequency = previousFrequency = rx.getFrequency();
  rx.setVolume(45);
  //rx.setBandwidth(bandwidthIdx, 1); //полоса 4 kHz
  showStatus(); //обновим дисплей полностью
  disp_refresh(); //обновить экран дисплея 
}

/*
   This function loads the contents of the ssb_patch_content array into the CI (Si4735)
   and starts the radio on SSB mode.
   Это делается каждый раз при переключении на SSB !
*/
void loadSSB()
{
  Serial.println("-->Switching to SSB..");
  disp4="Loading..";disp_refresh(); //на время загрузки висит надпись в нижней строке

  if(rx.isCurrentTuneFM()){ //БАГ при загрузке первый раз после FM..
    bandIdx = 1; //AM!!!
    useBand();
    delay(50);
  }

  rx.reset();
  //rx.queryLibraryId(); // Is it really necessary here?  Just powerDown() maigh work!
  rx.powerDown();
  rx.patchPowerUp();
  delay(50);
  rx.setI2CFastMode(); // Recommended
  rx.downloadPatch(ssb_patch_content, size_content);
  rx.setI2CStandardMode(); // goes back to default (100kHz)
  disp4="";disp_refresh();
  delay(50);
  // Parameters
  // 1-AUDIOBW - SSB Audio bandwidth; 0 = 1.2kHz (default); 1=2.2kHz; 2=3kHz; 3=4kHz; 4=500Hz; 5=1kHz;
  // 2-SBCUTFLT SSB - side band cutoff filter for band passand low pass filter ( 0 or 1)
  // 3-AVC_DIVIDER  - set 0 for SSB mode; set 3 for SYNC mode.
  // 4-AVCEN - SSB Automatic Volume Control (AVC) enable; 0=disable; 1=enable (default).
  // 5-SMUTESEL - SSB Soft-mute Based on RSSI or SNR (0 or 1).
  // 6-Automatic Frequency Control; 0=SYNC MODE, AFC enable; 1=SSB MODE, AFC disable.(АПЧ-ВЫКЛ)
  //g_si4735.setSSBConfig(g_bandwidthSSB[g_bwIndexSSB].idx, 1, 0, 1, 0, 1); //так в ats-20
  rx.setSSBConfig(bwIdxSSB, 1, 0, 1, 0, 1);
  delay(50);
  ssbLoaded = true; //флаг SSB
}

void bandwidth_am_up() {
        if (!rx.isCurrentTuneFM()) //полоса только для AM !
        {
          if (bandwidthIdx < 6) {bandwidthIdx++;}//[0 1 2 3 4 5 6]
          else {bandwidthIdx = 0;}
          rx.setBandwidth(bandwidthIdx, 1);
          Serial.println("BW: "+String(bandwidth[bandwidthIdx])+" kHz");
          fill_menu_string(); //обновить нижнюю строку дисплея (меню)
          disp_refresh();
        }
}
void bandwidth_am_down() {
        if (!rx.isCurrentTuneFM()) //полоса только для AM !
        {
          if (bandwidthIdx > 0) {bandwidthIdx--;}//[0 1 2 3 4 5 6]
          else {bandwidthIdx = 6;}
          rx.setBandwidth(bandwidthIdx, 1);
          Serial.println("BW: "+String(bandwidth[bandwidthIdx])+" kHz");
          fill_menu_string(); //обновить нижнюю строку дисплея (меню)
          disp_refresh();
        }
}
void bandwidth_ssb_up() {
      if (bwIdxSSB < 5) {bwIdxSSB++;}//[0 1 2 3 4 5]
      else {bwIdxSSB = 0;}
      rx.setSSBAudioBandwidth(bwIdxSSB); //установка полосы SSB
      Serial.println("BW SSB: "+String(bandwidthSSB[bwIdxSSB])+" kHz");
      fill_menu_string(); //обновить нижнюю строку дисплея (меню)
      disp_refresh();
}
void bandwidth_ssb_down() {
      if (bwIdxSSB > 0) {bwIdxSSB--;}//[0 1 2 3 4 5]
      else {bwIdxSSB = 5;}
      rx.setSSBAudioBandwidth(bwIdxSSB); //установка полосы SSB
      Serial.println("BW SSB: "+String(bandwidthSSB[bwIdxSSB])+" kHz");
      fill_menu_string(); //обновить нижнюю строку дисплея (меню)
      disp_refresh();
}
void bandwidth_up() { //распределитель BW-UP
  if(ssbLoaded){
    bandwidth_ssb_up();
  } else {
    bandwidth_am_up();
  }
}
void bandwidth_down() { //распределитель BW-DOWN
  if(ssbLoaded){
    bandwidth_ssb_down();
  } else {
    bandwidth_am_down();
  }
}

void volume_up() {
        rx.volumeUp(); //звук+
        currVol = rx.getCurrentVolume();
        Serial.println("Vol[0-63]="+String(currVol));
        showStatus(); //обновить весь экран дисплея
        disp_refresh();
}
void volume_down() {
        rx.volumeDown();
        currVol = rx.getCurrentVolume();
        Serial.println("Vol[0-63]="+String(currVol));
        showStatus(); //обновить весь экран дисплея
        disp_refresh();
}

void bfo_up(){
  if(ssbLoaded){
    currentBFO = currentBFO + currentBFOStep;
    rx.setSSBBfo(currentBFO);
    fill_menu_string(); //обновить нижнюю строку дисплея (меню)
    disp_refresh();
  }
}
void bfo_down(){
  if(ssbLoaded){
    currentBFO = currentBFO - currentBFOStep;
    rx.setSSBBfo(currentBFO);
    fill_menu_string(); //обновить нижнюю строку дисплея (меню)
    disp_refresh();
  }
}

void ssb_on(){
  loadSSB(); //грузим SSB прошивку!
  bandIdx_ssb=2; //индекс диапазона 0->40m
  useBand_ssb(); //включить диапазон SSB
  showStatus(); //обновить весь экран дисплея
  disp_refresh();
}
void ssb_off(){
  rx.reset(); //сброс на стандартную прошивку
  ssbLoaded = false;
  currentBFO = 0; //в меню что бы был 0
  bandIdx=0; //индекс диапазона 0->FM
  useBand(); //включить диапазон из списка
  delay(10);
  currentFrequency = previousFrequency = rx.getFrequency();
  currVol=45;
  rx.setVolume(currVol);
  bandwidthIdx = 1;
  rx.setBandwidth(bandwidthIdx, 1); //полоса 4 kHz
  showStatus(); //обновить весь экран дисплея
  disp_refresh();
}
//параметр для AGC
void agc_param_up(){
  if(gainParam<40) gainParam+=5; //увеличение с ограничением до 40
  Serial.println("param_up(), gainParam ="+String(gainParam)); //DEBUG
  if(gainParam==40) {
    rx.setAutomaticGainControl(0, gainParam);
    rx.setTuneFrequencyAntennaCapacitor(0); //входной аттерюатор - АВТО
  } else {
    rx.setAutomaticGainControl(0, gainParam); //включаем с коеффициентом 0-40
  }
  showStatus(); //обновить весь экран дисплея
  disp_refresh();
}
void agc_param_down(){
  if(gainParam>0) gainParam-=5; //уменьшение с ограничением до 0
  Serial.println("param_up(), gainParam ="+String(gainParam)); //DEBUG
  if(gainParam==0) {
    rx.setAutomaticGainControl(1, gainParam); //выключаем
  } else {
    rx.setAutomaticGainControl(0, gainParam); //включаем с коеффициентом 0-40
    rx.setTuneFrequencyAntennaCapacitor(1); //для КВ
  }
  showStatus(); //обновить весь экран дисплея
  disp_refresh();
}

void agc_on(){
  rx.setAutomaticGainControl(0, 25); //включаем AGC
  showStatus(); //обновить весь экран дисплея
  disp_refresh();
}
//Нет смысла отключать AGC(SSB его включает, на КВ без него хуже..)
void agc_off(){
  rx.setAutomaticGainControl(1, 0); //выключаем AGC
  showStatus(); //обновить весь экран дисплея
  disp_refresh();
}

//меням шаг 1/5
void change_step(){ //меняем (1/5 kHz) для AM, SSB
  if (!rx.isCurrentTuneFM()) { //для всех кроме FM
    if (currentStep==1) currentStep=5;
    else if (currentStep==5) currentStep=1;
    rx.setFrequencyStep(currentStep);
    showStatus(); //обновить весь экран дисплея
    disp_refresh();
    //Serial.println("change_step.currentStep="+String(currentStep));
  }
}