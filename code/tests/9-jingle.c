/*
* Simple test to see if speaker can play tones
*/
#include "i2s.h"
#include "rpi.h"
#include "gpio.h"
#include "bit-support.h"

// Note frequencies for "Twinkle, Twinkle, Little Star"
int note_frequencies[] = {262, 262, 392, 392, 440, 440, 392};
// Corresponding half-periods in microseconds (pre-calculated: 500000 / frequency)
int half_periods_us[] = {1908, 1908, 1276, 1276, 1136, 1136, 1276}; 
int note_durations_ms[] = {500, 500, 500, 500, 500, 500, 750}; // Duration of each note in milliseconds

void play_note_with_delay(int half_period_us, int duration_ms) {
    int cycles_for_duration = 20;

    for(int i = 0; i < cycles_for_duration; i++) {
        gpio_write(13, 1); // Set GPIO high
        delay_us(half_period_us); // Wait for half-period
        gpio_write(13, 0); // Set GPIO low
        delay_us(half_period_us); // Wait for half-period
    }
}

void bitbang_jingle() {
    int num_notes = sizeof(note_frequencies) / sizeof(note_frequencies[0]);
    for(int i = 0; i < num_notes; i++) {
        play_note_with_delay(half_periods_us[i], note_durations_ms[i]); // Play each note
        delay_us(200000); // Short pause between notes
    }
}

void notmain(void) {
    printk("Initializing speaker.\n");
    i2s_speaker_init();

    gpio_set_function(13, GPIO_FUNC_OUTPUT);

    printk("Beginning jingle...\n");
    bitbang_jingle();

    output("success!\n");
}