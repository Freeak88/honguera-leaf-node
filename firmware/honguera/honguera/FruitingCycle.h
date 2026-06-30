// Mushroom fruiting cycle state machine
// Tracks colonization vs fruiting phases and environmental targets

#ifndef FRUITING_CYCLE_H
#define FRUITING_CYCLE_H

#include <Arduino.h>

enum FruitingPhase {
    PHASE_COLONIZATION,
    PHASE_INITIATION,    // Cold shock / fresh air pulse
    PHASE_FRUITING,
    PHASE_HARVEST
};

struct PhaseTargets {
    float targetTemp;        // °C
    float targetHumidity;    // %RH
    int   targetCO2;         // ppm
    float targetAirflow;     // % fan speed
    unsigned long durationHours;  // max hours for this phase
};

class FruitingCycle {
public:
    FruitingCycle();
    ~FruitingCycle() = default;

    void begin();
    void update(float currentTemp, float currentHumidity, int currentCO2);
    
    void setPhase(FruitingPhase phase);
    FruitingPhase getPhase() const { return currentPhase_; }
    
    const PhaseTargets& getTargets() const { return targets_; }
    
    void setStrain(const char* name);
    const char* getStrain() const { return strainName_; }

    bool isCycleActive() const { return cycleActive_; }
    void startCycle();
    void endCycle();
    unsigned long getCycleElapsedHours() const;

private:
    FruitingPhase currentPhase_;
    PhaseTargets targets_;
    char strainName_[32];
    
    bool cycleActive_;
    unsigned long cycleStartTime_;
    unsigned long phaseStartTime_;

    void updateTargets();
    void checkPhaseTransition(float temp, float humidity, int co2);
    
    // Default targets by phase (oyster mushroom baseline)
    static const PhaseTargets DEFAULT_TARGETS[4];
};

#endif // FRUITING_CYCLE_H
