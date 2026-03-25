#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* Ses sürücüsü türü */
typedef enum {
    AUDIO_NONE  = 0,
    AUDIO_PC_SPEAKER = 1,
    AUDIO_AC97  = 2,
    AUDIO_HDA   = 3
} AudioDriverType;

/* Başlatma ve durum */
void audio_init(void);
AudioDriverType audio_get_type(void);
const char* audio_get_name(void);

/* PC Hoparlörü (her sistemde çalışır) */
void speaker_beep(uint32_t freq_hz, uint32_t duration_ms);
void speaker_beep_start(uint32_t freq_hz);
void speaker_stop(void);

/* Basit ton üretimi */
void audio_play_note(uint32_t freq_hz, uint32_t ms);
void audio_play_startup_sound(void);
void audio_play_error_sound(void);
void audio_play_click(void);

#endif /* AUDIO_H */
