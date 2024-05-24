#include "rpi.h"
#include "gpio.h"
#include "wav_file_reader.h"

void notmain() {
    size_t num_samples = 0;
    read_wav_file("./hello.wav", &num_samples);
}