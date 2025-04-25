#ifndef AUDIO_CONVERTER_H
#define AUDIO_CONVERTER_H

#include <vector>

// Function to convert MP3 data to WAV data in memory
bool convertMp3ToWavInMemory(const std::vector<uint8_t>& mp3Data, std::vector<uint8_t>& wavData);

// Function to convert MIDI data to WAV data in memory
bool convertMidiToWavInMemory(const std::vector<uint8_t>& midiData, std::vector<uint8_t>& wavData);

#endif // AUDIO_CONVERTER_H
