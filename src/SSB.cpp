#include <SI4735.h>
#include <patch_full.h>    // SSB patch for whole SSBRX full download

#define RESET_PIN 12
#define ESP32_I2C_SDA 21
#define ESP32_I2C_SCL 22

#define FM_BAND_TYPE 0
#define MW_BAND_TYPE 1
#define SW_BAND_TYPE 2
#define LW_BAND_TYPE 3

#define FM 0
#define LSB 1
#define USB 2
#define AM 3
#define LW 4
#define SSB 1
#define DEFAULT_VOLUME 50 // change it for your favorite sound volume

extern SI4735 rx;

const uint16_t size_content = sizeof ssb_patch_content; // see ssb_patch_content in patch_full.h or patch_init.h

bool ssbLoaded = false; //флаг SSB
bool bfoOn = false;
bool disableAgc = true;
bool fmStereo = true;
bool touch = false;

// Some variables to check the SI4735 status
extern uint16_t currentFrequency;
extern uint16_t previousFrequency;
uint8_t currentStep = 1;
uint8_t currentBFOStep = 25;
uint8_t volume = DEFAULT_VOLUME;

const char *bandModeDesc[] = {"FM ", "LSB", "USB", "AM "};
uint8_t currentMode = FM; //модуляция
const char *bandwidthSSB[] = {"1.2", "2.2", "3.0", "4.0", "0.5", "1.0"};//полоса инф. для печати
uint8_t bwIdxSSB = 2; //полоса пропускания сигнала


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
  {"FM  ", FM_BAND_TYPE, 8400, 10800, 10390, 10},
  {"LW  ", LW_BAND_TYPE, 100, 510, 300, 1},
  {"AM  ", MW_BAND_TYPE, 520, 1720, 810, 10},
  {"160m", SW_BAND_TYPE, 1800, 3500, 1900, 1}, // 160 meters
  {"80m ", SW_BAND_TYPE, 3500, 4500, 3700, 1}, // 80 meters
  {"60m ", SW_BAND_TYPE, 4500, 5500, 4850, 5},
  {"49m ", SW_BAND_TYPE, 5600, 6300, 6000, 5},
  {"41m ", SW_BAND_TYPE, 6800, 7800, 7100, 5}, // 40 meters
  {"31m ", SW_BAND_TYPE, 9200, 10000, 9600, 5},
  {"30m ", SW_BAND_TYPE, 10000, 11000, 10100, 1}, // 30 meters
  {"25m ", SW_BAND_TYPE, 11200, 12500, 11940, 5},
  {"22m ", SW_BAND_TYPE, 13400, 13900, 13600, 5},
  {"20m ", SW_BAND_TYPE, 14000, 14500, 14200, 1}, // 20 meters
  {"19m ", SW_BAND_TYPE, 15000, 15900, 15300, 5},
  {"18m ", SW_BAND_TYPE, 17200, 17900, 17600, 5},
  {"17m ", SW_BAND_TYPE, 18000, 18300, 18100, 1},  // 17 meters
  {"15m ", SW_BAND_TYPE, 21000, 21900, 21200, 1},  // 15 mters
  {"12m ", SW_BAND_TYPE, 24890, 26200, 24940, 1},  // 12 meters
  {"CB  ", SW_BAND_TYPE, 26200, 27900, 27500, 1},  // CB band (11 meters)
  {"10m ", SW_BAND_TYPE, 28000, 30000, 28400, 1}
};

const int lastBand = (sizeof band / sizeof(Band)) - 1;
int bandIdx = 0;


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



/*
   Switch the radio to current band
*/
void useBand()
{
  //cleanBfoRdsInfo();
  if (band[bandIdx].bandType == FM_BAND_TYPE) //начальная - 0 (FM)
  {
    currentMode = FM;
    rx.setTuneFrequencyAntennaCapacitor(0); //антенный аттенюатор - автоматически
    rx.setFM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
    bfoOn = ssbLoaded = false;
  }
  else //не FM
  {
    if (band[bandIdx].bandType == MW_BAND_TYPE || band[bandIdx].bandType == LW_BAND_TYPE)
      rx.setTuneFrequencyAntennaCapacitor(0); //СВ, ДВ - автомат
    else
      rx.setTuneFrequencyAntennaCapacitor(1); //КВ - ручной антенный аттенюатор

    if (ssbLoaded)
    {
      rx.setSSB(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep, currentMode);
      rx.setSSBAutomaticVolumeControl(1);
    }
    else
    {
      currentMode = AM;
      rx.reset();
      rx.setAM(band[bandIdx].minimumFreq, band[bandIdx].maximumFreq, band[bandIdx].currentFreq, band[bandIdx].currentStep);
      rx.setAutomaticGainControl(1, 0);
      bfoOn = false;
    }

  }

  currentFrequency = band[bandIdx].currentFreq;
  currentStep = band[bandIdx].currentStep;
  //showStatus(); //обновление всей информации на дисплее..
}

/*
   Goes to the next band (see Band table)
*/
void bandUp()
{
  // save the current frequency for the band
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStep = currentStep;

  if (bandIdx < lastBand)
  {
    bandIdx++;
  }
  else
  {
    bandIdx = 0;
  }
  useBand();
}

/*
   Goes to the previous band (see Band table)
*/
void bandDown()
{
  // save the current frequency for the band
  band[bandIdx].currentFreq = currentFrequency;
  band[bandIdx].currentStep = currentStep;
  if (bandIdx > 0)
  {
    bandIdx--;
  }
  else
  {
    bandIdx = lastBand;
  }
  useBand();
}


void radio_ssb_setup()
{

  //Serial.begin(9600);
  //while (!Serial);

  // Encoder pins
  //pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  //pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  //display.init();

  //delay(500);

  // Splash - Change it for your introduction text.
  /*
  display.backlight();
  display.setCursor(3, 0);
  display.print("SI4735 on ESP32");
  display.setCursor(2, 1);
  display.print("Arduino Library");
  delay(500);
  display.setCursor(1, 2);
  display.print("All in One Radio");
  delay(500);
  display.setCursor(4, 3);
  display.print("By PU2CLR");
  delay(2000);
  // end Splash
  display.clear();

  // Encoder interrupt
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), rotaryEncoder, CHANGE);
*/
  // The line below may be necessary to setup I2C pins on ESP32
  Wire.begin(ESP32_I2C_SDA, ESP32_I2C_SCL);
  
  rx.setup(RESET_PIN, 1); //0-FM, 1-AM

  // Set up the radio for the current band (see index table variable bandIdx )
  useBand();
  currentFrequency = previousFrequency = rx.getFrequency();

  rx.setVolume(volume);
  //display.clear();
  //showStatus();
}

