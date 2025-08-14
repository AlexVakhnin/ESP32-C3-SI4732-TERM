/*
  Based on PU2CLR sources,
  adapted for ESP32C3, 2025.
*/

#include <SI4735.h>

//#define FM_BAND_TYPE 0
//#define MW_BAND_TYPE 1
//#define SW_BAND_TYPE 2
//#define LW_BAND_TYPE 3

#define FM 0    // для currentMode
#define AM 3
#define LSB 1
#define USB 2


extern SI4735 rx;
extern bool ssbLoaded;
//extern uint8_t disableAgc;
//extern uint8_t currentAGCAtt;

// Some variables to check the SI4735 status
extern uint16_t currentFrequency;
extern uint16_t previousFrequency;
extern uint8_t currentMode; //модуляция 1-LSB 2-USB
extern uint8_t bandwidthIdx; //полоса SW
extern uint8_t bwIdxSSB; //полоса SSB
extern uint8_t currentStep;
extern int currentBFO;

//структура массивов диапазонов
typedef struct
{
  const char *bandName; // Band description
  uint8_t bandType;     // Модуляция FM, AM
  uint16_t minimumFreq; // Minimum frequency of the band
  uint16_t maximumFreq; // maximum frequency of the band
  uint16_t currentFreq; // Default frequency or current frequency
  uint16_t currentStep; // Defeult step (increment and decrement)
} Band;

//массив для диапазонов FM, SW
Band band[] = {
  {"FM  ", FM, 8400, 10800, 10710, 10}, //УКВ
  {"AM  ", AM, 520, 1720, 1385, 5}, //СВ
  {"60m ", AM, 4500, 5500, 4850, 5},
  {"49m ", AM, 5600, 6300, 6000, 5},
  {"41m ", AM, 6800, 7800, 7295, 5}, // 40 meters
  {"31m ", AM, 9200, 10000, 9440, 5},
  {"25m ", AM, 11200, 12500, 12035, 5}, //25
  {"22m ", AM, 13400, 13900, 13650, 5}, //22
  {"19m ", AM, 15000, 15900, 15665, 5},
  {"18m ", AM, 17200, 17900, 17615, 5},
  {"CB  ", AM, 26200, 27900, 26960, 5} // CB band (11 meters)
};
const int lastBand = (sizeof band / sizeof(Band)) - 1; //количество в списке
int bandIdx = 0; //текущий индекс диапазона FM

//массив для диапазонов HAM SSB
Band band_ssb[] = {
  {"160m", LSB, 1800, 3500, 1810, 1}, // 160 meters
  {"80m ", LSB, 3500, 4500, 3500, 1}, // 80 meters
  {"40m ", LSB, 7000, 7500, 7097, 1}, // 40 meters
  {"30m ", USB, 10000, 11000, 10100, 1}, // 30 meters
  {"20m ", USB, 14000, 14500, 14152, 1}, // 20 meters
  {"17m ", USB, 18000, 18300, 18068, 1}, // 17 meters
  {"15m ", USB, 21000, 21900, 21000, 1}, // 15 mters
  {"12m ", USB, 24890, 26200, 24890, 1}, // 12 meters
  {"CB  ", USB, 26200, 27900, 27500, 1}, // CB band (11 meters)
  {"10m ", USB, 28000, 30000, 28000, 1} //10m
};
const int lastBand_ssb = (sizeof band_ssb / sizeof(Band)) - 1; //количество в списке
int bandIdx_ssb = 0; //текущий индекс диапазона SSB

//название диапазона для дисплея
String band_name_d(){
  if (ssbLoaded) return String(band_ssb[bandIdx_ssb].bandName);
  else return String(band[bandIdx].bandName);
}


//установить диапазон FM. SW
void useBand()
{
  if (band[bandIdx].bandType == FM) //для FM(УКВ) диапазона
  {
    rx.setTuneFrequencyAntennaCapacitor(0); //антенный аттенюатор - автоматически (рекоменд.!)
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
  }
  else // для SW(КВ) диаизонов
  {
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор (как в примере)
      rx.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);

      rx.setAutomaticGainControl(0, 0); //включаем AGC(АРУ)
      //rx.setAmSoftMuteMaxAttenuation(4); // глушит шум, когда нет сигнала... [0-32]
      bandwidthIdx = 1;
      rx.setBandwidth(bandwidthIdx, 1); //4 kHz, 1-шумодав включен
  }
  currentFrequency = band[bandIdx].currentFreq; //данные из массива в текущие переменные
  currentStep = band[bandIdx].currentStep;
  currentMode = band[bandIdx].bandType; //0-FM 3-AM
}

//установить диапазон SSB
void useBand_ssb() {
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор
      //rx.setTuneFrequencyAntennaCapacitor(0); //КВ - автоматически антенный аттенюатор (рекоменд.!)

      rx.setSSB(band_ssb[bandIdx_ssb].minimumFreq, band_ssb[bandIdx_ssb].maximumFreq, band_ssb[bandIdx_ssb].currentFreq, band_ssb[bandIdx_ssb].currentStep, band_ssb[bandIdx_ssb].bandType);

      rx.setSSBAutomaticVolumeControl(1); //звук - авто
      rx.setAutomaticGainControl(0, 0); //включаем AGC (само включает при изм.частоты..)
      //rx.setSsbSoftMuteMaxAttenuation(0); // не глушим шум, когда нет сигнала - для SSB
      bwIdxSSB = 2;
      rx.setSSBAudioBandwidth(bwIdxSSB); //установка полосы SSB 3 кГц
      currentBFO = 0;
      rx.setSSBBfo(currentBFO);
      currentFrequency = band_ssb[bandIdx_ssb].currentFreq; //данные из массива в текущие переменные
      currentStep = band_ssb[bandIdx_ssb].currentStep;
      currentMode = band_ssb[bandIdx_ssb].bandType; //1-LSB 2-USB
}

//переход по диапазонам вверх
void bandUp() {
  if(ssbLoaded){
    //band_ssb[bandIdx_ssb].currentFreq = currentFrequency; //сохнаним частоту старого диапазона SSB
    if (bandIdx_ssb < lastBand_ssb) { bandIdx_ssb++; } //выбираем новый диапазон SSB (вверх)
    else { bandIdx_ssb = 0; } //переключение по кругу
    useBand_ssb();
  } else {
    band[bandIdx].currentFreq = currentFrequency; //сохнаним частоту старого диапазона
    if (bandIdx < lastBand) { bandIdx++; } //выбираем новый диапазон (вверх)
    else { bandIdx = 0; } //переключение по кругу
    useBand();
  }
}

//переход по диапазонам вниз
void bandDown() {
  if(ssbLoaded){
    //band_ssb[bandIdx_ssb].currentFreq = currentFrequency; //сохнаним частоту старого диапазона SSB
    if (bandIdx_ssb > 0) { bandIdx_ssb--; } //выбираем новый диапазон SSB (вниз)
    else { bandIdx_ssb = lastBand_ssb; } //переключение по кругу
    useBand_ssb();
  } else {
    band[bandIdx].currentFreq = currentFrequency; //сохнаним частоту старого диапазона
    if (bandIdx > 0) { bandIdx--; } //выбираем новый диапазон (вниз)
    else { bandIdx = lastBand; } //переключение по кругу
    useBand();
  }
}

/*

//Update receiver settings after changing band and modulation
//from ATS-20 radio..
// !!!вызываем при изменении диапазона и модуляции!!!
{
    else //НЕ FM (SW, SSB)!!!
    {

        if (g_ssbLoaded) //ПРОШИВКА SSB ---------------------------------------------
        {
            g_currentBFO = 0; //сброс BFO при переходе на диапазон !!!
            if (extraSSBReset)
                loadSSBPatch(); //загрудаем патч, если патч разрушен ?????

            //Call this before to call crazy volume after AM when SVC is off
            //Выключаем AVC !!!!!!
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);

            g_si4735.setSSB(......... //переключаем диапазон SSB

            updateSSBCutoffFilter();
            //Теперь включаем AVC !!!!!!
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);
            g_si4735.setSSBDspAfc(g_Settings[SettingsIndex::Sync].param == 1 ? 0 : 1);
            g_si4735.setSSBAvcDivider(g_Settings[SettingsIndex::Sync].param == 0 ? 0 : 3); //Set Sync mode
            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_si4735.setSSBAudioBandwidth(g_currentMode == CW ? g_bandwidthSSB[0].idx : g_bandwidthSSB[g_bwIndexSSB].idx);
            updateBFO();
            g_si4735.setSSBSoftMute(g_Settings[SettingsIndex::SSM].param);
        }
        else //НЕ SSB -------------------------------------------------------------------------
        {
            g_si4735.setAM(.............. //переключаем диапазон SW

            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
        }
        // тут общие КВ и SSB
        g_si4735.setAvcAmMaxGain(g_Settings[SettingsIndex::AutoVolControl].param);
    }
*/
