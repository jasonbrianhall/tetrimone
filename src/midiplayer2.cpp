// Add these includes at the top of the file, after the existing includes
#ifdef _WIN32
#include <windows.h>
#include <conio.h>  // For _kbhit() and _getch()
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

// Replace the global termios declaration with:
#ifndef _WIN32
struct termios old_tio;
#endif
volatile sig_atomic_t keep_running = 1;

// Replace the kbhit() function with this cross-platform version:
int kbhit() {
#ifdef _WIN32
    return _kbhit();
#else
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
#endif
}

// Add a cross-platform getch() function
int getch() {
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return ch;
#endif
}

// Replace the handle_sigint function with:
#ifdef _WIN32
BOOL WINAPI handle_console_ctrl(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT) {
        keep_running = 0;
        
        // Stop audio and perform cleanup
        isPlaying = false;
        SDL_PauseAudioDevice(audioDevice, 1);
        
        printf("\nPlayback interrupted. Cleaning up...\n");
        cleanup();
        
        // Exit the program
        exit(0);
        return TRUE;
    }
    
    return FALSE;
}
#else
void handle_sigint(int sig) {
    keep_running = 0;
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    
    // Stop audio and perform cleanup
    isPlaying = false;
    SDL_PauseAudioDevice(audioDevice, 1);
    
    printf("\nPlayback interrupted. Cleaning up...\n");
    cleanup();
    
    // Exit the program
    exit(0);
}
#endif

// Modify the playMidiFile function's terminal handling section:
void playMidiFile() {
    // Initialize variables for all channels
    for (int i = 0; i < 16; i++) {
        ChPatch[i] = 0;
        ChBend[i] = 0;
        ChVolume[i] = 127;
        ChPanning[i] = 64;
        ChVibrato[i] = 0;
    }
    
    // Reset playback state
    playTime = 0;
    isPlaying = true;
    paused = false;
    loopStart = false;
    loopEnd = false;
    playwait = 0;
    loopwait = 0;
    
    // Reset all OPL channels
    OPL_Reset();
    
    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
    
    printf("Playback started. Press:\n");
    printf("  q - Quit\n");
    printf("  Space - Pause/Resume\n");
    printf("  +/- - Increase/Decrease Volume\n");
    printf("  n - Toggle Volume Normalization\n");
    printf("  Ctrl+C - Stop Playback\n");
    
    // Set up terminal for non-blocking input
#ifdef _WIN32
    // Set up Windows console control handler
    SetConsoleCtrlHandler(handle_console_ctrl, TRUE);
#else
    struct termios new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    
    // Set up signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_sigint);
#endif
    
    // Reset keep_running flag
    keep_running = 1;
    
    // Main loop - handle console input
    while (isPlaying && keep_running) {
        // Check for key press without blocking
        if (kbhit()) {
            int ch = getch();  // Use our cross-platform getch function
            switch (ch) {
                case ' ':
                    paused = !paused;
                    printf("%s\n", paused ? "Paused" : "Resumed");
                    break;
                case 'q':
                    isPlaying = false;
                    break;
                case '+':
                case '=':
                    updateVolume(10);
                    break;
                case '-':
                    updateVolume(-10);
                    break;
                case 'n':
                    toggleNormalization();
                    break;
            }
        }
        
        // Sleep to prevent CPU hogging
#ifdef _WIN32
        Sleep(10); // 10 milliseconds
#else
        usleep(10000); // 10 milliseconds
#endif
    }
    
    // Restore original terminal settings
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
#endif
    
    // Stop audio
    SDL_PauseAudioDevice(audioDevice, 1);
    
    printf("Playback ended.\n");
}

// Cross-platform cleanup function
void cleanup() {
    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();
    
    // Cleanup OPL
    OPL_Shutdown();
    
    if (midiFile) {
        fclose(midiFile);
        midiFile = NULL;
    }

#ifndef _WIN32
    // Restore terminal settings on Unix-like systems
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
#endif
}
