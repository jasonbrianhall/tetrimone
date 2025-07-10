// zip_audio_loader.h - Load audio files from ZIP archive in memory
#ifndef ZIP_AUDIO_LOADER_H
#define ZIP_AUDIO_LOADER_H

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <allegro.h>


// Simple ZIP file reader for audio files
class ZipAudioLoader {
private:
    struct ZipFileEntry {
        std::string filename;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint32_t offset;
        std::vector<uint8_t> data;
    };
    
    std::map<std::string, ZipFileEntry> entries;
    std::vector<uint8_t> zipData;
    bool loaded;
    
    // ZIP file parsing
    bool parseZipFile(const std::vector<uint8_t>& data);
    bool extractFile(const std::string& filename, std::vector<uint8_t>& output);
    uint32_t readUint32(const uint8_t* data, size_t offset);
    uint16_t readUint16(const uint8_t* data, size_t offset);
    
    // Decompression (simple stored files only for now)
    bool decompressStored(const uint8_t* input, size_t inputSize, 
                         std::vector<uint8_t>& output, size_t outputSize);

public:
    ZipAudioLoader();
    ~ZipAudioLoader();
    
    // Load ZIP file into memory
    bool loadZipFile(const std::string& filename);
    bool loadZipFromMemory(const uint8_t* data, size_t size);
    
    // Extract audio file to temporary file for Allegro
    bool extractAudioFile(const std::string& filename, std::string& tempPath);
    
    // Get list of files in ZIP
    std::vector<std::string> getFileList() const;
    
    // Check if file exists in ZIP
    bool hasFile(const std::string& filename) const;
    
    // Cleanup
    void cleanup();
};

// Audio manager that uses ZIP loader
class ZipSoundManager {
private:
    ZipAudioLoader* zipLoader;
    std::map<std::string, SAMPLE*> samples;
    std::map<std::string, std::string> tempFiles; // Track temp files for cleanup
    bool soundEnabled;
    bool musicEnabled;
    int masterVolume;
    int musicVolume;
    int sfxVolume;
    
    std::string currentMusic;
    bool musicPlaying;
    int musicVoice; // Track the voice playing music
    
    // DOS 8.3 filename compatibility
    std::string getDosCompatibleName(const std::string& originalName);

public:
    ZipSoundManager();
    ~ZipSoundManager();
    
    // Initialization
    bool initialize();
    bool loadAudioZip(const std::string& zipFilename);
    void shutdown();
    
    // Audio loading from ZIP
    bool loadSampleFromZip(const std::string& name, const std::string& zipFilename);
    bool loadAllSoundsFromZip();
    
    // Playback functions
    void playSample(const std::string& name, int volume = 255, int pan = 128, int freq = 1000);
    void playMusic(const std::string& name, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    
    // Volume control
    void setMasterVolume(int volume);
    void setMusicVolume(int volume);
    void setSFXVolume(int volume);
    void setSoundEnabled(bool enabled);
    void setMusicEnabled(bool enabled);
    
    // Getters
    bool isSoundEnabled() const { return soundEnabled; }
    bool isMusicEnabled() const { return musicEnabled; }
    int getMasterVolume() const { return masterVolume; }
    int getMusicVolume() const { return musicVolume; }
    int getSFXVolume() const { return sfxVolume; }
    
    // Game-specific sound functions
    void playGameStart();
    void playPieceDrop();
    void playPieceRotate();
    void playPieceMove();
    void playLineClear(int lines);
    void playLevelUp();
    void playGameOver();
    void playThemeMusic(int theme);
    void playMenuSelect();
};

#endif // ZIP_AUDIO_LOADER_H
