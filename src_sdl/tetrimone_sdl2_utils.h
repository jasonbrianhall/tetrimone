#ifndef TETRIMONE_SDL2_UTILS_H
#define TETRIMONE_SDL2_UTILS_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <array>

// Color utilities
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(255), g(255), b(255), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    
    static Color from_double(double r, double g, double b, double a = 1.0) {
        return Color(
            static_cast<uint8_t>(r * 255),
            static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255),
            static_cast<uint8_t>(a * 255)
        );
    }
    
    static Color from_array(const std::array<double, 3>& arr) {
        return from_double(arr[0], arr[1], arr[2]);
    }
    
    SDL_Color to_sdl() const {
        return SDL_Color{r, g, b, a};
    }
};

// Predefined colors
namespace Colors {
    constexpr Color Black(0, 0, 0);
    constexpr Color White(255, 255, 255);
    constexpr Color Red(255, 0, 0);
    constexpr Color Green(0, 255, 0);
    constexpr Color Blue(0, 0, 255);
    constexpr Color Yellow(255, 255, 0);
    constexpr Color Cyan(0, 255, 255);
    constexpr Color Magenta(255, 0, 255);
    constexpr Color Gray(128, 128, 128);
    
    // Tetrimino colors (standard)
    constexpr Color Cyan_Block(0, 240, 240);      // I
    constexpr Color Yellow_Block(240, 240, 0);    // O
    constexpr Color Purple_Block(160, 0, 240);    // T
    constexpr Color Green_Block(0, 240, 0);       // S
    constexpr Color Red_Block(240, 0, 0);         // Z
    constexpr Color Blue_Block(0, 0, 240);        // J
    constexpr Color Orange_Block(240, 160, 0);    // L
}

// Rectangle utilities
struct Rect {
    int x, y, w, h;
    
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    
    SDL_Rect to_sdl() const {
        return SDL_Rect{x, y, w, h};
    }
    
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    
    bool intersects(const Rect& other) const {
        return !(x + w < other.x || x > other.x + other.w ||
                 y + h < other.y || y > other.y + other.h);
    }
};

// File utilities
class FileUtils {
public:
    static bool file_exists(const std::string& path) {
        FILE* file = fopen(path.c_str(), "r");
        if (file) {
            fclose(file);
            return true;
        }
        return false;
    }
    
    static std::string get_executable_dir() {
        char result[256];
        
        #ifdef _WIN32
            GetModuleFileNameA(NULL, result, sizeof(result));
            std::string path(result);
            return path.substr(0, path.find_last_of("\\/"));
        #else
            ssize_t count = readlink("/proc/self/exe", result, sizeof(result) - 1);
            if (count != -1) {
                result[count] = '\0';
                std::string path(result);
                return path.substr(0, path.find_last_of("\\/"));
            }
            return ".";
        #endif
    }
    
    static std::string find_asset(const std::string& assetName) {
        // Try relative path
        std::string path = "assets/" + assetName;
        if (file_exists(path)) return path;
        
        // Try executable directory
        path = get_executable_dir() + "/assets/" + assetName;
        if (file_exists(path)) return path;
        
        // Try one level up
        path = "../assets/" + assetName;
        if (file_exists(path)) return path;
        
        // Return original request
        return assetName;
    }
};

// Sound effect manager (placeholder for integration with AudioManager)
class SoundEffectCache {
private:
    std::string baseDir;
    bool initialized;
    
public:
    SoundEffectCache() : baseDir("assets/sounds"), initialized(false) {}
    
    bool init(const std::string& directory) {
        baseDir = directory;
        initialized = true;
        return FileUtils::file_exists(baseDir);
    }
    
    std::string get_sound_path(const std::string& soundName) {
        return baseDir + "/" + soundName;
    }
};

// Text renderer (for caching text surfaces)
class TextCache {
private:
    struct CachedText {
        std::string text;
        SDL_Texture* texture;
        int width, height;
    };
    
    std::vector<CachedText> cache;
    SDL_Renderer* renderer;
    TTF_Font* font;
    
public:
    TextCache(SDL_Renderer* r, TTF_Font* f) : renderer(r), font(f) {}
    
    ~TextCache() {
        clear();
    }
    
    SDL_Texture* get_or_render(const std::string& text, SDL_Color color) {
        // Check cache
        for (auto& cached : cache) {
            if (cached.text == text) {
                return cached.texture;
            }
        }
        
        // Render new text
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (!surface) return nullptr;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        cache.push_back({text, texture, surface->w, surface->h});
        SDL_FreeSurface(surface);
        
        return texture;
    }
    
    void clear() {
        for (auto& cached : cache) {
            if (cached.texture) {
                SDL_DestroyTexture(cached.texture);
            }
        }
        cache.clear();
    }
};

// Animation helper
class Animation {
private:
    double startTime;
    double duration;
    double currentTime;
    bool isPlaying;
    bool isLooping;
    
public:
    Animation(double duration) : startTime(0), duration(duration), currentTime(0), 
                                 isPlaying(false), isLooping(false) {}
    
    void start(bool loop = false) {
        startTime = SDL_GetTicks() / 1000.0;
        currentTime = 0;
        isPlaying = true;
        isLooping = loop;
    }
    
    void stop() {
        isPlaying = false;
    }
    
    void update() {
        if (!isPlaying) return;
        
        double now = SDL_GetTicks() / 1000.0;
        currentTime = now - startTime;
        
        if (currentTime >= duration) {
            if (isLooping) {
                startTime = now;
                currentTime = 0;
            } else {
                isPlaying = false;
                currentTime = duration;
            }
        }
    }
    
    double getProgress() const {
        return std::min(1.0, currentTime / duration);
    }
    
    bool isFinished() const {
        return !isPlaying && currentTime >= duration;
    }
    
    bool isActive() const {
        return isPlaying;
    }
};

// Easing functions
namespace Easing {
    inline double linear(double t) {
        return t;
    }
    
    inline double easeInQuad(double t) {
        return t * t;
    }
    
    inline double easeOutQuad(double t) {
        return t * (2.0 - t);
    }
    
    inline double easeInOutQuad(double t) {
        return t < 0.5 ? 2.0 * t * t : -1.0 + (4.0 - 2.0 * t) * t;
    }
    
    inline double easeInCubic(double t) {
        return t * t * t;
    }
    
    inline double easeOutCubic(double t) {
        return 1.0 + (--t) * t * t;
    }
    
    inline double easeInOutCubic(double t) {
        return t < 0.5 ? 4.0 * t * t * t : 1.0 + (--t) * 2.0 * (--t) * (--t);
    }
    
    inline double easeOutElastic(double t) {
        return sin(-13.0 * M_PI_2 * (t + 1.0)) * pow(2.0, -10.0 * t) + 1.0;
    }
}

// Performance profiler (debug)
class PerformanceProfiler {
private:
    struct Timer {
        std::string name;
        uint32_t startTime;
        uint32_t totalTime;
        int frameCount;
    };
    
    std::vector<Timer> timers;
    
public:
    void startTimer(const std::string& name) {
        for (auto& timer : timers) {
            if (timer.name == name) {
                timer.startTime = SDL_GetTicks();
                return;
            }
        }
        timers.push_back({name, SDL_GetTicks(), 0, 0});
    }
    
    void endTimer(const std::string& name) {
        for (auto& timer : timers) {
            if (timer.name == name && timer.startTime > 0) {
                uint32_t elapsed = SDL_GetTicks() - timer.startTime;
                timer.totalTime += elapsed;
                timer.frameCount++;
                timer.startTime = 0;
            }
        }
    }
    
    void printStats() {
        printf("=== Performance Stats ===\n");
        for (const auto& timer : timers) {
            if (timer.frameCount > 0) {
                double avgTime = (double)timer.totalTime / timer.frameCount;
                printf("%s: %.2f ms (avg of %d frames)\n", 
                       timer.name.c_str(), avgTime, timer.frameCount);
            }
        }
    }
    
    void reset() {
        for (auto& timer : timers) {
            timer.totalTime = 0;
            timer.frameCount = 0;
        }
    }
};

#endif // TETRIMONE_SDL2_UTILS_H
