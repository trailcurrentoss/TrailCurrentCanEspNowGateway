#include "globals.h"
#include "canHelper.h"
#include "espNowHelper.h"
#include <OtaUpdate.h>
#include <Preferences.h>

// Create OTA update handler (3-minute timeout, 180000 ms)
// Credentials are loaded from NVS when OTA is triggered; empty here for getHostName() only
OtaUpdate otaUpdate(180000, "", "");

void setup()
{
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize

  debugln("\n=== TrailCurrent CAN-to-ESPNow Gateway ===");
  debugln("CAN to ESP-NOW Bridge with OTA Updates");

  // Initialize CAN bus
  debugln("[CAN] Initializing CAN bus...");
  canHelper::initialize();

  // Initialize OTA (connects to WiFi)
  String hostname = otaUpdate.getHostName();

  debugf("[OTA] Device hostname: %s\n", hostname.c_str());
  debugln("[OTA] Ready to receive OTA trigger (CAN ID 0x0)");
  debugln("[OTA] OTA trigger format: CAN ID=0x0, Data[0:2]=MAC bytes matching device hostname");

  // Initialize ESPNow
  debugln("[ESPNOW] Initializing ESP-NOW...");
  espNowHelper::initialize();

  delay(500);
  debugf("[WIFI] Connected: %s\n", WiFi.isConnected() ? "YES" : "NO");
  if (WiFi.isConnected()) {
    debugf("[WIFI] IP: %s\n", WiFi.localIP().toString().c_str());
  }

  debugln("=== Setup Complete ===\n");
}

void loop()
{
  canHelper::checkCanBusForMessages();
  delay(10);
}
