#include "HongueraNode.h"
#include "config.h"
#include <ArduinoJson.h>

// Static instance pointer for callback
HongueraNode* HongueraNode::instance_ = nullptr;

HongueraNode::HongueraNode()
    : config_(nullptr)
    , systemManager_(nullptr)
    , taskManager_(nullptr)
    , commandHandler_(nullptr)
    , setupManager_(nullptr)
    , logger_(nullptr)
    , serialCommandHandler_(nullptr)
    , wifiManager_(nullptr)
    , bleManager_(nullptr)
    , mqttManager_(nullptr)
    , otaManager_(nullptr)
    , statusLED_(nullptr)
    , oneWireManager_(nullptr)
    , sensorManager_(nullptr)
    , actuator_(nullptr)
    , scheduleManager_(nullptr)
    , sht31Sensor_(nullptr)
    , ds18b20Sensor_(nullptr)
    , mhz19Sensor_(nullptr)
    , fruitingCycle_(nullptr)
    , humidityPID_(nullptr)
    , initialized_(false)
    , lastPublishTime_(0)
    , lastUpdateTime_(0) {

    instance_ = this;
}

HongueraNode::~HongueraNode() {
    instance_ = nullptr;
    delete config_;
    delete systemManager_;
    delete taskManager_;
    delete commandHandler_;
    delete setupManager_;
    delete logger_;
    delete serialCommandHandler_;
    delete wifiManager_;
    delete bleManager_;
    delete mqttManager_;
    delete otaManager_;
    delete oneWireManager_;
    delete sensorManager_;
    delete actuator_;
    delete scheduleManager_;
    delete sht31Sensor_;
    delete ds18b20Sensor_;
    delete mhz19Sensor_;
    delete fruitingCycle_;
    delete humidityPID_;
}

bool HongueraNode::initialize() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=== Honguera Node Firmware Starting ===");
    Serial.println("Version: " + String(FIRMWARE_VERSION));

    // Logger
    logger_ = new Logger();
    if (!logger_->initialize(LogLevel::INFO, true)) {
        Serial.println("FATAL: Logger init failed");
        return false;
    }
    logger_->info("Honguera", "Starting initialization...");

    // Config
    config_ = new RuntimeConfig();
    if (!config_->load()) {
        logger_->warning("Honguera", "No config found, using defaults");
        config_->resetToDefaults();
    }

    // System manager
    systemManager_ = new SystemManager();
    if (!systemManager_->initialize()) {
        logger_->error("Honguera", "SystemManager init failed");
        return false;
    }

    // Task manager
    taskManager_ = new TaskManager();
    if (!taskManager_->initialize()) {
        logger_->error("Honguera", "TaskManager init failed");
        return false;
    }

    // Command handler
    commandHandler_ = new CommandHandler();
    commandHandler_->setLogger(logger_);
    commandHandler_->setSystemManager(systemManager_);
    commandHandler_->setRuntimeConfig(config_);
    if (!commandHandler_->initialize()) {
        logger_->error("Honguera", "CommandHandler init failed");
        return false;
    }

    // Serial command handler
    serialCommandHandler_ = new SerialCommandHandler(config_, logger_, this);
    serialCommandHandler_->setEnabled(true);

    // Status LED
    statusLED_ = new StatusLED();
#ifdef USE_WS2812B_LED
    statusLED_->initialize(LED_WS2812B_PIN, 255);
#endif
#ifdef USE_RGB_LED
    statusLED_->initialize(LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN, 255);
#endif
    statusLED_->setStatus(LEDStatus::STARTUP);
    systemManager_->setStatusLED(statusLED_);

    // WiFi
    wifiManager_ = new WiFiManager();
    if (!wifiManager_->initialize()) {
        logger_->error("Honguera", "WiFiManager init failed");
        return false;
    }
    wifiManager_->setOnConnectedCallback([this]() {
        if (scheduleManager_) scheduleManager_->syncTime();
    });

    // BLE
    bleManager_ = new BLEConfigManager();
    if (!bleManager_->initialize(config_->getDeviceName())) {
        logger_->error("Honguera", "BLE init failed");
        return false;
    }
    bleManager_->setWiFiConfigCallback([this](const String& ssid, const String& pwd, const String& uid) {
        logger_->info("Honguera", "WiFi creds received via BLE: " + ssid);
        config_->setWiFiSSID(ssid);
        config_->setWiFiPassword(pwd);
        config_->setWiFiAutoConnect(true);
        config_->save();
        if (wifiManager_->connect(ssid, pwd)) {
            onWiFiConnected();
        }
    });

    // MQTT
    mqttManager_ = new MQTTManager();
    if (!mqttManager_->initialize()) {
        logger_->error("Honguera", "MQTTManager init failed");
        return false;
    }
    if (config_->hasMQTTCredentials()) {
        mqttManager_->configure(
            config_->getMQTTServer(), config_->getMQTTPort(),
            config_->getMQTTUsername(), config_->getMQTTPassword(),
            config_->getMQTTClientId()
        );
    }
    mqttManager_->setConnectedCallback([this]() { onMQTTConnected(); });
    mqttManager_->setDisconnectedCallback([this]() { onMQTTDisconnected(); });
    mqttManager_->setMessageCallback([this](const String& t, const String& p) { handleMQTTCommand(t, p); });

    // Start network
    startNetworkConnection();

    // Sensor manager (simplified — just orchestrates our built-in sensors)
    sensorManager_ = new SensorManager(config_, nullptr, nullptr, oneWireManager_, mqttManager_, logger_);
    sensorManager_->initialize();

    // OneWire (DS18B20)
    oneWireManager_ = new OneWireManager(*logger_);

    // SHT31
    sht31Sensor_ = new SHT31();
    sht31Sensor_->begin();

    // DS18B20
    ds18b20Sensor_ = new DS18B20(oneWireManager_);
    ds18b20Sensor_->begin();

    // MH-Z19B CO2 (UART2)
    mhz19Sensor_ = new MHZ19(18, 19);  // RX=18, TX=19
    mhz19Sensor_->begin();

    // Actuator (MOSFET + Relay)
    actuator_ = new Actuator();
    if (!actuator_->initialize(MOSFET_PIN, RELAY_PIN)) {
        logger_->error("Honguera", "Actuator init failed");
        return false;
    }
    actuator_->setSystemManager(systemManager_);
    actuator_->setRuntimeConfig(config_);
    actuator_->restoreStates();
    commandHandler_->setActuator(actuator_);

    // PWM
    if (systemManager_->getPWMController()) {
        commandHandler_->setPWMController(systemManager_->getPWMController());
    }

    // Schedule manager
    scheduleManager_ = new ScheduleManager();
    scheduleManager_->initialize(logger_, config_, actuator_, nullptr, 
                                 systemManager_->getPWMController(), 
                                 systemManager_->getPWMControllerMOSFET(), nullptr);
    commandHandler_->setScheduleManager(scheduleManager_);

    // Honguera-specific: Fruiting Cycle
    fruitingCycle_ = new FruitingCycle();
    fruitingCycle_->begin();
    fruitingCycle_->startCycle();

    // Honguera-specific: Humidity PID
    humidityPID_ = new HumidityPID();
    humidityPID_->setTarget(90.0f);

    // Heartbeat task
    taskManager_->addTask("Heartbeat", [this]() {
        publishSensorData();
        lastPublishTime_ = millis();
    }, config_->getHeartbeatInterval(), TaskPriority::NORMAL);

    systemManager_->setStatus(SystemStatus::RUNNING);
    initialized_ = true;
    logger_->info("Honguera", "✅ Initialization complete");
    return true;
}

void HongueraNode::update() {
    if (!initialized_) return;

    // Actuator timers (high priority)
    if (actuator_) actuator_->update();

    // Serial commands
    if (serialCommandHandler_) serialCommandHandler_->update();

    // Setup manager
    if (setupManager_) setupManager_->update();

    // System
    systemManager_->update();
    systemManager_->feedWatchdog();

    // PWM timers
    systemManager_->updatePWMTimers();

    // Task manager
    taskManager_->update();

    // Schedule
    if (scheduleManager_) scheduleManager_->update();

    // LED
    if (statusLED_) statusLED_->update();

    // WiFi
    if (wifiManager_) {
        NetworkStatus prev = wifiManager_->getStatus();
        wifiManager_->update();
        if (prev == NetworkStatus::WIFI_CONNECTED && wifiManager_->getStatus() != NetworkStatus::WIFI_CONNECTED) {
            logger_->warning("Network", "WiFi lost");
        }
    }

    // MQTT
    if (mqttManager_) mqttManager_->update();

    // OTA
    if (otaManager_) otaManager_->update();

    // BLE
    if (bleManager_ && bleManager_->isActive()) bleManager_->handleEvents();

    // Sensor readings
    static unsigned long lastRead = 0;
    if (mqttManager_ && mqttManager_->isConnected() && millis() - lastRead > 30000) {
        lastRead = millis();
        readAndPublishSensors();
    }
}

void HongueraNode::readAndPublishSensors() {
    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis();

    // SHT31
    float temp = NAN, hum = NAN;
    if (sht31Sensor_->read(temp, hum)) {
        doc["temperature_c"] = temp;
        doc["humidity_pct"] = hum;
    }

    // DS18B20 (substrate temperature)
    float substrateTemp = NAN;
    if (ds18b20Sensor_->read(substrateTemp)) {
        doc["substrate_temp_c"] = substrateTemp;
    }

    // MH-Z19B CO2
    int co2 = 0;
    if (mhz19Sensor_->readCO2(co2)) {
        doc["co2_ppm"] = co2;
    }

    // Fruiting cycle
    if (fruitingCycle_->isCycleActive()) {
        fruitingCycle_->update(temp, hum, co2);
        doc["phase"] = static_cast<int>(fruitingCycle_->getPhase());
        doc["phase_hours"] = fruitingCycle_->getCycleElapsedHours();
        auto& tgt = fruitingCycle_->getTargets();
        doc["target_temp_c"] = tgt.targetTemp;
        doc["target_humidity_pct"] = tgt.targetHumidity;
        doc["target_co2_ppm"] = tgt.targetCO2;
    }

    // Humidity PID
    if (!isnan(hum)) {
        float pidOut = humidityPID_->compute(hum);
        doc["pid_humidity_output"] = pidOut;
    }

    // Actuator state
    if (actuator_) {
        doc["mosfet"] = actuator_->getState(Actuator::Type::MOSFET);
        doc["relay"] = actuator_->getState(Actuator::Type::RELAY);
    }

    String payload;
    serializeJson(doc, payload);
    String topic = String(MQTT_TOPIC_PREFIX) + config_->getSerialNumber() + "/sensors";

    if (mqttManager_ && mqttManager_->isConnected()) {
        mqttManager_->publish(topic, payload, false);
    }
}

void HongueraNode::publishSensorData() {
    if (!mqttManager_ || !mqttManager_->isConnected()) return;

    StaticJsonDocument<256> hb;
    hb["uptime_s"] = millis() / 1000;
    hb["free_heap"] = systemManager_->getFreeHeap();
    hb["firmware"] = FIRMWARE_VERSION;
    if (WiFi.status() == WL_CONNECTED) hb["rssi"] = WiFi.RSSI();

    String p;
    serializeJson(hb, p);
    mqttManager_->publish(
        String(MQTT_TOPIC_PREFIX) + config_->getSerialNumber() + "/heartbeat", p, false
    );
}

void HongueraNode::startNetworkConnection() {
    if (config_->hasWiFiCredentials() && config_->isWiFiAutoConnect()) {
        if (wifiManager_->connect(config_->getWiFiSSID(), config_->getWiFiPassword())) {
            onWiFiConnected();
            return;
        }
    }
    logger_->info("Honguera", "Starting BLE config mode");
    bleManager_->start();
    statusLED_->setStatus(LEDStatus::BLE_CONFIG);
}

void HongueraNode::onWiFiConnected() {
    logger_->info("Honguera", "WiFi connected: " + WiFi.localIP().toString());
    statusLED_->setStatus(LEDStatus::CONNECTED);
    connectMQTT();
}

void HongueraNode::connectMQTT() {
    if (!mqttManager_ || !config_->hasMQTTCredentials()) return;
    mqttManager_->connect();
}

void HongueraNode::onMQTTConnected() {
    logger_->info("Honguera", "MQTT connected");
    mqttManager_->subscribeToDeviceTopics(config_->getSerialNumber());

    StaticJsonDocument<128> doc;
    doc["status"] = "online";
    doc["firmware"] = FIRMWARE_VERSION;
    String p; serializeJson(doc, p);
    mqttManager_->publish(
        String(MQTT_TOPIC_PREFIX) + config_->getSerialNumber() + "/status", p, true
    );
}

void HongueraNode::onMQTTDisconnected() {
    logger_->warning("Honguera", "MQTT disconnected");
}

void HongueraNode::handleMQTTCommand(const String& topic, const String& payload) {
    logger_->info("MQTT", "Cmd: " + topic + " -> " + payload);
    if (commandHandler_) commandHandler_->processCommand(topic, payload);
}

// Static callback
void HongueraNode::ledUpdateCallback(LEDStatus status) {
    if (instance_ && instance_->statusLED_) {
        instance_->statusLED_->setStatus(status);
    }
}
