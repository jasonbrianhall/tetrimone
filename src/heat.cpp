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
    printf("Heat level %f\n", heatLevel);
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

void drawFireyGlow(cairo_t* cr, double x, double y, double size, float heatLevel, double time) {
   if (heatLevel <= 0.7f) return;
   
   // Calculate glow intensity based on heat level
   float glowIntensity = (heatLevel - 0.7f) / 0.3f; // 0.0 to 1.0 range
   
   // Create animated glow with time-based pulsing (faster pulse at higher heat)
   double pulseSpeed = 1.0 + glowIntensity * 2.0; // Pulse faster when hotter
   double pulseTime = fmod(time * pulseSpeed, 2000.0) / 2000.0; // 2-second base cycle
   double pulse = 0.5 + 0.5 * sin(pulseTime * 2 * M_PI); // 0.0 to 1.0 pulse
   
   // Combine intensity with pulse for final glow strength
   double finalGlowStrength = glowIntensity * (0.6 + 0.4 * pulse);
   
   // Save the current state
   cairo_save(cr);
   
   // Create multiple glow layers for depth (more layers at higher heat)
   int numLayers = 3 + (int)(glowIntensity * 2); // 3-5 layers based on heat
   for (int layer = 0; layer < numLayers; layer++) {
       double layerSize = size + (layer + 1) * 6 * finalGlowStrength;
       double layerAlpha = finalGlowStrength * (0.5 - layer * 0.08);
       
       if (layerAlpha <= 0) continue;
       
       // Prevent division by zero for radius
       double radius = layerSize / 2;
       if (radius <= 0) radius = 1.0;
       
       // Create radial gradient for each glow layer
       cairo_pattern_t* gradient = cairo_pattern_create_radial(
           x + size/2, y + size/2, 0,
           x + size/2, y + size/2, radius
       );
       
       // Color shifts based on heat intensity
       float redIntensity = 1.0f;
       float greenIntensity = 0.3f + glowIntensity * 0.4f;
       float blueIntensity = glowIntensity > 0.8f ? 0.2f : 0.0f; // Add blue for extreme heat
       
       // Inner glow color (varies with intensity)
       cairo_pattern_add_color_stop_rgba(gradient, 0.0, 
           redIntensity, greenIntensity, blueIntensity, layerAlpha);
       
       // Outer glow color (red fading to transparent)
       cairo_pattern_add_color_stop_rgba(gradient, 1.0, 
           1.0, 0.0, 0.0, 0.0);
       
       cairo_set_source(cr, gradient);
       
       // Draw the glow circle
       cairo_arc(cr, x + size/2, y + size/2, radius, 0, 2 * M_PI);
       cairo_fill(cr);
       
       cairo_pattern_destroy(gradient);
   }
   
   // Add flickering fire particles for extra effect at high heat
   if (heatLevel > 0.85f) {
       cairo_set_source_rgba(cr, 1.0, 0.8, 0.0, 0.4 * pulse * glowIntensity);
       
       // More particles at higher heat
       int numParticles = 6 + (int)(glowIntensity * 4);
       if (numParticles <= 0) numParticles = 1; // Prevent division by zero
       
       for (int i = 0; i < numParticles; i++) {
           double angle = (i / (double)numParticles) * 2 * M_PI + pulseTime * 4 * M_PI;
           
           // Ensure rand() % doesn't get 0
           int randRange = 5;
           if (randRange <= 0) randRange = 1;
           
           double radius = size/2 + 5 + (rand() % randRange) * glowIntensity;
           double particleX = x + size/2 + cos(angle) * radius;
           double particleY = y + size/2 + sin(angle) * radius;
           
           double particleRadius = 1 + glowIntensity * 2;
           if (particleRadius <= 0) particleRadius = 1.0;
           
           cairo_arc(cr, particleX, particleY, particleRadius, 0, 2 * M_PI);
           cairo_fill(cr);
       }
   }
   
   cairo_restore(cr);
}

// New function for drawing freezy effect
void drawFreezyEffect(cairo_t* cr, double x, double y, double size, float heatLevel, double time) {
   if (heatLevel >= 0.3f) return;
   
   // Calculate freeze intensity (higher when colder)
   float freezeIntensity = (0.3f - heatLevel) / 0.3f; // 0.0 to 1.0 range
   
   // Create subtle shimmer effect for ice (slower when colder)
   double shimmerSpeed = 0.5 + (1.0 - freezeIntensity) * 0.5; // Slower shimmer when colder
   double shimmerTime = fmod(time * shimmerSpeed, 3000.0) / 3000.0; // 3-second base cycle
   double shimmer = 0.3 + 0.2 * sin(shimmerTime * 2 * M_PI);
   
   cairo_save(cr);
   
   // Draw ice crystal overlay (more opaque when colder)
   double iceOpacity = 0.15 + 0.25 * freezeIntensity;
   cairo_set_source_rgba(cr, 0.7, 0.9, 1.0, iceOpacity * shimmer);
   cairo_rectangle(cr, x, y, size, size);
   cairo_fill(cr);
   
   // Create multiple layers of sparkly stars
   for (int layer = 0; layer < 3; layer++) {
       // Different timing for each layer creates depth
       double layerTime = shimmerTime + (layer * 0.3);
       double layerShimmer = 0.4 + 0.6 * sin(layerTime * 2 * M_PI);
       
       // Star brightness varies by layer and freeze intensity
       double starAlpha = (0.3 + 0.7 * freezeIntensity) * layerShimmer * (1.0 - layer * 0.2);
       cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, starAlpha);
       
       // Number of stars increases with freeze intensity
       int numStars = 2 + layer + (int)(freezeIntensity * 4);
       
       for (int i = 0; i < numStars; i++) {
           // Use deterministic positioning based on block coordinates and layer
           int seed = (int)(x + y * 100 + layer * 1000 + i * 137);
           srand(seed); // Temporary seed for consistent positioning
           
           // Fix: Ensure we don't get division by zero or negative modulo
           int maxX = (int)(size - 8);
           int maxY = (int)(size - 8);
           if (maxX <= 0) maxX = 1;
           if (maxY <= 0) maxY = 1;
           
           double starX = x + 4 + (rand() % maxX);
           double starY = y + 4 + (rand() % maxY);
           
           // Star size varies with intensity and layer
           double starSize = 1.5 + freezeIntensity * 2 + layer * 0.5;
           
           // Draw 4-pointed star
           double armLength = starSize + layerShimmer * 0.5;
           
           // Horizontal arm
           cairo_move_to(cr, starX - armLength, starY);
           cairo_line_to(cr, starX + armLength, starY);
           
           // Vertical arm
           cairo_move_to(cr, starX, starY - armLength);
           cairo_line_to(cr, starX, starY + armLength);
           
           cairo_set_line_width(cr, 0.8 + layer * 0.2);
           cairo_stroke(cr);
           
           // Add diagonal arms for bigger stars at higher freeze intensity
           if (freezeIntensity > 0.5f && layer == 0) {
               double diagLength = armLength * 0.7;
               
               // Diagonal arms
               cairo_move_to(cr, starX - diagLength, starY - diagLength);
               cairo_line_to(cr, starX + diagLength, starY + diagLength);
               cairo_move_to(cr, starX - diagLength, starY + diagLength);
               cairo_line_to(cr, starX + diagLength, starY - diagLength);
               
               cairo_set_line_width(cr, 0.6);
               cairo_stroke(cr);
           }
       }
   }
   
   // Add floating sparkle particles around the block for extreme cold
   if (heatLevel < 0.1f) {
       cairo_set_source_rgba(cr, 0.9, 0.95, 1.0, 0.4 * shimmer);
       
       // Draw small sparkle points floating around the block
       for (int i = 0; i < 6; i++) {
           double angle = (i / 6.0) * 2 * M_PI + shimmerTime * M_PI;
           double distance = size/2 + 4 + sin(shimmerTime * 4 + i) * 2;
           double sparkleX = x + size/2 + cos(angle) * distance;
           double sparkleY = y + size/2 + sin(angle) * distance;
           
           // Draw tiny 4-pointed stars
           double tinySize = 1.0 + sin(shimmerTime * 3 + i * 2) * 0.5;
           
           cairo_move_to(cr, sparkleX - tinySize, sparkleY);
           cairo_line_to(cr, sparkleX + tinySize, sparkleY);
           cairo_move_to(cr, sparkleX, sparkleY - tinySize);
           cairo_line_to(cr, sparkleX, sparkleY + tinySize);
           
           cairo_set_line_width(cr, 0.5);
           cairo_stroke(cr);
       }
   }
   
   cairo_restore(cr);
}

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



