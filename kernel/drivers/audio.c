/* ============================================================
   audio.c - AegisOS Ses Sürücüsü
   Katmanlar:
     1. PC Speaker (8253 PIT CH2) — Her sistemde çalışır
     2. AC97 temel destek (PCI üzerinden)
   ============================================================ */
#include "../include/audio.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include <stdint.h>

static AudioDriverType audio_type = AUDIO_NONE;

/* ============================================================
   PC Speaker (PIT Kanal 2)
   ============================================================ */
#define PIT_BASE_FREQ  1193182UL
#define PIT_CH2_DATA   0x42
#define PIT_MODE_PORT  0x43
#define SPEAKER_PORT   0x61

static inline uint8_t spk_inb(uint16_t p){uint8_t r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}
static inline void spk_outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

void speaker_beep_start(uint32_t freq_hz) {
    if (freq_hz == 0) { speaker_stop(); return; }
    uint32_t divisor = (uint32_t)(PIT_BASE_FREQ / freq_hz);

    /* PIT Kanal 2: kare dalga modu (mod 3) */
    spk_outb(PIT_MODE_PORT, 0xB6);
    spk_outb(PIT_CH2_DATA, (uint8_t)(divisor & 0xFF));
    spk_outb(PIT_CH2_DATA, (uint8_t)(divisor >> 8));

    /* Hoparlörü etkinleştir (bit 0 ve 1) */
    uint8_t tmp = spk_inb(SPEAKER_PORT);
    spk_outb(SPEAKER_PORT, tmp | 0x03);
}

void speaker_stop(void) {
    uint8_t tmp = spk_inb(SPEAKER_PORT);
    spk_outb(SPEAKER_PORT, tmp & ~0x03);
}

void speaker_beep(uint32_t freq_hz, uint32_t duration_ms) {
    speaker_beep_start(freq_hz);
    pit_sleep_ms(duration_ms);
    speaker_stop();
}

/* ============================================================
   Ton/Nota çalma
   ============================================================ */
/* Müzik nota frekansları (Hz) */
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

void audio_play_note(uint32_t freq_hz, uint32_t ms) {
    speaker_beep(freq_hz, ms);
    pit_sleep_ms(20); /* Notalar arası sessizlik */
}

void audio_play_startup_sound(void) {
    /* Yükselme melodisi */
    audio_play_note(NOTE_C4, 80);
    audio_play_note(NOTE_E4, 80);
    audio_play_note(NOTE_G4, 80);
    audio_play_note(NOTE_C5, 150);
}

void audio_play_error_sound(void) {
    speaker_beep(200, 150);
    pit_sleep_ms(50);
    speaker_beep(200, 150);
}

void audio_play_click(void) {
    speaker_beep(800, 10);
}

/* ============================================================
   Başlatma
   ============================================================ */
void audio_init(void) {
    /* PC Speaker her sistemde mevcuttur */
    audio_type = AUDIO_PC_SPEAKER;
    vga_puts("  [SES] PC Speaker surucusu baslatildi\n", 0x0A);
}

AudioDriverType audio_get_type(void) { return audio_type; }

const char* audio_get_name(void) {
    switch (audio_type) {
        case AUDIO_PC_SPEAKER: return "PC Speaker (PIT CH2)";
        case AUDIO_AC97:       return "Intel AC97";
        case AUDIO_HDA:        return "Intel HDA";
        default:               return "Ses surucusu yok";
    }
}
