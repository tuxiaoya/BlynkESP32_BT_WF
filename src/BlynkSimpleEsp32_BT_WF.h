/****************************************************************************************************************************
 * BlynkSimpleEsp32_BT_WF.h
 * For ESP32 using BlueTooth along with WiFi
 *
 * Library for inclusion of both ESP32 Blynk BT / BLE and WiFi libraries. Then select one at runtime.
 * Forked from Blynk library v0.6.1 https://github.com/blynkkk/blynk-library/releases
 * Built by Khoi Hoang https://github.com/khoih-prog/BlynkGSM_ESPManager
 * Licensed under MIT license
 * Version: 1.0.2
 *
 * Original Blynk Library author:
 * @file       BlynkSimpleESP32.h
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Nov 2017
 * @brief
 *
 * Version Modified By   Date      Comments
 * ------- -----------  ---------- -----------
 *  1.0.0   K Hoang      25/01/2020 Initial coding
 *  1.0.1   K Hoang      27/01/2020 Enable simultaneously running BT/BLE and WiFi
 *  1.0.2   K Hoang      04/02/2020 Add Blynk WiFiManager support similar to Blynk_WM library
 *****************************************************************************************************************************/

#ifndef BlynkSimpleEsp32_BT_WF_h
#define BlynkSimpleEsp32_BT_WF_h

#ifndef ESP32
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#ifndef BLYNK_INFO_CONNECTION
#define BLYNK_INFO_CONNECTION "ESP32_BT"
#endif

#define BLYNK_SEND_ATOMIC
#define BLYNK_SEND_CHUNK 40

// KH
#define BLYNK_TIMEOUT_MS     30000UL

#include "sdkconfig.h"

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)

#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <utility/BlynkFifo.h>

class BlynkTransportEsp32_BT
{
  public:
    BlynkTransportEsp32_BT()
      : mConn (false)
      , mName ("Blynk")
    {}

    void setDeviceName(const char* name) {
      mName = name;
    }

    // IP redirect not available
    void begin(char BLYNK_UNUSED *h, uint16_t BLYNK_UNUSED p) {}

    void begin() {
      instance = this;

      if (!btStarted() && !btStart()) {
        BLYNK_LOG1(BLYNK_F("btStart failed"));
        return;
      }

      esp_bluedroid_status_t bt_state = esp_bluedroid_get_status();
      if (bt_state == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        if (esp_bluedroid_init()) {
          BLYNK_LOG1(BLYNK_F("esp_bluedroid_init failed"));
          return;
        }
      }

      if (bt_state != ESP_BLUEDROID_STATUS_ENABLED) {
        if (esp_bluedroid_enable()) {
          BLYNK_LOG1(BLYNK_F("esp_bluedroid_enable failed"));
          return;
        }
      }

      if (esp_spp_register_callback(esp_spp_cb) != ESP_OK) {
        BLYNK_LOG1(BLYNK_F("esp_spp_register_callback failed"));
        return;
      }

      if (esp_spp_init(ESP_SPP_MODE_CB) != ESP_OK) {
        BLYNK_LOG1(BLYNK_F("esp_spp_init failed"));
        return;
      }

      if (esp_bredr_tx_power_set(ESP_PWR_LVL_N2, ESP_PWR_LVL_P7) != ESP_OK)
      {
        BLYNK_LOG1(BLYNK_F("esp_bredr_tx_power_set failed"));
      };

      if (esp_bt_dev_set_device_name(mName) != ESP_OK)
      {
        BLYNK_LOG1(BLYNK_F("esp_bt_dev_set_device_name failed"));
      }
    }

    bool connect() {
      mBuffRX.clear();
      return mConn = true;
    }

    void disconnect() {
      mConn = false;
    }

    bool connected() {
      return mConn;
    }

    size_t read(void* buf, size_t len) {
      millis_time_t start = BlynkMillis();
      while (BlynkMillis() - start < BLYNK_TIMEOUT_MS) {
        if (available() < len) {
          delay(1);
        } else {
          break;
        }
      }
      size_t res = mBuffRX.get((uint8_t*)buf, len);
      return res;
    }

    size_t write(const void* buf, size_t len) {
      if (!spp_handle) {
        return 0;
      }

      esp_err_t err = esp_spp_write(spp_handle, len, (uint8_t *)buf);
      return (err == ESP_OK) ? len : 0;
    }

    size_t available() {
      size_t rxSize = mBuffRX.size();
      return rxSize;
    }


    static
    void putData(uint8_t* data, uint16_t len) {
      if (instance)
      {
        // BLYNK_DBG_DUMP(">> ", data, len);
        instance->mBuffRX.put(data, len);
      }
    }

  private:
    static BlynkTransportEsp32_BT* instance;
    static uint32_t spp_handle;

    static void onConnect();
    static void onDisconnect();

    bool mConn;
    const char* mName;

    BlynkFifo<uint8_t, BLYNK_MAX_READBYTES * 2> mBuffRX;

    static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
    {
      switch (event)
      {
        case ESP_SPP_INIT_EVT: // Once the SPP callback has been registered, ESP_SPP_INIT_EVT is triggered
          esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
          esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
          break;

        case ESP_SPP_CLOSE_EVT:// After the SPP disconnection, ESP_SPP_CLOSE_EVT is triggered.
          spp_handle = 0;
          onDisconnect();
          break;

        case ESP_SPP_DATA_IND_EVT:// Data received
          if (param->data_ind.len > 0)
          {
            instance->putData((uint8_t*)param->data_ind.data, param->data_ind.len);
          }
          else
          {
            BLYNK_LOG1(BLYNK_F("ESP_SPP_DATA_IND_EVT ERROR"));
          }
          break;

        case ESP_SPP_CONG_EVT: // SPP connection congestion status changed
          BLYNK_LOG1(BLYNK_F("ESP_SPP_CONG_EVT"));
          break;

        case ESP_SPP_SRV_OPEN_EVT://Server connection open
          spp_handle = param->open.handle;
          onConnect();
          break;

        default:
          break;
      }
    }
};

class BlynkEsp32_BT
  : public BlynkProtocol<BlynkTransportEsp32_BT>
{
    typedef BlynkProtocol<BlynkTransportEsp32_BT> Base;
  public:
    BlynkEsp32_BT(BlynkTransportEsp32_BT& transp)
      : Base(transp)
    {}

    void begin(const char* auth)
    {
      Base::begin(auth);
      state = DISCONNECTED;
      conn.begin();
    }

    // KH
    void begin(const char* auth, char* ssid, char* pass)
    {
      begin(auth);
    }     
    
    void setDeviceName(const char* name) {
      conn.setDeviceName(name);
    }

};

BlynkTransportEsp32_BT* BlynkTransportEsp32_BT::instance = NULL;
uint32_t BlynkTransportEsp32_BT::spp_handle = 0;

// KH
static BlynkTransportEsp32_BT _blynkTransport_BT;
BlynkEsp32_BT Blynk_BT(_blynkTransport_BT);

#define Blynk Blynk_BT

void BlynkTransportEsp32_BT::onConnect() {
  BLYNK_LOG1(BLYNK_F("BT connect"));
  Blynk_BT.startSession();
};

void BlynkTransportEsp32_BT::onDisconnect() {
  BLYNK_LOG1(BLYNK_F("BT disconnect"));
  Blynk_BT.disconnect();
}
//

#include <BlynkWidgets.h>

#endif
#endif
