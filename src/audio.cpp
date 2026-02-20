#include "audio.h"
#include <SDL_mixer.h>
#include <iostream>

static Mix_Chunk* meow_sound = nullptr;
static int meow_channel = -1;

bool audio_init() {
    // Open the audio device at standard CD quality
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
        return false;
    }
    
    // Try to load the M4A file! (If it fails, game continues silently)
    meow_sound = Mix_LoadWAV("assets/sounds/meow.m4a");
    if (!meow_sound) {
        // Fallback just in case you ever convert it to WAV
        meow_sound = Mix_LoadWAV("assets/sounds/meow.wav"); 
    }
    return true;
}

void audio_quit() {
    if (meow_sound) Mix_FreeChunk(meow_sound);
    Mix_CloseAudio();
}

void audio_play_meow() {
    if (meow_sound) {
        // Play on the first available channel
        meow_channel = Mix_PlayChannel(-1, meow_sound, 0);
    }
}

void audio_stop_all() {
    Mix_HaltChannel(-1);
}

void audio_set_volume(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    int mix_vol = (percent * MIX_MAX_VOLUME) / 100;
    Mix_Volume(-1, mix_vol);
}

bool audio_is_playing() {
    if (meow_channel == -1) return false;
    return Mix_Playing(meow_channel) > 0;
}