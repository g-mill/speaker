/*
* Simple test to see if speaker can play tones
*/
#include "i2s.h"
#include "rpi.h"
#include "gpio.h"
#include "bit-support.h"

#define BCLK_PIN  10 // Assign your BCLK pin number
#define LRCLK_PIN 11 // Assign your LRCLK (WS) pin number
#define DIN_PIN   13 // Assign your DIN pin number
#define BIT_DEPTH 16

static int lrclk_state = 0;


void bitbang_write(uint32_t mono_channel_data){

    const int bits_per_frame = 32; // For simplicity, assuming 32 bits for the channel in the frame
    int i;
    int bit_state;

    // Half period of BCLK in microseconds (approximation)
    int bclk_half_period_us = 1;

    lrclk_state = !lrclk_state;
    gpio_write(LRCLK_PIN, lrclk_state);

    // Mono audio: Only using one channel (e.g., left channel, LRCLK low)

    for (i = 0; i < bits_per_frame * 5000; i++) {
        // Toggle BCLK low
        gpio_write(BCLK_PIN, 0);
        delay_us(bclk_half_period_us);

        // Set DIN for the current bit of mono_channel_data
        bit_state = (mono_channel_data & (1 << (BIT_DEPTH - 1 - i))) != 0;
        gpio_write(DIN_PIN, bit_state);

        // Toggle BCLK high
        gpio_write(BCLK_PIN, 1);
        delay_us(bclk_half_period_us);
    }
}

void notmain(void) {
    printk("Initializing speaker.\n");
    i2s_speaker_init();

    gpio_set_function(13, GPIO_FUNC_OUTPUT);

    printk("Doing a bitbang write to the speaker.\n");
    bitbang_write(0xFFFF);

    output("success!\n");
}