#include "SDCard.h"
#include <Arduino.h>
#include <SD.h>

SPIClass customSPI(VSPI);


SDCard::SDCard() {
  fileCount = 0;
}

bool SDCard::begin() {
    customSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, customSPI)) {
        Serial.println("SD card initialization failed.");
        return false;
    }
    Serial.println("SD card initialized.");
    return true;
}



void SDCard::saveSampleNames(const char* folderPath) {
    fileCount = 0; 

    File dir = SD.open(folderPath);
    if (!dir) {
        Serial.print("Failed to open folder: ");
        Serial.println(folderPath);
        return;
    }

    if (!dir.isDirectory()) {
        Serial.print(folderPath);
        Serial.println(" is not a directory.");
        dir.close();
        return;
    }

    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory() && fileCount < MAX_FILES) {
            sampleNames[fileCount++] = String(entry.name());
        }
        entry.close();
    }

    dir.close();
}







void SDCard::printStoredFileNames() {
    Serial.println("Stored file names:");
    for (int i = 0; i < fileCount; i++) {
        Serial.print("  ");
        Serial.println(sampleNames[i]);
    }
}






bool SDCard::readSampleToBuffer16Bit(int index, int16_t* buffer, int maxBufferSize, int& sampleLength) {
    sampleLength = 0;

    if (fileCount == 0) {
        Serial.println("No sample files found.");
        return false;
    }

    if (index < 0 || index >= fileCount) {
        Serial.println("Invalid sample index.");
        return false;
    }

    String filePath = String("/seq/") + sampleNames[index];
    File file = SD.open(filePath.c_str());

    if (!file) {
        Serial.print("Failed to open file: ");
        Serial.println(filePath);
        return false;
    }

    uint32_t byteSize = file.size();
    int totalSamples = byteSize / 2;
    Serial.print("Sample file contains ");
    Serial.print(totalSamples);
    Serial.println(" samples (16-bit)");

    // Only read up to maxBufferSize samples to avoid overflow
    int samplesToRead = (totalSamples > maxBufferSize) ? maxBufferSize : totalSamples;

    // Read samples in a loop to avoid partial reads
    sampleLength = 0;
    while (sampleLength < samplesToRead && file.available() >= 2) {
        uint8_t low = file.read();
        uint8_t high = file.read();
        int16_t sample = (high << 8) | low;
        buffer[sampleLength++] = sample;
    }

    file.close();

    Serial.print("Read ");
    Serial.print(sampleLength);
    Serial.println(" samples into buffer.");

    return sampleLength > 0;
}














