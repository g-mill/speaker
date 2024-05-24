#include "i2s.h"
#include "rpi.h"
#include "gpio.h"
#include "bit-support.h"

volatile i2s_regs_t *i2s_regs = (volatile i2s_regs_t *)I2S_REGS_BASE;
volatile cm_regs_t *cm_regs = (volatile cm_regs_t *)CM_REGS_BASE;

#define addr(x) ((uint32_t)&(x))

void i2s_speaker_init(void) {
    dev_barrier();

    gpio_set_function(I2S_PIN_CLK, GPIO_FUNC_ALT0);  // 18 (BCLCK)
    gpio_set_function(I2S_PIN_FS, GPIO_FUNC_ALT0);  // 19 (LRC)
    gpio_set_function(I2S_PIN_DOUT, GPIO_FUNC_ALT0);  // 21, for speaker
    // leave gain pin and SD pin floating

    dev_barrier();

    // disable I2S clock --> 0x5A000000
    PUT32((unsigned)&(cm_regs->pcm_ctrl), CM_REGS_MSB);
    delay_us(10);

    // configure I2S clock
    uint32_t val = CM_REGS_MSB;
    val = bits_set(val, 0, 3, 0b0001);
    val = bits_set(val, 9, 10, 0b11);
    PUT32((unsigned)&(cm_regs->pcm_ctrl), val);

    // set div
    val = CM_REGS_MSB;
    val |= (CM_DIV_INT << 12 | CM_DIV_FRAC);
    PUT32((unsigned)&(cm_regs->pcm_div), val);

    // turn on I2S clock
    val = GET32((unsigned)&(cm_regs->pcm_ctrl));
    val = bit_set(val, 4);
    val |= CM_REGS_MSB;
    PUT32((unsigned)&(cm_regs->pcm_ctrl), val);

    dev_barrier();

    // disable I2S
    PUT32((unsigned)&(i2s_regs->cs), 0);
    delay_us(100);
    
    // set up mode
    val = 0;
    val = bit_set(val, 20);  // don't enable frame sync
    val = bit_set(val, 22);
    val |= (63 << 10);  // flen
    val |= (32);  // fslen
    PUT32((unsigned)&(i2s_regs->mode), val);

    // for tx need to set tx channel settings
    val = 0;
    val = bit_set(val, 30);
    // val = bit_set(val, 31);  // set extension bit for 32-bit samples
    val = bits_set(val, 16, 19, 8);
    PUT32((unsigned)&(i2s_regs->tx_cfg), val);

    // finalize control reg
    val = 0;
    val = bit_set(val, 0);  // enable i2s
    val = bit_set(val, 24);  // clock sync helper
    val = bit_set(val, 25);  // release from standby
    val = bit_set(val, 3);  // clear tx fifo
    val = bit_set(val, 2);  // enable tx
    PUT32((unsigned)&(i2s_regs->cs), val);

    dev_barrier();
}


void i2s_mic_init(void) {
    dev_barrier();

    gpio_set_function(I2S_PIN_CLK, GPIO_FUNC_ALT0);  // 18 (BCLCK)
    gpio_set_function(I2S_PIN_FS, GPIO_FUNC_ALT0);  // 19 (LRC)
    gpio_set_function(I2S_PIN_DIN, GPIO_FUNC_ALT0);  // 20, for mic

    dev_barrier();

    // configure I2S clock
    uint32_t val = CM_REGS_MSB;
    val = bits_set(val, 0, 3, 0b0001);
    val = bits_set(val, 9, 10, 0b11);
    PUT32((unsigned)&(cm_regs->pcm_ctrl), val);

    // set div
    val = CM_REGS_MSB;
    val |= (CM_DIV_INT << 12 | CM_DIV_FRAC);
    PUT32((unsigned)&(cm_regs->pcm_div), val);

    // turn on I2S clock
    val = GET32((unsigned)&(cm_regs->pcm_ctrl));
    val = bit_set(val, 4);
    val |= CM_REGS_MSB;
    PUT32((unsigned)&(cm_regs->pcm_ctrl), val);

    dev_barrier();

    // set up mode
    val = 0;
    val |= (63 << 10);  // flen
    val |= (32);  // fslen
    PUT32((unsigned)&(i2s_regs->mode), val);

    // for rx need to set rx channel settings
    val = 0;
    val = bit_set(val, 30);
    val = bit_set(val, 31);
    val = bits_set(val, 16, 19, 0);
    PUT32((unsigned)&(i2s_regs->rx_cfg), val);

    // finalize control reg
    val = 0;
    val = bit_set(val, 0);  // enable i2s
    val = bit_set(val, 24);  // clock sync helper
    val = bit_set(val, 25);  // release from standby
    val = bit_set(val, 4);  // clear rx fifo
    val = bit_set(val, 1);  // enable rx
    PUT32((unsigned)&(i2s_regs->cs), val);

    dev_barrier();
}


int32_t i2s_read_sample(void) {
    dev_barrier();
    while(bit_is_off(i2s_regs->cs, 20)){}  // empty fifo, nothing to read

    return(i2s_regs->fifo);
}


void i2s_write_sample(int32_t sample){
    dev_barrier();

    while (!bit_is_on(i2s_regs->cs, 19)) {}  // wait for space in fifo

    i2s_regs->fifo = sample;

    dev_barrier();
}