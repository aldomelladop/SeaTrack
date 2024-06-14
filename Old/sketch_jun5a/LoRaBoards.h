#ifndef LORABOARDS_H
#define LORABOARDS_H

#include <Arduino.h>
#include <XPowersLib.h>
#include <SPI.h>

#ifdef HAS_PMU
extern XPowersLibInterface *PMU;
extern bool pmuInterrupt;
#endif

void setupBoards();
void updateBatteryStatus();
void getChipInfo();
void beginWiFi();

bool beginSDCard();
bool beginDisplay();
void disablePeripherals();
bool beginPower();
void printResult(bool radio_online);
void flashLed();

// Definiciones de pines SPI
#define RADIO_SCLK_PIN  5
#define RADIO_MISO_PIN  19
#define RADIO_MOSI_PIN  27

#endif // LORABOARDS_H
