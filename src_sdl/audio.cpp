#include <iostream>
#include <vector>
#include <mpg123.h>
#include <sndfile.h>

int convert_mp3_to_wav(const char* input_file, const char* output_file) {
    // Initialize mpg123
    mpg123_handle* mh;
    int err = MPG123_OK;
    
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    if (mh == NULL) {
        std::cerr << "Error initializing mpg123: " << mpg123_plain_strerror(err) << std::endl;
        return -1;
    }
    
    // Open the MP3 file
    if (mpg123_open(mh, input_file) != MPG123_OK) {
        std::cerr << "Cannot open MP3 file: " << mpg123_strerror(mh) << std::endl;
        mpg123_delete(mh);
        mpg123_exit();
        return -1;
    }
    
    // Get the audio format information
    long rate;
    int channels, encoding;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        std::cerr << "Cannot get format: " << mpg123_strerror(mh) << std::endl;
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return -1;
    }
    
    // Force output format
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, encoding);
    
    // Get buffer size and allocate buffer
    size_t buffer_size = mpg123_outblock(mh);
    unsigned char* buffer = new unsigned char[buffer_size];
    
    // Set up output WAV file
    SF_INFO sfinfo;
    sfinfo.samplerate = rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    
    SNDFILE* outfile = sf_open(output_file, SFM_WRITE, &sfinfo);
    if (!outfile) {
        std::cerr << "Cannot open output WAV file: " << sf_strerror(outfile) << std::endl;
        delete[] buffer;
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return -1;
    }
    
    // Read and convert
    size_t done;
    while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
        if (done == 0) break;
        
        // Convert buffer to float for libsndfile (assuming output is 16-bit PCM)
        std::vector<short> samples(done / sizeof(short));
        short* short_buffer = reinterpret_cast<short*>(buffer);
        for (size_t i = 0; i < done / sizeof(short); i++) {
            samples[i] = short_buffer[i];
        }
        
        // Write to WAV file
        sf_writef_short(outfile, samples.data(), samples.size() / channels);
    }
    
    // Clean up
    sf_close(outfile);
    delete[] buffer;
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    
    std::cout << "Successfully converted " << input_file << " to " << output_file << std::endl;
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input.mp3 output.wav" << std::endl;
        return 1;
    }
    
    return convert_mp3_to_wav(argv[1], argv[2]);
}
