#include <iostream>
#include <vector>
#include <mpg123.h>
#include <sndfile.h>
#include <cstring>
#include <cstdint>
#include "audioconverter.h"


// Simple but reliable MP3 to WAV conversion
bool convertMp3ToWavInMemory(const std::vector<uint8_t>& mp3Data, std::vector<uint8_t>& wavData) {
    // Initialize mpg123
    mpg123_init();
    
    int err = MPG123_OK;
    mpg123_handle* mh = mpg123_new(nullptr, &err);
    if (!mh) {
        std::cerr << "Error creating mpg123 handle: " << mpg123_plain_strerror(err) << std::endl;
        return false;
    }
    
    // Clean up resources properly
    struct Cleanup {
        mpg123_handle* handle;
        Cleanup(mpg123_handle* h) : handle(h) {}
        ~Cleanup() {
            mpg123_close(handle);
            mpg123_delete(handle);
            mpg123_exit();
        }
    } cleanup(mh);
    
    // Open feed and decode
    if (mpg123_open_feed(mh) != MPG123_OK) {
        std::cerr << "Cannot open MP3 feed: " << mpg123_strerror(mh) << std::endl;
        return false;
    }
    
    // Feed all data at once
    if (mpg123_feed(mh, mp3Data.data(), mp3Data.size()) != MPG123_OK) {
        std::cerr << "Error feeding MP3 data: " << mpg123_strerror(mh) << std::endl;
        return false;
    }
    
    // Get audio format
    long rate;
    int channels, encoding;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        std::cerr << "Cannot get MP3 format: " << mpg123_strerror(mh) << std::endl;
        return false;
    }
    
    // Force standard output format
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, MPG123_ENC_SIGNED_16);
    
    // Decode the entire MP3 to a temporary buffer
    std::vector<short> pcmData;
    size_t buffer_size = mpg123_outblock(mh);
    std::vector<unsigned char> buffer(buffer_size);
    
    while (true) {
        size_t done = 0;
        int ret = mpg123_read(mh, buffer.data(), buffer_size, &done);
        
        if (ret == MPG123_OK || ret == MPG123_DONE) {
            if (done > 0) {
                // Convert to shorts and add to PCM data
                size_t samples = done / sizeof(short);
                size_t oldSize = pcmData.size();
                pcmData.resize(oldSize + samples);
                memcpy(pcmData.data() + oldSize, buffer.data(), done);
            }
            
            if (ret == MPG123_DONE) {
                break;
            }
        } 
        else {
            std::cerr << "Error decoding MP3: " << mpg123_strerror(mh) << std::endl;
            if (pcmData.empty()) {
                return false;
            }
            break;
        }
    }
    
    // If no audio data was decoded, fail
    if (pcmData.empty()) {
        std::cerr << "No audio data decoded from MP3" << std::endl;
        return false;
    }
    
    // Now create a WAV file from the PCM data
    // Simple WAV file creation without relying on libsndfile
    
    // WAV header
    struct WavHeader {
        // RIFF header
        char riff_header[4] = {'R', 'I', 'F', 'F'};
        uint32_t wave_size;        // File size - 8
        char wave_header[4] = {'W', 'A', 'V', 'E'};
        
        // Format chunk
        char fmt_header[4] = {'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels;
        uint32_t sample_rate;
        uint32_t byte_rate;       // sample_rate * num_channels * bits_per_sample / 8
        uint16_t block_align;     // num_channels * bits_per_sample / 8
        uint16_t bits_per_sample = 16;
        
        // Data chunk
        char data_header[4] = {'d', 'a', 't', 'a'};
        uint32_t data_bytes;      // num_samples * num_channels * bits_per_sample / 8
    };
    
    // Calculate WAV sizes
    uint32_t data_bytes = pcmData.size() * sizeof(short);
    uint32_t wav_size = 36 + data_bytes;
    
    // Create header
    WavHeader header;
    header.wave_size = wav_size;
    header.num_channels = channels;
    header.sample_rate = rate;
    header.byte_rate = rate * channels * 16 / 8;
    header.block_align = channels * 16 / 8;
    header.data_bytes = data_bytes;
    
    // Create WAV data
    wavData.resize(sizeof(header) + data_bytes);
    
    // Copy header
    memcpy(wavData.data(), &header, sizeof(header));
    
    // Copy audio data
    memcpy(wavData.data() + sizeof(header), pcmData.data(), data_bytes);
    
    std::cout << "Generated WAV: " << wavData.size() << " bytes, sample rate: " << rate << 
              ", channels: " << channels << std::endl;
    
    return true;
}
