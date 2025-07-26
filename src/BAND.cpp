#include <SI4735.h>
//#include <patch_full.h>    // SSB patch for whole SSBRX full download

//#define RESET_PIN 12
//#define ESP32_I2C_SDA 21
//#define ESP32_I2C_SCL 22

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define LSB 1
#define USB 2


extern SI4735 rx;
extern bool ssbLoaded;

// Some variables to check the SI4735 status
extern uint16_t currentFrequency;
extern uint16_t previousFrequency;
extern uint8_t currentMode; //модуляция 1-LSB 2-USB
uint8_t currentStep = 1;


//диапазоны
/*
   Band data structure
*/
typedef struct
{
  const char *bandName; // Band description
  uint8_t bandType;     // Band type (FM, MW or SW)
  uint16_t minimumFreq; // Minimum frequency of the band
  uint16_t maximumFreq; // maximum frequency of the band
  uint16_t currentFreq; // Default frequency or current frequency
  uint16_t currentStep; // Defeult step (increment and decrement)
} Band;

/*
   Band table
*/
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
  if (band[bandIdx].bandType == FM_BAND_TYPE) //если диапазон - 0 (FM)
  {
    rx.setTuneFrequencyAntennaCapacitor(0); //антенный аттенюатор - автоматически
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
  }
  else // для SW диаизона
  {
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор
      //rx.setTuneFrequencyAntennaCapacitor(0); //КВ - автоматически антенный аттенюатор (рекоменд.!)

      rx.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
      //rx.setAutomaticGainControl(1, 0); //AGC-automatic gain control = ВЫКЛ (так хуже..)
      rx.setAutomaticGainControl(0, 0); //AGC-automatic gain control = ВКЛ !
  }
  currentFrequency = band[bandIdx].currentFreq;
  currentStep = band[bandIdx].currentStep;
}

//установить диапазон SSB
void useBand_ssb() {
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор
      //rx.setTuneFrequencyAntennaCapacitor(0); //КВ - автоматически антенный аттенюатор (рекоменд.!)

      rx.setSSB(band_ssb[bandIdx_ssb].minimumFreq, band_ssb[bandIdx_ssb].maximumFreq, band_ssb[bandIdx_ssb].currentFreq, band_ssb[bandIdx_ssb].currentStep, band_ssb[bandIdx_ssb].bandType);
      rx.setSSBAutomaticVolumeControl(1);

      currentFrequency = band_ssb[bandIdx_ssb].currentFreq;
      currentStep = band_ssb[bandIdx_ssb].currentStep;
      currentMode = band_ssb[bandIdx_ssb].bandType; //1-LSB 2-USB
}

//переход по диапазонам
void bandUp() {
  if(ssbLoaded){
    if (bandIdx_ssb < lastBand_ssb) { bandIdx_ssb++; }
    else { bandIdx_ssb = 0; }
    useBand_ssb();
  } else {
    if (bandIdx < lastBand) { bandIdx++; }
    else { bandIdx = 0; }
    useBand();
  }
}
void bandDown() {
  if(ssbLoaded){
    if (bandIdx_ssb > 0) { bandIdx_ssb--; }
    else { bandIdx_ssb = lastBand_ssb; }
    useBand_ssb();
  } else {
    if (bandIdx > 0) { bandIdx--; }
    else { bandIdx = lastBand; }
    useBand();
  }
}
