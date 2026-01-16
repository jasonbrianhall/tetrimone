#include "tetrimone.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void TetrimoneBoard::updateHeat() {
        heatDecayTimer = g_timeout_add(1000, [](gpointer data) -> gboolean {
            TetrimoneBoard* board = static_cast<TetrimoneBoard*>(data);
            board->coolDown();
            return TRUE; // Keep timer running
        }, this);

}

void TetrimoneBoard::coolDown() {
    // Base cooling rate per 100ms (independent of game timer speed)
    float baseCoolingRate = 0.005f;
    
    // Optional: Still apply a small level-based modifier if desired
    // This makes higher levels slightly more intense by retaining heat longer
    float levelHeatRetention = 1.0f + (level - 1) * 0.1f;
    
    // Prevent division by zero or very small numbers
    if (levelHeatRetention < 0.0001f) {
        levelHeatRetention = 0.0001f;
    }
    
    float adjustedCoolingRate = baseCoolingRate / levelHeatRetention;
    
    // Use adjusted cooling rate for all difficulties
    heatLevel -= adjustedCoolingRate;
    
    if (heatLevel < 0) {
        heatLevel = 0.0;
    }
    
    // Round to nearest hundredth
    heatLevel = round(heatLevel * 10000.0f) / 10000.0f;
    //printf("Heat level %f\n", heatLevel);
    #ifdef DEBUG
    printf("Heat level %f\n", heatLevel);
    #endif
}


/*std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel) {
    std::array<double, 3> result = baseColor;
    
    if (heatLevel < 0.5f) {
        // Cooling down - blend toward ice blue
        float iceAmount = (0.5f - heatLevel) * 2.0f; // 0.0 to 1.0
        result[0] = baseColor[0] * (1.0f - iceAmount) + 0.7 * iceAmount;  // Blue tint
        result[1] = baseColor[1] * (1.0f - iceAmount) + 0.9 * iceAmount;  // Keep green high
        result[2] = baseColor[2] * (1.0f - iceAmount) + 1.0 * iceAmount;  // Max blue
    } else if (heatLevel > 0.5f) {
        // Heating up - blend toward fire colors
        float fireAmount = (heatLevel - 0.5f) * 2.0f; // 0.0 to 1.0
        result[0] = baseColor[0] * (1.0f - fireAmount) + 1.0 * fireAmount;  // Max red
        result[1] = baseColor[1] * (1.0f - fireAmount) + (0.5 + fireAmount * 0.3) * fireAmount; // Orange
        result[2] = baseColor[2] * (1.0f - fireAmount * 0.8);  // Reduce blue for fire
    }
    
    return result;
}*/

std::array<double, 3> getHeatModifiedColor(const std::array<double, 3>& baseColor, float heatLevel) {
    std::array<double, 3> result = baseColor;
    
    if (heatLevel < 0.5f) {
        // Cooling down - make colors duller (darker)
        float dullAmount = (0.5f - heatLevel) * 2.0f; // 0.0 to 1.0
        float dullFactor = 1.0f - (dullAmount * 0.6f); // Scale down to 40% at minimum
        
        result[0] = baseColor[0] * dullFactor;
        result[1] = baseColor[1] * dullFactor;
        result[2] = baseColor[2] * dullFactor;
    } else if (heatLevel > 0.5f) {
        // Heating up - make colors brighter
        float brightAmount = (heatLevel - 0.5f) * 2.0f; // 0.0 to 1.0
        float brightFactor = 1.0f + (brightAmount * 0.5f); // Scale up to 150% at maximum
        
        result[0] = std::min(1.0, baseColor[0] * brightFactor);
        result[1] = std::min(1.0, baseColor[1] * brightFactor);
        result[2] = std::min(1.0, baseColor[2] * brightFactor);
    }
    
    return result;
}

float TetrimoneBoard::getHeatLevel() {
    return heatLevel;
}

void TetrimoneBoard::setHeatLevel(float heatLevelData) {
    heatLevel=heatLevelData;
}



