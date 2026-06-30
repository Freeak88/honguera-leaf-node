#include "FruitingCycle.h"

// Default targets for oyster mushroom (Pleurotus ostreatus)
const PhaseTargets FruitingCycle::DEFAULT_TARGETS[4] = {
    // Colonization: warm, dark, low airflow, high CO₂ OK
    { 24.0f, 90.0f, 5000, 10, 336 },   // 14 days
    // Initiation: cold shock, fresh air, lower CO₂
    { 16.0f, 95.0f, 800,  80, 72  },   // 3 days
    // Fruiting: moderate, humid, low CO₂
    { 20.0f, 90.0f, 600,  60, 168 },   // 7 days
    // Harvest: maintenance
    { 20.0f, 85.0f, 800,  50, 48  },   // 2 days
};

FruitingCycle::FruitingCycle()
    : currentPhase_(PHASE_COLONIZATION)
    , cycleActive_(false)
    , cycleStartTime_(0)
    , phaseStartTime_(0)
{
    strainName_[0] = '\0';
    targets_ = DEFAULT_TARGETS[0];
}

void FruitingCycle::begin() {
    phaseStartTime_ = millis();
}

void FruitingCycle::setStrain(const char* name) {
    strncpy(strainName_, name, sizeof(strainName_) - 1);
    strainName_[sizeof(strainName_) - 1] = '\0';
}

void FruitingCycle::startCycle() {
    cycleActive_ = true;
    cycleStartTime_ = millis();
    phaseStartTime_ = millis();
    currentPhase_ = PHASE_COLONIZATION;
    targets_ = DEFAULT_TARGETS[PHASE_COLONIZATION];
}

void FruitingCycle::endCycle() {
    cycleActive_ = false;
}

void FruitingCycle::setPhase(FruitingPhase phase) {
    currentPhase_ = phase;
    phaseStartTime_ = millis();
    targets_ = DEFAULT_TARGETS[phase];
}

unsigned long FruitingCycle::getCycleElapsedHours() const {
    if (!cycleActive_) return 0;
    return (millis() - cycleStartTime_) / 3600000UL;
}

void FruitingCycle::update(float currentTemp, float currentHumidity, int currentCO2) {
    if (!cycleActive_) return;
    checkPhaseTransition(currentTemp, currentHumidity, currentCO2);
}

void FruitingCycle::checkPhaseTransition(float temp, float humidity, int co2) {
    unsigned long phaseHours = (millis() - phaseStartTime_) / 3600000UL;
    
    // If exceeded max phase duration, auto-advance
    if (phaseHours >= targets_.durationHours) {
        if (currentPhase_ < PHASE_HARVEST) {
            setPhase(static_cast<FruitingPhase>(currentPhase_ + 1));
        }
        return;
    }
    
    // Manual phase transitions are handled externally via MQTT command
}

void FruitingCycle::updateTargets() {
    targets_ = DEFAULT_TARGETS[currentPhase_];
}
