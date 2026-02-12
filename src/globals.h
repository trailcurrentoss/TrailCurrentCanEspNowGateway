#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "Secrets.h"
#include <esp_wifi.h>
#include <esp_now.h>
#include "driver/twai.h"
#include "debug.h"

bool newDataToSend = false;

typedef struct
{
    uint32_t identifier;
    uint8_t data_length_code; /**< Data length code max value of 8 */
    byte dataByte0;
    byte dataByte1;
    byte dataByte2;
    byte dataByte3;
    byte dataByte4;
    byte dataByte5;
    byte dataByte6;
    byte dataByte7;
} esp_now_message_t;

esp_now_message_t outgoingMessage;