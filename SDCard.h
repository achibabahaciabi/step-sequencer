#ifndef SDCARD_H
#define SDCARD_H

#include <SD.h>


#define MAX_FILES 50


// === SD CARD
#define SD_CS    26
#define SD_MOSI  27
#define SD_MISO  14
#define SD_SCK   12


extern SPIClass customSPI;

class SDCard {
  public:
    SDCard();
    bool begin();
    void saveSampleNames(const char* folderPath);
    void printStoredFileNames();
     String sampleNames[32];
     int fileCount;




bool readSampleToBuffer16Bit(int index, int16_t* buffer, int maxBufferSize, int& sampleLength) ;




};

#endif
