#include "LoRaBoards.h"

#ifdef HAS_PMU
XPowersLibInterface *PMU = NULL;
bool pmuInterrupt = false;

static void setPmuFlag() {
  pmuInterrupt = true;
}
#endif

bool beginPower() {
#ifdef HAS_PMU
  if (!PMU) {
    PMU = new XPowersAXP2101(PMU_WIRE_PORT);
    if (!PMU->init()) {
      Serial.println("Warning: Failed to find AXP2101 power management");
      delete PMU;
      PMU = NULL;
    } else {
      Serial.println("AXP2101 PMU init succeeded, using AXP2101 PMU");
    }
  }

  if (!PMU) {
    PMU = new XPowersAXP192(PMU_WIRE_PORT);
    if (!PMU->init()) {
      Serial.println("Warning: Failed to find AXP192 power management");
      delete PMU;
      PMU = NULL;
    } else {
      Serial.println("AXP192 PMU init succeeded, using AXP192 PMU");
    }
  }

  if (!PMU) {
    return false;
  }

  PMU->setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);
  pinMode(PMU_IRQ, INPUT_PULLUP);
  attachInterrupt(PMU_IRQ, setPmuFlag, FALLING);

  if (PMU->getChipModel() == XPOWERS_AXP192) {
    PMU->setProtectedChannel(XPOWERS_DCDC3);
    PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
    PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
    PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
    PMU->enablePowerOutput(XPOWERS_LDO2);
    PMU->enablePowerOutput(XPOWERS_LDO3);
    PMU->setProtectedChannel(XPOWERS_DCDC1);
    PMU->setProtectedChannel(XPOWERS_DCDC3);
    PMU->enablePowerOutput(XPOWERS_DCDC1);
    PMU->disablePowerOutput(XPOWERS_DCDC2);
    PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);
    PMU->enableIRQ(XPOWERS_AXP192_VBUS_REMOVE_IRQ | XPOWERS_AXP192_VBUS_INSERT_IRQ | XPOWERS_AXP192_BAT_CHG_DONE_IRQ | XPOWERS_AXP192_BAT_CHG_START_IRQ | XPOWERS_AXP192_BAT_REMOVE_IRQ | XPOWERS_AXP192_BAT_INSERT_IRQ | XPOWERS_AXP192_PKEY_SHORT_IRQ);
  } else if (PMU->getChipModel() == XPOWERS_AXP2101) {
    PMU->disablePowerOutput(XPOWERS_DCDC2);
    PMU->disablePowerOutput(XPOWERS_DCDC3);
    PMU->disablePowerOutput(XPOWERS_DCDC4);
    PMU->disablePowerOutput(XPOWERS_DCDC5);
    PMU->disablePowerOutput(XPOWERS_ALDO1);
    PMU->disablePowerOutput(XPOWERS_ALDO4);
    PMU->disablePowerOutput(XPOWERS_BLDO1);
    PMU->disablePowerOutput(XPOWERS_BLDO2);
    PMU->disablePowerOutput(XPOWERS_DLDO1);
    PMU->disablePowerOutput(XPOWERS_DLDO2);

    PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
    PMU->enablePowerOutput(XPOWERS_VBACKUP);
    PMU->setProtectedChannel(XPOWERS_DCDC1);
    PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO2);
    PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO3);
  }

  PMU->enableSystemVoltageMeasure();
  PMU->enableVbusVoltageMeasure();
  PMU->enableBattVoltageMeasure();
#endif
  return true;
}

void updateBatteryStatus() {
#ifdef HAS_PMU
  if (PMU) {
    float batteryVoltage = PMU->getBatteryVoltage();
    int batteryPercent = PMU->getBatteryLevel();
    bool isCharging = PMU->isCharging();
    bool isVbusIn = PMU->isVbusIn();

    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.println(" mV");

    Serial.print("Battery Percent: ");
    Serial.print(batteryPercent);
    Serial.println(" %");

    Serial.print("Charging: ");
    Serial.println(isCharging ? "YES" : "NO");

    Serial.print("VBUS In: ");
    Serial.println(isVbusIn ? "YES" : "NO");

    // Enviar datos a Blynk
    Blynk.virtualWrite(V10, batteryVoltage);
    Blynk.virtualWrite(V11, batteryPercent);
    Blynk.virtualWrite(V12, isCharging);
    Blynk.virtualWrite(V13, isVbusIn);
  }
#endif
}

void setupBoards() {
  Serial.begin(115200);
  Serial.println("setupBoards");
  getChipInfo();

#if defined(ARDUINO_ARCH_ESP32)
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
#endif

#ifdef HAS_SDCARD
  SDCardSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI);
#endif

#ifdef I2C_SDA
  Wire.begin(I2C_SDA, I2C_SCL);
#endif

#ifdef HAS_GPS
  SerialGPS.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif

  beginPower();
  beginSDCard();
  beginDisplay();
  beginWiFi();

  Serial.println("init done.");
}

void beginWiFi() {
    // Esta función puede estar vacía o incluir cualquier configuración adicional que necesites para WiFi
}