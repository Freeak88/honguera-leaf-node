#ifndef HONGUERA_NODE_H
#define HONGUERA_NODE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Network
#include "network/WiFiManager.h"
#include "network/BLEConfigManager.h"
#include "network/MQTTManager.h"
#include "network/OTAManager.h"

// Core
#include "core/CommandHandler.h"
#include "core/TaskManager.h"
#include "core/SetupManager.h"

// System
#include "system/SystemManager.h"
#include "system/ScheduleManager.h"

// Hardware
#include "hardware/Actuator.h"
#include "hardware/OneWireManager.h"
#include "hardware/PWMController.h"
#include "hardware/StatusLED.h"

// Sensors
#include "sensors/SensorManager.h"
#include "hardware/sensors/SHT31.h"
#include "hardware/sensors/DS18B20.h"
#include "hardware/sensors/MHZ19.h"

// Diagnostics
#include "diagnostics/Logger.h"
#include "diagnostics/SerialCommandHandler.h"

// Runtime
#include "runtime/RuntimeConfig.h"

// Honguera-specific
#include "honguera/FruitingCycle.h"
#include "honguera/HumidityPID.h"

class HongueraNode {
public:
    HongueraNode();
    ~HongueraNode();

    bool initialize();
    void update();

private:
    // Core systems
    Logger*         logger_;
    RuntimeConfig*  config_;
    WiFiManager*    wifiManager_;
    BLEConfigManager* bleManager_;
    MQTTManager*    mqttManager_;
    OTAManager*     otaManager_;
    CommandHandler* commandHandler_;
    TaskManager*    taskManager_;
    SetupManager*   setupManager_;

    // Hardware
    Actuator*       actuator_;
    OneWireManager* oneWireManager_;
    PWMController*  pwmController_;
    StatusLED*      statusLED_;

    // Sensors
    SensorManager*  sensorManager_;
    SHT31*          sht31Sensor_;
    DS18B20*        ds18b20Sensor_;
    MHZ19*          mhz19Sensor_;

    // Honguera-specific
    FruitingCycle*  fruitingCycle_;
    HumidityPID*    humidityPID_;

    // System
    SystemManager*  systemManager_;
    ScheduleManager* scheduleManager_;

    // State
    unsigned long lastPublishTime_;
    unsigned long lastUpdateTime_;
    bool initialized_;

    // Compat methods for SerialCommandHandler
    StatusLED* getStatusLED() { return statusLED_; }
    void simulateRegistrationAck(const String& payload) {}

    // Internal methods
    bool initLogger();
    bool initConfig();
    bool initNetwork();
    bool initSensors();
    bool initActuators();
    bool initHonguera();
    bool initMQTT();

    void publishSensorData();
    void handleOTA();
    void watchdogCheck();

    // MQTT callback
    static void mqttCallback(const String& topic, const String& payload);
    void handleMQTTCommand(const String& topic, const String& payload);

    // Status LED
    static void ledUpdateCallback(LEDStatus status);
};

#endif // HONGUERA_NODE_H
