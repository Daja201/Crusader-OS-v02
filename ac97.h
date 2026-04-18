#ifndef AC97_H
#define AC97_H
#include <stdint.h>

int ac97_init(void);
int ac97_play_test_tone(void);
int ac97_play_pcm(void* buffer, uint32_t length_bytes);

#endif
