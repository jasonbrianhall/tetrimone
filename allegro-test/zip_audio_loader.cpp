// zip_audio_loader.cpp - Implementation of ZIP audio loading system
#include "zip_audio_loader.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ZIP file format constants
#define ZIP_LOCAL_FILE_HEADER_SIG 0x04034b50
#define ZIP_CENTRAL_DIR_SIG 0x02014b50
#define ZIP_END_CENTRAL_DIR_SIG 0x06054b50

ZipAudioLoader::ZipAudioLoader() : loaded(false) {}

ZipAudioLoader::~ZipAudioLoader() {
    cleanup();
}

bool ZipAudioLoader::loadZipFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open ZIP file: " << filename << std::endl;
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read entire file into memory
    zipData.resize(fileSize);
    file.read(reinterpret_cast<char*>(zipData.data()), fileSize);
    file.close();
    
    return parseZipFile(zipData);
}

bool ZipAudioLoader::loadZipFromMemory(const uint8_t* data, size_t size) {
    zipData.assign(data, data + size);
    return parseZipFile(zipData);
}

bool ZipAudioLoader::parseZipFile(const std::vector<uint8_t>& data) {
    if (data.size() < 22) return false; // Too small for ZIP
    
    // Find end of central directory record
    size_t endDirOffset = data.size() - 22;
    while (endDirOffset > 0) {
        if (readUint32(data.data(), endDirOffset) == ZIP_END_CENTRAL_DIR_SIG) {
            break;
        }
        endDirOffset--;
    }
    
    if (readUint32(data.data(), endDirOffset) != ZIP_END_CENTRAL_DIR_SIG) {
        std::cerr << "Invalid ZIP file: End of central directory not found" << std::endl;
        return false;
    }
    
    // Read central directory info
    uint16_t numEntries = readUint16(data.data(), endDirOffset + 10);
    uint32_t centralDirSize = readUint32(data.data(), endDirOffset + 12);
    uint32_t centralDirOffset = readUint32(data.data(), endDirOffset + 16);
    
    // Parse central directory entries
    size_t offset = centralDirOffset;
    for (uint16_t i = 0; i < numEntries && offset < data.size(); i++) {
        if (readUint32(data.data(), offset) != ZIP_CENTRAL_DIR_SIG) {
            break;
        }
        
        uint16_t filenameLen = readUint16(data.data(), offset + 28);
        uint16_t extraLen = readUint16(data.data(), offset + 30);
        uint16_t commentLen = readUint16(data.data(), offset + 32);
        uint32_t localHeaderOffset = readUint32(data.data(), offset + 42);
        
        // Extract filename
        std::string filename(reinterpret_cast<const char*>(data.data() + offset + 46), filenameLen);
        
        // Parse local file header to get actual data
        size_t localOffset = localHeaderOffset;
        if (readUint32(data.data(), localOffset) == ZIP_LOCAL_FILE_HEADER_SIG) {
            uint16_t localFilenameLen = readUint16(data.data(), localOffset + 26);
            uint16_t localExtraLen = readUint16(data.data(), localOffset + 28);
            uint32_t compressedSize = readUint32(data.data(), localOffset + 18);
            uint32_t uncompressedSize = readUint32(data.data(), localOffset + 22);
            
            ZipFileEntry entry;
            entry.filename = filename;
            entry.compressed_size = compressedSize;
            entry.uncompressed_size = uncompressedSize;
            entry.offset = localOffset + 30 + localFilenameLen + localExtraLen;
            
            entries[filename] = entry;
            std::cout << "Found in ZIP: " << filename << " (" << compressedSize << " bytes)" << std::endl;
        }
        
        offset += 46 + filenameLen + extraLen + commentLen;
    }
    
    loaded = true;
    std::cout << "ZIP file loaded successfully with " << entries.size() << " files" << std::endl;
    return true;
}

bool ZipAudioLoader::extractFile(const std::string& filename, std::vector<uint8_t>& output) {
    auto it = entries.find(filename);
    if (it == entries.end()) {
        return false;
    }
    
    const ZipFileEntry& entry = it->second;
    
    // For simplicity, we only support stored (uncompressed) files
    // In a full implementation, you'd add deflate decompression
    if (entry.compressed_size == entry.uncompressed_size) {
        output.resize(entry.uncompressed_size);
        std::copy(zipData.begin() + entry.offset, 
                 zipData.begin() + entry.offset + entry.compressed_size,
                 output.begin());
        return true;
    }
    
    std::cerr << "Compressed files not supported yet: " << filename << std::endl;
    return false;
}

bool ZipAudioLoader::extractAudioFile(const std::string& filename, std::string& tempPath) {
    std::vector<uint8_t> fileData;
    if (!extractFile(filename, fileData)) {
        return false;
    }
    
    // Create temporary file with DOS-compatible name for extraction
    static int tempCounter = 0;
    char tempName[13]; // 8.3 format max
    sprintf(tempName, "aud%05d.tmp", tempCounter++);
    
    std::ofstream tempFile(tempName, std::ios::binary);
    if (!tempFile.is_open()) {
        return false;
    }
    
    tempFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
    tempFile.close();
    
    tempPath = tempName;
    return true;
}

std::vector<std::string> ZipAudioLoader::getFileList() const {
    std::vector<std::string> files;
    for (const auto& entry : entries) {
        files.push_back(entry.first);
    }
    return files;
}

bool ZipAudioLoader::hasFile(const std::string& filename) const {
    return entries.find(filename) != entries.end();
}

void ZipAudioLoader::cleanup() {
    entries.clear();
    zipData.clear();
    loaded = false;
}

uint32_t ZipAudioLoader::readUint32(const uint8_t* data, size_t offset) {
    return data[offset] | (data[offset + 1] << 8) | 
           (data[offset + 2] << 16) | (data[offset + 3] << 24);
}

uint16_t ZipAudioLoader::readUint16(const uint8_t* data, size_t offset) {
    return data[offset] | (data[offset + 1] << 8);
}

// ZipSoundManager implementation
ZipSoundManager::ZipSoundManager() {
    zipLoader = new ZipAudioLoader();
    soundEnabled = true;
    musicEnabled = true;
    masterVolume = 255;
    musicVolume = 200;
    sfxVolume = 255;
    musicPlaying = false;
    musicVoice = -1;
    currentMusic = "";
}

ZipSoundManager::~ZipSoundManager() {
    shutdown();
    delete zipLoader;
}

bool ZipSoundManager::initialize() {
    if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) != 0) {
        std::cerr << "Failed to initialize sound system: " << allegro_error << std::endl;
        return false;
    }
    
    set_volume(masterVolume, musicVolume);
    std::cout << "ZIP Sound system initialized successfully" << std::endl;
    return true;
}

bool ZipSoundManager::loadAudioZip(const std::string& zipFilename) {
    if (!zipLoader->loadZipFile(zipFilename)) {
        std::cerr << "Failed to load audio ZIP: " << zipFilename << std::endl;
        return false;
    }
    
    return loadAllSoundsFromZip();
}

// DOS 8.3 filename mapper
std::string ZipSoundManager::getDosCompatibleName(const std::string& originalName) {
    // Create a mapping table for long filenames to DOS 8.3 format
    static std::map<std::string, std::string> filenameMap = {
        // Sound effects
        {"drop.mp3", "drop.mp3"},
        {"rotate.mp3", "rotate.mp3"},
        {"lateralmove.mp3", "move.mp3"},
        {"single.mp3", "single.mp3"},
        {"double.mp3", "double.mp3"},
        {"triple.mp3", "triple.mp3"},
        {"excellent.mp3", "excel.mp3"},
        {"levelup.mp3", "levelup.mp3"},
        {"gameover.mp3", "gameover.mp3"},
        {"start.mp3", "start.mp3"},
        {"select.mp3", "select.mp3"},
        {"clear.mp3", "clear.mp3"},
        {"tetrimone.mp3", "tetrmone.mp3"},
        {"levelupretro.mp3", "levelrtr.mp3"},
        {"gameoverretro.mp3", "gameortr.mp3"},
        
        // Music files
        {"title-screen.mp3", "title.mp3"},
        {"TetrimoneA.mp3", "tetra.mp3"},
        {"TetrimoneB.mp3", "tetrb.mp3"},
        {"TetrimoneC.mp3", "tetrc.mp3"},
        {"theme.mp3", "theme.mp3"},
        {"futuristic.mp3", "future.mp3"},
        {"airforce.mp3", "airforce.mp3"},
        {"americathebeautiful.mp3", "america.mp3"},
        {"grandoldflag.mp3", "grandflag.mp3"},
        {"johnny.mp3", "johnny.mp3"},
        {"riverkwai.mp3", "riverkwai.mp3"}
    };
    
    auto it = filenameMap.find(originalName);
    if (it != filenameMap.end()) {
        return it->second;
    }
    
    // If not found, try to auto-generate DOS name
    std::string dosName = originalName;
    if (dosName.length() > 12) { // 8.3 = 12 chars max with dot
        size_t dotPos = dosName.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string name = dosName.substr(0, dotPos);
            std::string ext = dosName.substr(dotPos);
            
            if (name.length() > 8) {
                name = name.substr(0, 8);
            }
            if (ext.length() > 4) {
                ext = ext.substr(0, 4);
            }
            dosName = name + ext;
        }
    }
    
    return dosName;
}

bool ZipSoundManager::loadSampleFromZip(const std::string& name, const std::string& zipFilename) {
    std::string tempPath;
    if (!zipLoader->extractAudioFile(zipFilename, tempPath)) {
        return false;
    }
    
    SAMPLE* sample = nullptr;
    
    // Try to load the file - Allegro 4 should handle WAV, and some builds support MP3
    sample = load_sample(tempPath.c_str());
    
    if (!sample) {
        // If direct loading failed, try loading as WAV/VOC format
        std::cerr << "Failed to load sample from temp file: " << tempPath << std::endl;
        std::cerr << "Note: MP3 support may be limited in this Allegro build" << std::endl;
        remove(tempPath.c_str());
        return false;
    }
    
    // Free existing sample if it exists
    if (samples[name]) {
        destroy_sample(samples[name]);
    }
    
    samples[name] = sample;
    tempFiles[name] = tempPath;
    
    std::cout << "Loaded sample from ZIP: " << name << " (" << zipFilename << ")" << std::endl;
    return true;
}

bool ZipSoundManager::loadAllSoundsFromZip() {
    bool allLoaded = true;
    
    std::cout << "Loading audio files from ZIP..." << std::endl;
    
    // Load sound effects - try MP3 first, fallback to WAV
    allLoaded &= loadSampleFromZip("drop", "drop.mp3") || loadSampleFromZip("drop", "drop.wav");
    allLoaded &= loadSampleFromZip("rotate", "rotate.mp3") || loadSampleFromZip("rotate", "rotate.wav");
    allLoaded &= loadSampleFromZip("move", "lateralmove.mp3") || loadSampleFromZip("move", "lateralmove.wav");
    allLoaded &= loadSampleFromZip("single", "single.mp3") || loadSampleFromZip("single", "single.wav");
    allLoaded &= loadSampleFromZip("double", "double.mp3") || loadSampleFromZip("double", "double.wav");
    allLoaded &= loadSampleFromZip("triple", "triple.mp3") || loadSampleFromZip("triple", "triple.wav");
    allLoaded &= loadSampleFromZip("excellent", "excellent.mp3") || loadSampleFromZip("excellent", "excellent.wav");
    allLoaded &= loadSampleFromZip("levelup", "levelup.mp3") || loadSampleFromZip("levelup", "levelup.wav");
    allLoaded &= loadSampleFromZip("gameover", "gameover.mp3") || loadSampleFromZip("gameover", "gameover.wav");
    allLoaded &= loadSampleFromZip("start", "start.mp3") || loadSampleFromZip("start", "start.wav");
    allLoaded &= loadSampleFromZip("select", "select.mp3") || loadSampleFromZip("select", "select.wav");
    allLoaded &= loadSampleFromZip("clear", "clear.mp3") || loadSampleFromZip("clear", "clear.wav");
    allLoaded &= loadSampleFromZip("tetrimone", "tetrimone.mp3") || loadSampleFromZip("tetrimone", "tetrimone.wav");
    
    // Load retro versions (MP3 only)
    loadSampleFromZip("levelup_retro", "levelupretro.mp3");
    loadSampleFromZip("gameover_retro", "gameoverretro.mp3");
    
    // Load music files - try MP3 first, fallback to MID
    std::cout << "Loading music files from ZIP..." << std::endl;
    loadSampleFromZip("title", "title-screen.mp3") || loadSampleFromZip("title", "title-screen.mid");
    loadSampleFromZip("tetrimone_a", "TetrimoneA.mp3") || loadSampleFromZip("tetrimone_a", "TetrimoneA.mid");
    loadSampleFromZip("tetrimone_b", "TetrimoneB.mp3") || loadSampleFromZip("tetrimone_b", "TetrimoneB.mid");
    loadSampleFromZip("tetrimone_c", "TetrimoneC.mp3") || loadSampleFromZip("tetrimone_c", "TetrimoneC.mid");
    loadSampleFromZip("theme", "theme.mp3") || loadSampleFromZip("theme", "theme.mid");
    loadSampleFromZip("futuristic", "futuristic.mp3") || loadSampleFromZip("futuristic", "futuristic.mid");
    loadSampleFromZip("airforce", "airforce.mp3") || loadSampleFromZip("airforce", "airforce.mid");
    loadSampleFromZip("america", "americathebeautiful.mp3") || loadSampleFromZip("america", "americathebeautiful.mid");
    loadSampleFromZip("grand_old_flag", "grandoldflag.mp3") || loadSampleFromZip("grand_old_flag", "grandoldflag.mid");
    loadSampleFromZip("johnny", "johnny.mp3") || loadSampleFromZip("johnny", "johnny.mid");
    loadSampleFromZip("river_kwai", "riverkwai.mp3") || loadSampleFromZip("river_kwai", "riverkwai.mid");
    
    std::cout << "Loaded " << samples.size() << " audio files from ZIP" << std::endl;
    
    // Display audio format capabilities
    std::cout << "Audio format support:" << std::endl;
    std::cout << "- WAV: Yes (native Allegro support)" << std::endl;
    std::cout << "- MP3: " << (samples.size() > 0 ? "Detected" : "Limited") << " (depends on Allegro build)" << std::endl;
    std::cout << "- MID: Fallback available" << std::endl;
    
    return allLoaded;
}

void ZipSoundManager::shutdown() {
    stopMusic();
    
    // Free all samples
    for (auto& pair : samples) {
        if (pair.second) {
            destroy_sample(pair.second);
        }
    }
    samples.clear();
    
    // Clean up temp files
    for (const auto& pair : tempFiles) {
        remove(pair.second.c_str());
    }
    tempFiles.clear();
    
    remove_sound();
}

void ZipSoundManager::playSample(const std::string& name, int volume, int pan, int freq) {
    if (!soundEnabled) return;
    
    auto it = samples.find(name);
    if (it != samples.end() && it->second) {
        int adjustedVolume = (volume * sfxVolume * masterVolume) / (255 * 255);
        play_sample(it->second, adjustedVolume, pan, freq, 0);
    }
}

void ZipSoundManager::playMusic(const std::string& name, bool loop) {
    if (!musicEnabled) return;
    
    stopMusic();
    
    auto it = samples.find(name);
    if (it != samples.end() && it->second) {
        int adjustedVolume = (musicVolume * masterVolume) / 255;
        musicVoice = play_sample(it->second, adjustedVolume, 128, 1000, loop ? 1 : 0);
        if (musicVoice != -1) {
            currentMusic = name;
            musicPlaying = true;
            std::cout << "Playing music from ZIP: " << name << std::endl;
        }
    }
}

void ZipSoundManager::stopMusic() {
    if (musicPlaying && musicVoice != -1) {
        stop_sample(samples[currentMusic]);
        musicPlaying = false;
        musicVoice = -1;
        currentMusic = "";
    }
}

void ZipSoundManager::pauseMusic() {
    // Basic implementation - just stop for now
    if (musicPlaying) {
        stopMusic();
    }
}

void ZipSoundManager::resumeMusic() {
    if (!currentMusic.empty() && musicEnabled) {
        playMusic(currentMusic, true);
    }
}

void ZipSoundManager::setMasterVolume(int volume) {
    masterVolume = std::max(0, std::min(255, volume));
    set_volume(masterVolume, musicVolume);
}

void ZipSoundManager::setMusicVolume(int volume) {
    musicVolume = std::max(0, std::min(255, volume));
    set_volume(masterVolume, musicVolume);
}

void ZipSoundManager::setSFXVolume(int volume) {
    sfxVolume = std::max(0, std::min(255, volume));
}

void ZipSoundManager::setSoundEnabled(bool enabled) {
    soundEnabled = enabled;
}

void ZipSoundManager::setMusicEnabled(bool enabled) {
    musicEnabled = enabled;
    if (!enabled && musicPlaying) {
        stopMusic();
    }
}

// Game-specific sound functions
void ZipSoundManager::playGameStart() {
    playSample("start");
}

void ZipSoundManager::playPieceDrop() {
    playSample("drop", 200);
}

void ZipSoundManager::playPieceRotate() {
    playSample("rotate", 150);
}

void ZipSoundManager::playPieceMove() {
    playSample("move", 100);
}

void ZipSoundManager::playLineClear(int lines) {
    switch (lines) {
        case 1: playSample("single"); break;
        case 2: playSample("double"); break;
        case 3: playSample("triple"); break;
        case 4:
        default: playSample("excellent"); break;
    }
}

void ZipSoundManager::playLevelUp() {
    if (samples.find("levelup_retro") != samples.end()) {
        playSample("levelup_retro");
    } else {
        playSample("levelup");
    }
}

void ZipSoundManager::playGameOver() {
    if (samples.find("gameover_retro") != samples.end()) {
        playSample("gameover_retro");
    } else {
        playSample("gameover");
    }
}

void ZipSoundManager::playThemeMusic(int theme) {
    std::vector<std::string> musicTracks = {
        "tetrimone_a", "tetrimone_b", "tetrimone_c", "theme",
        "futuristic", "airforce", "america", "grand_old_flag",
        "johnny", "river_kwai"
    };
    
    if (!musicTracks.empty()) {
        std::string track = musicTracks[theme % musicTracks.size()];
        if (currentMusic != track) {
            playMusic(track, true);
        }
    }
}

void ZipSoundManager::playMenuSelect() {
    playSample("select", 180);
}
