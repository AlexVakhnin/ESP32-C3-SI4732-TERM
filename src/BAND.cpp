/*
  Based on PU2CLR sources,
  adapted for ESP32C3, 2025.
*/

#include <SI4735.h>

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define FM 0    // для currentMode
#define AM 3
#define LSB 1
#define USB 2


extern SI4735 rx;
extern bool ssbLoaded;
//extern uint8_t disableAgc;
extern uint8_t currentAGCAtt;

// Some variables to check the SI4735 status
extern uint16_t currentFrequency;
extern uint16_t previousFrequency;
extern uint8_t currentMode; //модуляция 1-LSB 2-USB
extern uint8_t bandwidthIdx;
uint8_t currentStep = 1;


//структура массивов диапазонов
typedef struct
{
  const char *bandName; // Band description
  uint8_t bandType;     // Band type (FM, MW or SW)
  uint16_t minimumFreq; // Minimum frequency of the band
  uint16_t maximumFreq; // maximum frequency of the band
  uint16_t currentFreq; // Default frequency or current frequency
  uint16_t currentStep; // Defeult step (increment and decrement)
} Band;

//массив для диапазонов FM, SW
Band band[] = {
  {"FM  ", FM_BAND_TYPE, 8400, 10800, 10710, 10}, //УКВ
  {"AM  ", MW_BAND_TYPE, 520, 1720, 1385, 5}, //СВ
  {"60m ", SW_BAND_TYPE, 4500, 5500, 4850, 5},
  {"49m ", SW_BAND_TYPE, 5600, 6300, 6000, 5},
  {"41m ", SW_BAND_TYPE, 6800, 7800, 7295, 5}, // 40 meters
  {"31m ", SW_BAND_TYPE, 9200, 10000, 9440, 5},
  {"25m ", SW_BAND_TYPE, 11200, 12500, 12035, 5}, //25
  {"22m ", SW_BAND_TYPE, 13400, 13900, 13650, 5}, //22
  {"19m ", SW_BAND_TYPE, 15000, 15900, 15665, 5},
  {"18m ", SW_BAND_TYPE, 17200, 17900, 17615, 5}
};
const int lastBand = (sizeof band / sizeof(Band)) - 1; //количество в списке
int bandIdx = 0; //текущий индекс диапазона FM

//массив для диапазонов HAM SSB
Band band_ssb[] = {
  {"160m", LSB, 1800, 3500, 1810, 1}, // 160 meters
  {"80m ", LSB, 3500, 4500, 3500, 1}, // 80 meters
  {"40m ", LSB, 7000, 7500, 7000, 1}, // 40 meters
  {"30m ", USB, 10000, 11000, 10100, 1}, // 30 meters
  {"20m ", USB, 14000, 14500, 14000, 1}, // 20 meters
  {"17m ", USB, 18000, 18300, 18068, 1}, // 17 meters
  {"15m ", USB, 21000, 21900, 21000, 1}, // 15 mters
  {"12m ", USB, 24890, 26200, 24890, 1}, // 12 meters
  {"CB  ", USB, 26200, 27900, 27500, 1}, // CB band (11 meters)
  {"10m ", USB, 28000, 30000, 28000, 1} //10m
};
const int lastBand_ssb = (sizeof band_ssb / sizeof(Band)) - 1; //количество в списке
int bandIdx_ssb = 0; //текущий индекс диапазона SSB

//название диапазона для терминала
String band_name(){
  String s="";
  s+=String(band[bandIdx].bandName);
  s+="["+String(band[bandIdx].minimumFreq)+"-";
  s+=String(band[bandIdx].maximumFreq)+"]";
  return s;
}

//название диапазона для дисплея
String band_name_d(){
  if (ssbLoaded) return String(band_ssb[bandIdx_ssb].bandName);
  else return String(band[bandIdx].bandName);
}


//установить диапазон FM. SW
void useBand()
{
  if (band[bandIdx].bandType == FM_BAND_TYPE) //для FM(УКВ) диапазона
  {
    currentMode = FM;
    rx.setTuneFrequencyAntennaCapacitor(0); //антенный аттенюатор - автоматически
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
  }
  else // для SW(КВ) диаизонов
  {
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор (как в примере)
      //rx.setTuneFrequencyAntennaCapacitor(0); //КВ - автоматически антенный аттенюатор (рекоменд.!)
      rx.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);

      //это настройка AGC(АРУ) - большое влияние на качество сигнала на КВ !!!
      currentAGCAtt = 0; //max gain
      rx.setAutomaticGainControl(0, 0); //включаем AGC
      bandwidthIdx = 1; //4 kHz для SW
      rx.setBandwidth(bandwidthIdx, 1); //4 kHz, 1-шумодав включен
      currentMode = AM;
  }
  currentFrequency = band[bandIdx].currentFreq;
  currentStep = band[bandIdx].currentStep;
  //Serial.println("useBand(), urrentAGCAtt ="+String(currentAGCAtt)); //DEBUG
}

//установить диапазон SSB
void useBand_ssb() {
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор
      //rx.setTuneFrequencyAntennaCapacitor(0); //КВ - автоматически антенный аттенюатор (рекоменд.!)

      rx.setSSB(band_ssb[bandIdx_ssb].minimumFreq, band_ssb[bandIdx_ssb].maximumFreq, band_ssb[bandIdx_ssb].currentFreq, band_ssb[bandIdx_ssb].currentStep, band_ssb[bandIdx_ssb].bandType);
      rx.setSSBAutomaticVolumeControl(1); //уст. при инициплизации в loadSSB() !!!!!

      currentFrequency = band_ssb[bandIdx_ssb].currentFreq;
      currentStep = band_ssb[bandIdx_ssb].currentStep;
      currentMode = band_ssb[bandIdx_ssb].bandType; //1-LSB 2-USB
}

//переход по диапазонам вверх
void bandUp() {
  if(ssbLoaded){
    band_ssb[bandIdx_ssb].currentFreq = currentFrequency; //сохнаним частоту диапазона SSB
    if (bandIdx_ssb < lastBand_ssb) { bandIdx_ssb++; } //вращаем вверх диапазон SSB
    else { bandIdx_ssb = 0; }
    useBand_ssb();
  } else {
    band[bandIdx].currentFreq = currentFrequency; //сохнаним частоту диапазона
    if (bandIdx < lastBand) { bandIdx++; } //вращаем вверх диапазон
    else { bandIdx = 0; }
    useBand();
  }
}

//переход по диапазонам вниз
void bandDown() {
  if(ssbLoaded){
    band_ssb[bandIdx_ssb].currentFreq = currentFrequency; //сохнаним частоту диапазона SSB
    if (bandIdx_ssb > 0) { bandIdx_ssb--; } //вращаем вниз диапазон SSB
    else { bandIdx_ssb = lastBand_ssb; }
    useBand_ssb();
  } else {
    band[bandIdx].currentFreq = currentFrequency; //сохнаним частоту диапазона
    if (bandIdx > 0) { bandIdx--; }//вращаем вниз диапазон
    else { bandIdx = lastBand; }
    useBand();
  }
}

/*

//Update receiver settings after changing band and modulation
//fron ATS-20 radio..
void applyBandConfiguration(bool extraSSBReset = false)
{
    g_si4735.setTuneFrequencyAntennaCapacitor(uint16_t(g_bandIndex == SW_BAND_TYPE));
    if (g_bandIndex == FM_BAND_TYPE)
    {
        g_currentMode = FM;
        g_si4735.setFM(g_bandList[g_bandIndex].minimumFreq,
            g_bandList[g_bandIndex].maximumFreq,
            g_bandList[g_bandIndex].currentFreq,
            g_tabStepFM[g_bandList[g_bandIndex].currentStepIdx]);
        g_si4735.setSeekFmLimits(g_bandList[g_bandIndex].minimumFreq, g_bandList[g_bandIndex].maximumFreq);
        g_si4735.setSeekFmSpacing(1);
        g_ssbLoaded = false;
#if USE_RDS
        setRDSConfig(g_Settings[SettingsIndex::RDSError].param);
#endif
        g_si4735.setFifoCount(1);
        g_bwIndexFM = g_bandList[g_bandIndex].bandwidthIdx;
        g_si4735.setFmBandwidth(g_bwIndexFM);
        g_si4735.setFMDeEmphasis(g_Settings[SettingsIndex::DeEmp].param == 0 ? 1 : 2);
    }
    else //НЕ FM !!!
    {
        uint16_t minFreq = g_bandList[g_bandIndex].minimumFreq;
        uint16_t maxFreq = g_bandList[g_bandIndex].maximumFreq;
        if (g_bandIndex == SW_BAND_TYPE)
        {
            minFreq = SW_LIMIT_LOW;
            maxFreq = SW_LIMIT_HIGH;
        }

        if (g_ssbLoaded) //ПРОШИВКА SSB ---------------------------------------------
        {
            g_currentBFO = 0; //сброс BFO при переходе на диапазон !!!
            if (extraSSBReset)
                loadSSBPatch();

            //Call this before to call crazy volume after AM when SVC is off
            g_si4735.setSSBAutomaticVolumeControl(g_Settings[SettingsIndex::SVC].param);

            g_si4735.setSSB(minFreq,
                maxFreq,
                g_bandList[g_bandIndex].currentFreq,
                g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps ? 0 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx],
                g_currentMode == CW ? g_Settings[SettingsIndex::CWSwitch].param + 1 : g_currentMode);

            updateSSBCutoffFilter();
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
            g_currentMode = AM;
            g_si4735.setAM(minFreq,
                maxFreq,
                g_bandList[g_bandIndex].currentFreq,
                g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps ? 0 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx]);
            g_si4735.setAmSoftMuteMaxAttenuation(g_Settings[SettingsIndex::SoftMute].param);
            g_bwIndexAM = g_bandList[g_bandIndex].bandwidthIdx;
            g_si4735.setBandwidth(g_bandwidthAM[g_bwIndexAM].idx, 1);
        }

        agcSetFunc();
        g_si4735.setAvcAmMaxGain(g_Settings[SettingsIndex::AutoVolControl].param);
        g_si4735.setSeekAmLimits(minFreq, maxFreq);
        g_si4735.setSeekAmSpacing((g_bandList[g_bandIndex].currentStepIdx >= g_amTotalSteps) ? 1 : g_tabStep[g_bandList[g_bandIndex].currentStepIdx]);
    }

    g_currentFrequency = g_bandList[g_bandIndex].currentFreq;
    if (g_currentMode == FM)
        g_FMStepIndex = g_bandList[g_bandIndex].currentStepIdx;
    else
        g_stepIndex = g_bandList[g_bandIndex].currentStepIdx;

    if ((g_bandIndex == LW_BAND_TYPE || g_bandIndex == MW_BAND_TYPE)
        && g_stepIndex > g_amTotalStepsSSB)
        g_stepIndex = g_amTotalStepsSSB;

    if (!g_settingsActive)
        showStatus(true);
    resetEepromDelay();
}

*/