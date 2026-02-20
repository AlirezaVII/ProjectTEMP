#ifndef AUDIO_H
#define AUDIO_H

bool audio_init();
void audio_quit();
void audio_play_meow();
void audio_stop_all();
void audio_set_volume(int percent); // 0 to 100
bool audio_is_playing();

#endif