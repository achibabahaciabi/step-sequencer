#include <Arduino.h>
#include <BluetoothA2DPSource.h>

#include <U8g2lib.h>
#include <Wire.h>

#include "kick.h"
#include "snare.h"
#include "hihat.h"


#include <SPI.h>

#include "SDCard.h"
#include "Tus.h"
#include "RotaryEncoder.h"


// --- Keypad Setup
Tus tus;
// --- Rotary Encoder Pins
RotaryEncoder encoder; // uses default pins: 32, 33, 25




enum UIState { NAVIGATE, BPM_EDIT, SUB_SD_KICK ,SUB_SD_SNARE,SUB_SD_HIHAT,VOL_EDIT};
UIState uiState = NAVIGATE;

// --- Instrument selection
enum Instrument { KICK, SNARE, HIHAT };
Instrument currentInstrument = KICK;

// --- Menu Items
enum MenuItem { MENU_KICK, MENU_SNARE, MENU_HIHAT, MENU_BPM, MENU_VOLUME, MENU_COUNT };

MenuItem rotatedItem = MENU_KICK;





// ---kick types
enum KickType { STANDARD_KICK, SD_DATA_SAMPLE_KICK };
KickType currentKick = STANDARD_KICK;

// ---snare types
enum SnareType { STANDARD_SNARE, SD_DATA_SAMPLE_SNARE };
SnareType currentSnare = STANDARD_SNARE;

// --hihat types
enum HihatType { STANDARD_HIHAT, SD_DATA_SAMPLE__HIHAT };
HihatType currentHihat = STANDARD_HIHAT;



enum VolumeEditState { VOLUME_SELECT, VOLUME_ADJUST };
VolumeEditState volumeEditState = VOLUME_SELECT;

// Add this enum
enum VolumeItem { VOL_KICK, VOL_SNARE, VOL_HIHAT, VOL_COUNT };
VolumeItem volItem = VOL_KICK; // current selection while in VOL_EDIT




SDCard sd;
// buffers for samples taken from sd card 
#define SAMPLE_SIZE (13 * 1024 / 2)  // number of int16 samples, as before

int sampleLength1_sd_kick = 0;
static int16_t buffer1_sd_kick[SAMPLE_SIZE];  // static preallocated buffer
int index_sd_kick = 0;

int sampleLength2_sd_snare = 0;
static int16_t buffer2_sd_snare [SAMPLE_SIZE]; 
int index_sd_snare = 0;

int sampleLength3_sd_hihat= 0;
static int16_t buffer3_sd_hihat [SAMPLE_SIZE]; 
int index_sd_hihat = 0;










// --- OLED I2C
#define OLED_SDA 4
#define OLED_SCL 15
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);







float kickVolume = 1.0f;
float snareVolume = 1.0f;
float hihatVolume = 1.0f;


// --- Bluetooth A2DP Setup
BluetoothA2DPSource a2dp_source;
const int sampleRate = 44100;
float bpm = 60.0f;
int step_duration_samples = (int)((60.0f / bpm / 4.0f) * sampleRate);

// --- Sequencer Setup
const int NUM_STEPS = 16;
bool kick_steps[NUM_STEPS] = {false};
bool snare_steps[NUM_STEPS] = {false};
bool hihat_steps[NUM_STEPS] = {false};
int current_step = 0;
size_t sample_in_step = 0;

// --- Playback state
bool playing_kick = false, playing_snare = false, playing_hihat = false;
size_t kick_sample_index = 0, snare_sample_index = 0, hihat_sample_index = 0;








// --- Audio Callback: Provide samples to A2DP ---
int32_t audio_data_callback(uint8_t *data, int32_t len) {
  int16_t *buffer = (int16_t *)data;
  int samples = len / 2; // 2 bytes per sample (16-bit mono)

  for (int i = 0; i < samples; i++) {
    if (sample_in_step == 0) {
      if (kick_steps[current_step]) { playing_kick = true; kick_sample_index = 0; }
      if (snare_steps[current_step]) { playing_snare = true; snare_sample_index = 0; }
      if (hihat_steps[current_step]) { playing_hihat = true; hihat_sample_index = 0; }
    }

    int32_t sample = 0;

    if (playing_kick) {

    
  if (currentKick == STANDARD_KICK && kick_sample_index < kick_data_len) {
    sample += (int32_t)(kick_data[kick_sample_index++]*kickVolume);
    //-->check
  } else if (currentKick == SD_DATA_SAMPLE_KICK && kick_sample_index < sampleLength1_sd_kick) {
    sample += (int32_t)(buffer1_sd_kick[kick_sample_index++]*kickVolume);
  } else playing_kick = false;

    }

    if (playing_snare) {
  if (currentSnare == STANDARD_SNARE && snare_sample_index < snare_data_len) {
    sample += (int32_t)(snare_data[snare_sample_index++]*snareVolume);
  } else if (currentSnare == SD_DATA_SAMPLE_SNARE && snare_sample_index < sampleLength2_sd_snare) {
    sample += (int32_t)(buffer2_sd_snare[snare_sample_index++]*snareVolume);
  } else {
    playing_snare = false;
  }
}


    if (playing_hihat) {
  if (currentHihat == STANDARD_HIHAT && hihat_sample_index < hihat_data_len) {
    sample += (int32_t)(hihat_data[hihat_sample_index++]*hihatVolume);
  } else if (currentHihat == SD_DATA_SAMPLE__HIHAT && hihat_sample_index < sampleLength3_sd_hihat) {
    sample += (int32_t)(buffer3_sd_hihat[hihat_sample_index++]*hihatVolume);
  } else {
    playing_hihat = false;
  }
}


    // Clamp to int16 range
    sample = constrain(sample, -32768, 32767);
    buffer[i] = (int16_t)sample;

    sample_in_step++;
    if (sample_in_step >= step_duration_samples) {
      sample_in_step = 0;
      current_step = (current_step + 1) % NUM_STEPS;
    }
  }

  return len;
}




// --- Draw OLED UI ---
void drawSequencerState() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);

  char status[32] = ""; 

  u8g2.drawStr(0, 7, "Kck");
  u8g2.drawStr(0, 23, "Snr");
  u8g2.drawStr(0, 39, "Hht");

  for (int i = 0; i < NUM_STEPS; i++) {
    int x = 20 + i * 6;
    if (kick_steps[i]) u8g2.drawBox(x, 0, 5, 5); else u8g2.drawFrame(x, 0, 5, 5);
    if (snare_steps[i]) u8g2.drawBox(x, 16, 5, 5); else u8g2.drawFrame(x, 16, 5, 5);
    if (hihat_steps[i]) u8g2.drawBox(x, 32, 5, 5); else u8g2.drawFrame(x, 32, 5, 5);
  }

  int cursorX = 20 + current_step * 6;
  u8g2.drawLine(cursorX, 0, cursorX, 38);

if (uiState == NAVIGATE) {
    switch (rotatedItem) {
      case MENU_KICK:
        strcpy(status, "SELECT: KICK"); break;
      case MENU_SNARE:
        strcpy(status, "SELECT: SNARE"); break;
      case MENU_HIHAT:
        strcpy(status, "SELECT: HIHAT"); break;
       case MENU_BPM:
      sprintf(status, "SELECT: BPM (%.0f)", bpm); break;
       case MENU_VOLUME:
        strcpy(status, "SELECT: VOL"); break;
  
    }
  }
  else if (uiState == BPM_EDIT) {
  sprintf(status, "EDIT BPM: %.0f", bpm);
}

  else if (uiState == SUB_SD_KICK) {
    //  list sample names here: sd.sampleNames[index];
    //-->check
    strcpy(status,  sd.sampleNames[index_sd_kick].c_str());
    

  }
  else if (uiState == SUB_SD_SNARE) {
  strcpy(status, sd.sampleNames[index_sd_snare].c_str());
}
else if (uiState == SUB_SD_HIHAT) {
  strcpy(status, sd.sampleNames[index_sd_hihat].c_str());
}

else if (uiState == VOL_EDIT) {
  if (volumeEditState == VOLUME_SELECT) {
    switch (volItem) {
      case VOL_KICK: strcpy(status, "VOL SELECT: KICK"); break;
      case VOL_SNARE: strcpy(status, "VOL SELECT: SNARE"); break;
      case VOL_HIHAT: strcpy(status, "VOL SELECT: HIHAT"); break;
    }
  } else if (volumeEditState == VOLUME_ADJUST) {
    switch (volItem) {
      case VOL_KICK: sprintf(status, "ADJ KICK VOL: %.2f", kickVolume); break;
      case VOL_SNARE: sprintf(status, "ADJ SNARE VOL: %.2f", snareVolume); break;
      case VOL_HIHAT: sprintf(status, "ADJ HIHAT VOL: %.2f", hihatVolume); break;
    }
  }
}




  

  u8g2.drawStr(0, 60, status);
  u8g2.sendBuffer();
}


void setup() {
  Serial.begin(115200);

sd.begin();
sd.saveSampleNames("/seq");
sd.printStoredFileNames();





  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();

encoder.begin();

  a2dp_source.set_local_name("ESP32_DrumSeq");
  a2dp_source.set_data_callback(audio_data_callback);
  a2dp_source.start("NOVA MINI"); 
}

void loop() {
  // --- Rotary encoder button toggle NAVIGATE/EDIT ---
  encoder.loop();  

  if (encoder.wasButtonPressed()) {
  switch (uiState) {

case NAVIGATE:
  if (rotatedItem == MENU_KICK) {
    uiState = SUB_SD_KICK;
  } else if (rotatedItem == MENU_SNARE) {
    uiState = SUB_SD_SNARE;
  } else if (rotatedItem == MENU_HIHAT) {
    uiState = SUB_SD_HIHAT;
  } else if (rotatedItem == MENU_BPM) {
    uiState = BPM_EDIT;
  }
  //--> volume
   else if (rotatedItem == MENU_VOLUME) {
    uiState = VOL_EDIT;
  }
  break;

case VOL_EDIT:
  if (volumeEditState == VOLUME_SELECT) {
    volumeEditState = VOLUME_ADJUST; // now adjusting selected volume
  } else {
    // Done adjusting, go back to main menu
    volumeEditState = VOLUME_SELECT;
    uiState = NAVIGATE;
  }
  break;



//-->bpm edit exit
case BPM_EDIT:
 uiState = NAVIGATE; // takes it back to main menu items
break;

   case SUB_SD_KICK:
      
if (sd.readSampleToBuffer16Bit(index_sd_kick, buffer1_sd_kick, SAMPLE_SIZE, sampleLength1_sd_kick)) {
  currentKick = SD_DATA_SAMPLE_KICK;
  Serial.println("SD sample loaded successfully.");
} else {
  currentKick = STANDARD_KICK;
  Serial.println("Failed to load SD sample. Using STANDARD_KICK.");
}
uiState = NAVIGATE;
      break;



case SUB_SD_SNARE:
  if (sd.readSampleToBuffer16Bit(index_sd_snare, buffer2_sd_snare, SAMPLE_SIZE, sampleLength2_sd_snare)) {
    currentSnare = SD_DATA_SAMPLE_SNARE;
    Serial.println("SD snare loaded successfully.");
  } else {
    currentSnare = STANDARD_SNARE;
    Serial.println("Failed to load SD snare. Using STANDARD_SNARE.");
  }
  uiState = NAVIGATE;
  break;

case SUB_SD_HIHAT:
  if (sd.readSampleToBuffer16Bit(index_sd_hihat,buffer3_sd_hihat, SAMPLE_SIZE, sampleLength3_sd_hihat)) {
    currentHihat = SD_DATA_SAMPLE__HIHAT;
    Serial.println("SD hihat loaded successfully.");
  } else {
    currentHihat = STANDARD_HIHAT;
    Serial.println("Failed to load SD hihat. Using STANDARD_HIHAT.");
  }
  uiState = NAVIGATE;
  break;


  
  }
}





  int delta = encoder.getDelta();
  if (delta != 0) {
    switch (uiState) {
      case NAVIGATE:
        rotatedItem = (MenuItem)((rotatedItem + delta + MENU_COUNT) % MENU_COUNT);
        switch (rotatedItem) {
          case MENU_KICK:  currentInstrument = KICK;  break;
          case MENU_SNARE: currentInstrument = SNARE; break;
          case MENU_HIHAT: currentInstrument = HIHAT; break;
          case MENU_BPM: /* no change to instrument */ break;
           case MENU_VOLUME:                           break;
        }
        break;

      case BPM_EDIT:
        if (rotatedItem == MENU_BPM) {
          bpm += delta;
          bpm = constrain(bpm, 40.0f, 300.0f);
          step_duration_samples = (int)((60.0f / bpm / 4.0f) * sampleRate);
        }
       break;


        case SUB_SD_KICK:
         index_sd_kick += delta;
         index_sd_kick = constrain(index_sd_kick, 0, sd.fileCount -1);
      break;

      case SUB_SD_SNARE:
      index_sd_snare += delta;
      index_sd_snare = constrain(index_sd_snare, 0, sd.fileCount - 1);
      break;

     case SUB_SD_HIHAT:
     index_sd_hihat += delta;
     index_sd_hihat = constrain(index_sd_hihat, 0, sd.fileCount - 1);
     break;


     //--> volume
case VOL_EDIT:
      if (volumeEditState == VOLUME_SELECT) {
        volItem = (VolumeItem)((volItem + delta + VOL_COUNT) % VOL_COUNT);
      } else if (volumeEditState == VOLUME_ADJUST) {
        switch (volItem) {
          case VOL_KICK:
            kickVolume += delta * 0.05f;
            kickVolume = constrain(kickVolume, 0.0f, 1.0f);
            break;
          case VOL_SNARE:
            snareVolume += delta * 0.05f;
            snareVolume = constrain(snareVolume, 0.0f, 1.0f);
            break;
          case VOL_HIHAT:
            hihatVolume += delta * 0.05f;
            hihatVolume = constrain(hihatVolume, 0.0f, 1.0f);
            break;
        }
      }
      break;
    }
  }



















  // --- Keypad input ---
  char key = tus.keypad.getKey();
  if (key) {
    int step = tus.mapKeyToStep(key);
    if (step >= 0 && step < NUM_STEPS) {
      switch (currentInstrument) {
        case KICK: kick_steps[step] = !kick_steps[step]; break;
        case SNARE: snare_steps[step] = !snare_steps[step]; break;
        case HIHAT: hihat_steps[step] = !hihat_steps[step]; break;
      }
     
    }
  }

  // --- Redraw OLED periodically ---
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw > 100) {
    drawSequencerState();
    lastDraw = millis();
  }
}






