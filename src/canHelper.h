#pragma once
#include "globals.h"
#include "espNowHelper.h"
#include <OtaUpdate.h>
#include <Preferences.h>
#define CAN_RX 13
#define CAN_TX 15
// Interval:
#define POLLING_RATE_MS 33
static bool driver_installed = false;

// Forward declaration for OTA handler
extern OtaUpdate otaUpdate;

// WiFi credential reception state (CAN ID 0x01 protocol)
static bool wifiConfigInProgress = false;
static uint8_t wifiSsidBuffer[33];       // Max 32 chars + null
static uint8_t wifiPasswordBuffer[64];   // Max 63 chars + null
static uint8_t wifiSsidLen = 0;
static uint8_t wifiPasswordLen = 0;
static uint8_t wifiSsidReceived = 0;
static uint8_t wifiPasswordReceived = 0;

namespace canHelper
{
    void initialize()
    {
        // Show CAN configuration
        debugf("[CAN] Initializing with TX=GPIO%d, RX=GPIO%d, Speed=500kbps\n", CAN_TX, CAN_RX);

        // Initialize configuration structures using macro initializers
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NO_ACK);
        // g_config.rx_queue_len = 20; // Increase RX queue length to hold more messages
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Look in the api-reference for other speed sets.
        // Filter to only listen for messages from a single extended can identifier. Will need to be changed once we have
        // two different senders. One for on/off and another for 0 >> 255 values.
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        // Install TWAI driver
        debugln("[CAN] Installing driver...");
        if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
        {
            debugln("[CAN] ✓ Driver installed successfully");
        }
        else
        {
            debugln("[CAN] ✗ ERROR: Failed to install driver - check GPIO pins!");
            return;
        }

        // Start TWAI driver
        debugln("[CAN] Starting driver...");
        if (twai_start() == ESP_OK)
        {
            debugln("[CAN] ✓ Driver started successfully");
        }
        else
        {
            debugln("[CAN] ✗ ERROR: Failed to start driver");
            return;
        }

        // Reconfigure alerts to detect frame receive, Bus-Off error and RX queue full states
        debugln("[CAN] Configuring alerts...");
        uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
        if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
        {
            debugln("[CAN] ✓ Alerts configured successfully");
        }
        else
        {
            debugln("[CAN] ✗ ERROR: Failed to reconfigure alerts");
            return;
        }

        // TWAI driver is now successfully installed and started
        debugln("[CAN] ✓ CAN Bus fully initialized and ready to receive messages");
        driver_installed = true;
    }

    static void saveWifiCredentials(const char* ssid, const char* password) {
        Preferences prefs;
        prefs.begin("wifi", false);  // read-write
        prefs.putString("ssid", ssid);
        prefs.putString("password", password);
        prefs.end();
        debugf("[WiFi] Credentials saved to NVS (SSID: %s)\n", ssid);
    }

    static void handleWifiConfigMessage(twai_message_t &message) {
        uint8_t msgType = message.data[0];

        switch (msgType) {
            case 0x01: {  // Start message
                wifiSsidLen = message.data[1];
                wifiPasswordLen = message.data[2];
                wifiSsidReceived = 0;
                wifiPasswordReceived = 0;
                memset(wifiSsidBuffer, 0, sizeof(wifiSsidBuffer));
                memset(wifiPasswordBuffer, 0, sizeof(wifiPasswordBuffer));
                wifiConfigInProgress = true;
                debugf("[WiFi] Config start: SSID len=%d, Password len=%d\n", wifiSsidLen, wifiPasswordLen);
                break;
            }

            case 0x02: {  // SSID chunk
                if (!wifiConfigInProgress) break;
                uint8_t dataBytes = message.data_length_code - 2;
                uint8_t remaining = wifiSsidLen - wifiSsidReceived;
                if (dataBytes > remaining) dataBytes = remaining;
                if (wifiSsidReceived + dataBytes <= 32) {
                    memcpy(wifiSsidBuffer + wifiSsidReceived, &message.data[2], dataBytes);
                    wifiSsidReceived += dataBytes;
                }
                break;
            }

            case 0x03: {  // Password chunk
                if (!wifiConfigInProgress) break;
                uint8_t dataBytes = message.data_length_code - 2;
                uint8_t remaining = wifiPasswordLen - wifiPasswordReceived;
                if (dataBytes > remaining) dataBytes = remaining;
                if (wifiPasswordReceived + dataBytes <= 63) {
                    memcpy(wifiPasswordBuffer + wifiPasswordReceived, &message.data[2], dataBytes);
                    wifiPasswordReceived += dataBytes;
                }
                break;
            }

            case 0x04: {  // End message with checksum
                if (!wifiConfigInProgress) break;
                wifiConfigInProgress = false;

                uint8_t checksum = 0;
                for (uint8_t i = 0; i < wifiSsidReceived; i++) checksum ^= wifiSsidBuffer[i];
                for (uint8_t i = 0; i < wifiPasswordReceived; i++) checksum ^= wifiPasswordBuffer[i];

                if (checksum == message.data[1] && wifiSsidReceived == wifiSsidLen && wifiPasswordReceived == wifiPasswordLen) {
                    wifiSsidBuffer[wifiSsidReceived] = '\0';
                    wifiPasswordBuffer[wifiPasswordReceived] = '\0';
                    saveWifiCredentials((const char*)wifiSsidBuffer, (const char*)wifiPasswordBuffer);
                } else {
                    debugf("[WiFi] Config failed: checksum %s, SSID %d/%d bytes, Password %d/%d bytes\n",
                           (checksum == message.data[1]) ? "OK" : "MISMATCH",
                           wifiSsidReceived, wifiSsidLen, wifiPasswordReceived, wifiPasswordLen);
                }
                break;
            }
        }
    }

    static void handle_rx_message(twai_message_t &message)
    {
        // Print message details
        debugf("[CAN] >>> Message Received: ID=0x%03X, DLC=%d, Data=",
               message.identifier, message.data_length_code);
        for (int i = 0; i < message.data_length_code; i++) {
            debugf("%02X ", message.data[i]);
        }
        debugln();

        // OTA trigger message (ID 0x0)
        if (message.identifier == 0x0) {
            debugln("[OTA] CAN trigger received");

            // Get current device hostname
            String currentHostName = otaUpdate.getHostName();

            // Extract target hostname from CAN data
            char targetHostName[14];
            debugf("[OTA] Raw CAN data: %02X %02X %02X\n",
                    message.data[0], message.data[1], message.data[2]);

            sprintf(targetHostName, "esp32-%02X%02X%02X",
                    message.data[0], message.data[1], message.data[2]);

            debugf("[OTA] Target hostname:  '%s'\n", targetHostName);
            debugf("[OTA] Current hostname: '%s'\n", currentHostName.c_str());

            // Check if this OTA trigger is for this device
            if (currentHostName.equals(targetHostName)) {
                debugln("[OTA] ✓ Hostname matched - reading WiFi credentials from NVS");
                Preferences prefs;
                prefs.begin("wifi", true);  // read-only
                String ssid = prefs.getString("ssid", "");
                String password = prefs.getString("password", "");
                prefs.end();

                if (ssid.length() > 0 && password.length() > 0) {
                    debugf("[OTA] Using stored WiFi credentials (SSID: %s)\n", ssid.c_str());
                    OtaUpdate ota(180000, ssid.c_str(), password.c_str());
                    ota.waitForOta();
                    debugln("[OTA] OTA mode exited - resuming normal operation");
                } else {
                    debugln("[OTA] ERROR: No WiFi credentials in NVS - cannot start OTA");
                }
            } else {
                debugln("[OTA] ✗ Hostname mismatch - ignoring OTA trigger");
            }
            return;  // Don't forward OTA trigger messages via ESP-NOW
        }

        // WiFi credential configuration message (ID 0x01)
        if (message.identifier == 0x01) {
            handleWifiConfigMessage(message);
            return;  // Don't forward WiFi config messages via ESP-NOW
        }

        // Forward regular CAN messages via ESP-NOW
        outgoingMessage.identifier = message.identifier;
        outgoingMessage.data_length_code = message.data_length_code;
        if (message.data_length_code = 8)
        {
            outgoingMessage.dataByte7 = message.data[7];
        }
        else
        {
            outgoingMessage.dataByte7 = 0x0;
        }
        if (message.data_length_code = 7)
        {
            outgoingMessage.dataByte6 = message.data[6];
        }
        else
        {
            outgoingMessage.dataByte6 = 0x0;
        }
        if (message.data_length_code = 6)
        {
            outgoingMessage.dataByte5 = message.data[5];
        }
        else
        {
            outgoingMessage.dataByte5 = 0x0;
        }
        if (message.data_length_code = 5)
        {
            outgoingMessage.dataByte4 = message.data[4];
        }
        else
        {
            outgoingMessage.dataByte4 = 0;
        }
        if (message.data_length_code = 4)
        {
            outgoingMessage.dataByte3 = message.data[3];
        }
        else
        {
            outgoingMessage.dataByte3 = 0x0;
        }
        if (message.data_length_code = 3)
        {
            outgoingMessage.dataByte2 = message.data[2];
        }
        else
        {
            outgoingMessage.dataByte2 = 0x0;
        }
        if (message.data_length_code = 2)
        {
            outgoingMessage.dataByte1 = message.data[1];
        }
        else
        {
            outgoingMessage.dataByte1 = 0x0;
        }
        if (message.data_length_code = 1)
        {
            outgoingMessage.dataByte0 = message.data[0];
        }
        else
        {
            outgoingMessage.dataByte0 = 0;
        }
        newDataToSend = true;
        espNowHelper::sendData();
    }

    void checkCanBusForMessages()
    {
        // Check if alert happened
        uint32_t alerts_triggered;
        twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS));
        twai_status_info_t twaistatus;
        twai_get_status_info(&twaistatus);

        // Debug: Show CAN status periodically (every 100 loops)
        static unsigned long loop_count = 0;
        loop_count++;
        if (loop_count % 100 == 0) {
            debugf("[CAN] Status: RX errors=%lu, TX errors=%lu, RX queued=%lu\n",
                   twaistatus.rx_error_counter, twaistatus.tx_error_counter, twaistatus.msgs_to_rx);
        }

        // Check if message is received
        if (alerts_triggered & TWAI_ALERT_RX_DATA)
        {
            debugln("[CAN] *** RX DATA ALERT - Message(s) detected ***");
            // One or more messages received. Handle all.
            twai_message_t message;
            while (twai_receive(&message, 0) == ESP_OK)
            {
                handle_rx_message(message);
            }
        }

        // Debug: Check for other alerts
        if (alerts_triggered & TWAI_ALERT_ERR_PASS) {
            debugln("[CAN] WARNING: Error Passive state");
        }
        if (alerts_triggered & TWAI_ALERT_BUS_ERROR) {
            debugln("[CAN] WARNING: Bus error detected");
        }
        if (alerts_triggered & TWAI_ALERT_RX_QUEUE_FULL) {
            debugln("[CAN] WARNING: RX queue full - messages may be lost!");
        }
    }
}