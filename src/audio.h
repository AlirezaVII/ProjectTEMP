#ifndef AUDIO_H
#define AUDIO_H

#include <string> // <--- FIXED: Added this missing include!

struct Mix_Chunk;

bool audio_init();
void audio_quit();
void audio_play_meow();
void audio_stop_all();
void audio_set_volume(int percent); // 0 to 100
bool audio_is_playing();

Mix_Chunk* audio_load_sound(const std::string& path);
void audio_play_chunk(Mix_Chunk* chunk, int volume);

#endif