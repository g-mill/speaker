#if 1
//#include <SPIFFS.h>
//#include <FS.h>
#include "wav_file_reader.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "demand.h"
#include "rpi.h"
#include <string.h>
#define TRUE 1 
#define FALSE 0

char* seconds_to_time(float seconds);

FILE *file;
HEADER header;

int16_t* read_wav_file(char *filename, size_t* num_samples_out) {
    #if 1
    // Open the WAV file for reading
    printk("opening  file\n");
    file = fopen(filename, "rb");
    if (!file) {
        panic("error opening input file\n");
    }

    // getting riff string, bytes 1-4
    int read = fread(header.riff, sizeof(header.riff), 1, file);
    assert(read);
    printk("riff: %s\n", header.riff); 

    // getting size of file in bytes, bytes 5-8
    uint8_t buf[4];
    read = fread(buf, sizeof(buf), 1, file);
    printk("little endian file bytes: %u %u %u %u\n", buf[0], buf[1], buf[2], buf[3]);
    header.overall_size  = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);  // convert little endian to big endian 4 byte int
    printk("overall size: bytes:%u, Kb:%u\n", header.overall_size, header.overall_size/1024);

    // getting "WAVE" file marker, bytes 9-12
    read = fread(header.wave, sizeof(header.wave), 1, file);
    printk("wave marker: %s\n", header.wave);

    // getting format chunk marker, bytes 13-16
    read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, file);
    printk("format marker: %s\n", header.fmt_chunk_marker);

    // getting length of format data, bytes 17-20
    read = fread(buf, sizeof(buf), 1, file);
    printk("little endian length of format data: %u %u %u %u\n", buf[0], buf[1], buf[2], buf[3]);
    header.length_of_fmt = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);  // convert little endian to big endian 4 byte integer
    printk("length of format header: %u\n", header.length_of_fmt);

    // getting type of format, bytes 21-22
    read = fread(buf, sizeof(buf)/2, 1, file); 
    printk("little endian type of format: %u %u n", buf[0], buf[1]);
    header.format_type = buf[0] | (buf[1] << 8);
    char format_name[10] = "";
    if (header.format_type == 1) {
        strcpy(format_name,"PCM"); 
    } else if (header.format_type == 6) {
        strcpy(format_name, "A-law");
        panic("not PCM type\n");
    } else if (header.format_type == 7) {
        strcpy(format_name, "Mu-law");
        panic("not PCM type\n");
    }
    printk("format type: %u %s\n", header.format_type, format_name);

    // getting number of channels, bytes 23-24
    read = fread(buf, sizeof(buf)/2, 1, file);
    printk("litte endian number of channels: %u %u\n", buf[0], buf[1]);
    header.channels = buf[0] | (buf[1] << 8);
    printk("number of channels: %u\n", header.channels);

    // getting sample rate, bytes 25-28
    read = fread(buf, sizeof(buf), 1, file);
    printk("litte endian sample rate: %u %u %u %u\n", buf[0], buf[1], buf[2], buf[3]);
    header.sample_rate = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    printk("sample rate: %u\n", header.sample_rate);

    // getting byte rate, bytes 29-32
    read = fread(buf, sizeof(buf), 1, file);
    printk("litte endian byte rate: %u %u %u %u\n", buf[0], buf[1], buf[2], buf[3]);
    header.byterate  = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    printk("byte rate: %u , bit rate:%u\n", header.byterate, header.byterate*8);

    // getting header block alignment, bytes 33-34
    read = fread(buf, sizeof(buf)/2, 1, file);
    printk("little endian block alignment: %u %u\n", buf[0], buf[1]);
    header.block_align = buf[0] | (buf[1] << 8);
    printk("block alignment: %u\n", header.block_align);

    // getting bits per sample, bytes 35-36
    read = fread(buf, sizeof(buf)/2, 1, file);
    printk("little endian bits per sample: %u %u\n", buf[0], buf[1]);
    header.bits_per_sample = buf[0] | (buf[1] << 8);
    printk("bits per sample: %u\n", header.bits_per_sample);

    // getting data marker, bytes 37-40
    read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, file);
    printk("data marker: %s\n", header.data_chunk_header);

    // getting data file size, bytes 41-44
    read = fread(buf, sizeof(buf), 1, file);
    printk("little endian file size: %u %u %u %u\n", buf[0], buf[1], buf[2], buf[3]);
    header.data_size = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    printk("size of data chunk: %u\n", header.data_size);

    // calculate no.of samples
    uint32_t num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
    *num_samples_out = num_samples;
    printk("number of samples:%lu\n", num_samples);

    uint32_t size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
    printk("size of each sample:%ld bytes\n", size_of_each_sample);

    // calculate duration of file
    float duration_in_seconds = (float)header.overall_size / header.byterate;
    printk("approx. duration in seconds=%f\n", duration_in_seconds);
    printk("approx. duration in h:m:s=%s\n", seconds_to_time(duration_in_seconds));

    long low_limit = 0l;
    long high_limit = 0l;
    switch (header.bits_per_sample) {
        case 8:
            low_limit = -128;
            high_limit = 127;
            break;
        case 16:
            low_limit = -32768;
            high_limit = 32767;
            break;
        case 32:
            low_limit = -2147483648;
            high_limit = 2147483647;
            break;
    }                  
    printf("Valid range for data values : %ld to %ld\n", low_limit, high_limit);

    long bytes_in_each_channel = (size_of_each_sample / header.channels);

    int16_t* samples = kmalloc(num_samples * sizeof(int16_t));
    char data_buffer[size_of_each_sample];
    for (int i = 1; i <= num_samples; i++) {
        printk("==========Sample %ld / %ld=============n", i, num_samples);
        read = fread(data_buffer, sizeof(data_buffer), 1, file);
        
        uint32_t xchannels = 0;
        int16_t data_in_channel = 0;
        int offset = 0;
        for (xchannels = 0; xchannels < header.channels; xchannels++) {
            if (bytes_in_each_channel == 4) {
                data_in_channel = (data_buffer[offset] & 0x00ff) | ((data_buffer[offset + 1] & 0x00ff) << 8) | ((data_buffer[offset + 2] & 0x00ff) << 16) | (data_buffer[offset + 3] << 24);
            } else if (bytes_in_each_channel == 2) {
                data_in_channel = (data_buffer[offset] & 0x00ff) | (data_buffer[offset + 1] << 8);
            } else if (bytes_in_each_channel == 1) {
                data_in_channel = data_buffer[offset] & 0x00ff;
                data_in_channel -= 128;
            }
            offset += bytes_in_each_channel;
            printk("%d ", data_in_channel);
            samples[i - 1] = data_in_channel;
        }
    }

    return samples;
/*
    // Read audio samples from the WAV file into the buffer
    while ((num_samples_read = fread(buffer, sizeof(int16_t), BUFFER_SIZE, file)) > 0) {
        // Process the audio samples in the buffer
        // Here you can perform further processing or analysis
        total_samples_read += num_samples_read;
    }

    if (feof(file)) {
        printf("Total samples read: %zu\n", total_samples_read);
    } else {
        fprintf(stderr, "Error reading from file\n");
    }

    // Close the file
    fclose(file);

    return 0;
*/

    #else
    //     filename = (char*) kmalloc(sizeof(char) * 1024);
    //     if (filename == NULL) {
    //         printk("Error in mallocn");
    //         exit(1);
    //     }

    // // get file path
    // char cwd[1024];
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {

    //     strcpy(filename, cwd);

        
    //     strcat(filename, "/");
    //     strcat(filename, name);
    //     printk("%sn", filename);
    // }

    // open file
    printk("Opening  file\n");
    ptr = fopen(filename, "rb");
    if (ptr == NULL) {
        PANIC("Error opening file\n");
    }

    int read = 0;

    // read header parts
    read = fread(header.riff, sizeof(header.riff), 1, ptr);
    printk("(1-4): %s n", header.riff); 

    read = fread(buffer4, sizeof(buffer4), 1, ptr);
    printk("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

    // convert little endian to big endian 4 byte int
    header.overall_size  = buffer4[0] | 
                            (buffer4[1]<<8) | 
                            (buffer4[2]<<16) | 
                            (buffer4[3]<<24);

    printk("(5-8) Overall size: bytes:%u, Kb:%u n", header.overall_size, header.overall_size/1024);

    read = fread(header.wave, sizeof(header.wave), 1, ptr);
    printk("(9-12) Wave marker: %sn", header.wave);

    read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, ptr);
    printk("(13-16) Fmt marker: %sn", header.fmt_chunk_marker);

    read = fread(buffer4, sizeof(buffer4), 1, ptr);
    printk("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

    // convert little endian to big endian 4 byte integer
    header.length_of_fmt = buffer4[0] |
                                (buffer4[1] << 8) |
                                (buffer4[2] << 16) |
                                (buffer4[3] << 24);
    printk("(17-20) Length of Fmt header: %u n", header.length_of_fmt);

    read = fread(buffer2, sizeof(buffer2), 1, ptr); 
    printk("%u %u n", buffer2[0], buffer2[1]);

    header.format_type = buffer2[0] | (buffer2[1] << 8);
    char format_name[10] = "";
    if (header.format_type == 1)
    strcpy(format_name,"PCM"); 
    else if (header.format_type == 6)
    strcpy(format_name, "A-law");
    else if (header.format_type == 7)
    strcpy(format_name, "Mu-law");

    printk("(21-22) Format type: %u %s n", header.format_type, format_name);

    read = fread(buffer2, sizeof(buffer2), 1, ptr);
    printk("%u %u n", buffer2[0], buffer2[1]);

    header.channels = buffer2[0] | (buffer2[1] << 8);
    printk("(23-24) Channels: %u n", header.channels);

    read = fread(buffer4, sizeof(buffer4), 1, ptr);
    printk("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

    header.sample_rate = buffer4[0] |
                            (buffer4[1] << 8) |
                            (buffer4[2] << 16) |
                            (buffer4[3] << 24);

    printk("(25-28) Sample rate: %un", header.sample_rate);

    read = fread(buffer4, sizeof(buffer4), 1, ptr);
    printk("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

    header.byterate  = buffer4[0] |
                            (buffer4[1] << 8) |
                            (buffer4[2] << 16) |
                            (buffer4[3] << 24);
    printk("(29-32) Byte Rate: %u , Bit Rate:%un", header.byterate, header.byterate*8);

    read = fread(buffer2, sizeof(buffer2), 1, ptr);
    printk("%u %u n", buffer2[0], buffer2[1]);

    header.block_align = buffer2[0] |
                        (buffer2[1] << 8);
    printk("(33-34) Block Alignment: %u n", header.block_align);

    read = fread(buffer2, sizeof(buffer2), 1, ptr);
    printk("%u %u n", buffer2[0], buffer2[1]);

    header.bits_per_sample = buffer2[0] |
                        (buffer2[1] << 8);
    printk("(35-36) Bits per sample: %u n", header.bits_per_sample);

    read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, ptr);
    printk("(37-40) Data Marker: %s n", header.data_chunk_header);

    read = fread(buffer4, sizeof(buffer4), 1, ptr);
    printk("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

    header.data_size = buffer4[0] |
                    (buffer4[1] << 8) |
                    (buffer4[2] << 16) | 
                    (buffer4[3] << 24 );
    printk("(41-44) Size of data chunk: %u n", header.data_size);


    // calculate no.of samples
    long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
    printk("Number of samples:%lu n", num_samples);

    long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
    printk("Size of each sample:%ld bytesn", size_of_each_sample);

    // calculate duration of file
    float duration_in_seconds = (float) header.overall_size / header.byterate;
    printk("Approx.Duration in seconds=%fn", duration_in_seconds);
    printk("Approx.Duration in h:m:s=%sn", seconds_to_time(duration_in_seconds));


    //size_t num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
    int16_t* samples = kmalloc(num_samples * sizeof(int16_t));

    // read each sample from data chunk if PCM
    if (header.format_type == 1) { // PCM
        printk("Dump sample data? Y/N?");
        char c = 'n';
        scanf("%c", &c);
        if (c == 'Y' || c == 'y') { 
            long i =0;
            char data_buffer[size_of_each_sample];
            int  size_is_correct = TRUE;

            // make sure that the bytes-per-sample is completely divisible by num.of channels
            long bytes_in_each_channel = (size_of_each_sample / header.channels);
            if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
                printk("Error: %ld x %ud <> %ldn", bytes_in_each_channel, header.channels, size_of_each_sample);
                size_is_correct = FALSE;
            }

            if (size_is_correct) { 
                        // the valid amplitude range for values based on the bits per sample
                long low_limit = 0l;
                long high_limit = 0l;

                switch (header.bits_per_sample) {
                    case 8:
                        low_limit = -128;
                        high_limit = 127;
                        break;
                    case 16:
                        low_limit = -32768;
                        high_limit = 32767;
                        break;
                    case 32:
                        low_limit = -2147483648;
                        high_limit = 2147483647;
                        break;
                }					

                printk("nn.Valid range for data values : %ld to %ld n", low_limit, high_limit);
                for (i =1; i <= num_samples; i++) {
                    printk("==========Sample %ld / %ld=============n", i, num_samples);
                    read = fread(data_buffer, sizeof(data_buffer), 1, ptr);
                    if (read == 1) {
                    
                        // dump the data read
                        unsigned int  xchannels = 0;
                        int data_in_channel = 0;
                        int offset = 0; // move the offset for every iteration in the loop below
                        for (xchannels = 0; xchannels < header.channels; xchannels ++ ) {
                            printk("Channel#%d : ", (xchannels+1));
                            // convert data from little endian to big endian based on bytes in each channel sample
                            if (bytes_in_each_channel == 4) {
                                data_in_channel = (data_buffer[offset] & 0x00ff) | 
                                                    ((data_buffer[offset + 1] & 0x00ff) <<8) | 
                                                    ((data_buffer[offset + 2] & 0x00ff) <<16) | 
                                                    (data_buffer[offset + 3]<<24);
                            }
                            else if (bytes_in_each_channel == 2) {
                                data_in_channel = (data_buffer[offset] & 0x00ff) |
                                                    (data_buffer[offset + 1] << 8);
                            }
                            else if (bytes_in_each_channel == 1) {
                                data_in_channel = data_buffer[offset] & 0x00ff;
                                data_in_channel -= 128; //in wave, 8-bit are unsigned, so shifting to signed
                            }

                            offset += bytes_in_each_channel;		
                            printk("%d ", data_in_channel);

                            // check if value was in range
                            if (data_in_channel < low_limit || data_in_channel > high_limit)
                                printk("**value out of rangen");

                            printk(" | ");
                        }

                        printk("n");
                    }
                    else {
                        printk("Error reading file. %d bytesn", read);
                        break;
                    }

                } // 	for (i =1; i <= num_samples; i++) {

            } // 	if (size_is_correct) { 

        } // if (c == 'Y' || c == 'y') { 
    } //  if (header.format_type == 1) { 

    *num_samples_out = num_samples;

    printk("Closing file..n");
    fclose(ptr);

    // cleanup before quitting
    free(filename);
    return samples;

    #endif
}



/**
 * Convert seconds into hh:mm:ss format
 * Params:
 *	seconds - seconds value
* Returns: hms - formatted string
**/
char* seconds_to_time(float raw_seconds) {
    char *hms;
    int hours, hours_residue, minutes, seconds, milliseconds;
    hms = (char*) kmalloc(100);

    //   sprintk(hms, "%f", raw_seconds);
    *hms = raw_seconds;

    //   hours = (int) raw_seconds/3600;
    //   hours_residue = (int) raw_seconds % 3600;
    //   minutes = hours_residue/60;
    //   seconds = hours_residue % 60;
    //   milliseconds = 0;

    //   // get the decimal part of raw_seconds to get milliseconds
    //   char *pos;
    //   pos = strchr(hms, '.');
    //   int ipos = (int) (pos - hms);
    //   char decimalpart[15];
    //   memset(decimalpart, ' ', sizeof(decimalpart));
    //   strncpy(decimalpart, &hms[ipos+1], 3);
    //   milliseconds = atoi(decimalpart);	


    //   sprintk(hms, "%d:%d:%d.%d", hours, minutes, seconds, milliseconds);
    return hms;
}

#else
/**
 * Read and parse a wave file
 *
 **/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wave_file_reader.h"
#define TRUE 1 
#define FALSE 0

// WAVE header structure

unsigned char buffer4[4];
unsigned char buffer2[2];

char* seconds_to_time(float seconds);


FILE *ptr;
char *filename;
struct HEADER header;

int16_t* read_file(char *name, size_t* num_samples_out) {

filename = (char*) malloc(sizeof(char) * 1024);
if (filename == NULL) {
printf("Error in mallocn");
exit(1);
}

// get file path
char cwd[1024];
if (getcwd(cwd, sizeof(cwd)) != NULL) {

    strcpy(filename, cwd);

    // get filename from command line
    if (argc < 2) {
    printf("No wave file specifiedn");
    return;
    }
    
    strcat(filename, "/");
    strcat(filename, argv[1]);
    printf("%sn", filename);
}

// open file
printf("Opening  file..n");
ptr = fopen(filename, "rb");
if (ptr == NULL) {
    printf("Error opening filen");
    exit(1);
}

int read = 0;

// read header parts

read = fread(header.riff, sizeof(header.riff), 1, ptr);
printf("(1-4): %s n", header.riff); 

read = fread(buffer4, sizeof(buffer4), 1, ptr);
printf("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

// convert little endian to big endian 4 byte int
header.overall_size  = buffer4[0] | 
                        (buffer4[1]<<8) | 
                        (buffer4[2]<<16) | 
                        (buffer4[3]<<24);

printf("(5-8) Overall size: bytes:%u, Kb:%u n", header.overall_size, header.overall_size/1024);

read = fread(header.wave, sizeof(header.wave), 1, ptr);
printf("(9-12) Wave marker: %sn", header.wave);

read = fread(header.fmt_chunk_marker, sizeof(header.fmt_chunk_marker), 1, ptr);
printf("(13-16) Fmt marker: %sn", header.fmt_chunk_marker);

read = fread(buffer4, sizeof(buffer4), 1, ptr);
printf("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

// convert little endian to big endian 4 byte integer
header.length_of_fmt = buffer4[0] |
                            (buffer4[1] << 8) |
                            (buffer4[2] << 16) |
                            (buffer4[3] << 24);
printf("(17-20) Length of Fmt header: %u n", header.length_of_fmt);

read = fread(buffer2, sizeof(buffer2), 1, ptr); printf("%u %u n", buffer2[0], buffer2[1]);

header.format_type = buffer2[0] | (buffer2[1] << 8);
char format_name[10] = "";
if (header.format_type == 1)
strcpy(format_name,"PCM"); 
else if (header.format_type == 6)
strcpy(format_name, "A-law");
else if (header.format_type == 7)
strcpy(format_name, "Mu-law");

printf("(21-22) Format type: %u %s n", header.format_type, format_name);

read = fread(buffer2, sizeof(buffer2), 1, ptr);
printf("%u %u n", buffer2[0], buffer2[1]);

header.channels = buffer2[0] | (buffer2[1] << 8);
printf("(23-24) Channels: %u n", header.channels);

read = fread(buffer4, sizeof(buffer4), 1, ptr);
printf("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

header.sample_rate = buffer4[0] |
                        (buffer4[1] << 8) |
                        (buffer4[2] << 16) |
                        (buffer4[3] << 24);

printf("(25-28) Sample rate: %un", header.sample_rate);

read = fread(buffer4, sizeof(buffer4), 1, ptr);
printf("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

header.byterate  = buffer4[0] |
                        (buffer4[1] << 8) |
                        (buffer4[2] << 16) |
                        (buffer4[3] << 24);
printf("(29-32) Byte Rate: %u , Bit Rate:%un", header.byterate, header.byterate*8);

read = fread(buffer2, sizeof(buffer2), 1, ptr);
printf("%u %u n", buffer2[0], buffer2[1]);

header.block_align = buffer2[0] |
                    (buffer2[1] << 8);
printf("(33-34) Block Alignment: %u n", header.block_align);

read = fread(buffer2, sizeof(buffer2), 1, ptr);
printf("%u %u n", buffer2[0], buffer2[1]);

header.bits_per_sample = buffer2[0] |
                    (buffer2[1] << 8);
printf("(35-36) Bits per sample: %u n", header.bits_per_sample);

read = fread(header.data_chunk_header, sizeof(header.data_chunk_header), 1, ptr);
printf("(37-40) Data Marker: %s n", header.data_chunk_header);

read = fread(buffer4, sizeof(buffer4), 1, ptr);
printf("%u %u %u %un", buffer4[0], buffer4[1], buffer4[2], buffer4[3]);

header.data_size = buffer4[0] |
                (buffer4[1] << 8) |
                (buffer4[2] << 16) | 
                (buffer4[3] << 24 );
printf("(41-44) Size of data chunk: %u n", header.data_size);


// calculate no.of samples
long num_samples = (8 * header.data_size) / (header.channels * header.bits_per_sample);
printf("Number of samples:%lu n", num_samples);

long size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
printf("Size of each sample:%ld bytesn", size_of_each_sample);

// calculate duration of file
float duration_in_seconds = (float) header.overall_size / header.byterate;
printf("Approx.Duration in seconds=%fn", duration_in_seconds);
printf("Approx.Duration in h:m:s=%sn", seconds_to_time(duration_in_seconds));



// read each sample from data chunk if PCM
if (header.format_type == 1) { // PCM
    printf("Dump sample data? Y/N?");
    char c = 'n';
    scanf("%c", &c);
    if (c == 'Y' || c == 'y') { 
        long i =0;
        char data_buffer[size_of_each_sample];
        int  size_is_correct = TRUE;

        // make sure that the bytes-per-sample is completely divisible by num.of channels
        long bytes_in_each_channel = (size_of_each_sample / header.channels);
        if ((bytes_in_each_channel  * header.channels) != size_of_each_sample) {
            printf("Error: %ld x %ud <> %ldn", bytes_in_each_channel, header.channels, size_of_each_sample);
            size_is_correct = FALSE;
        }

        if (size_is_correct) { 
                    // the valid amplitude range for values based on the bits per sample
            long low_limit = 0l;
            long high_limit = 0l;

            switch (header.bits_per_sample) {
                case 8:
                    low_limit = -128;
                    high_limit = 127;
                    break;
                case 16:
                    low_limit = -32768;
                    high_limit = 32767;
                    break;
                case 32:
                    low_limit = -2147483648;
                    high_limit = 2147483647;
                    break;
            }					

            printf("nn.Valid range for data values : %ld to %ld n", low_limit, high_limit);
            for (i =1; i <= num_samples; i++) {
                printf("==========Sample %ld / %ld=============n", i, num_samples);
                read = fread(data_buffer, sizeof(data_buffer), 1, ptr);
                if (read == 1) {
                
                    // dump the data read
                    unsigned int  xchannels = 0;
                    int data_in_channel = 0;
                    int offset = 0; // move the offset for every iteration in the loop below
                    for (xchannels = 0; xchannels < header.channels; xchannels ++ ) {
                        printf("Channel#%d : ", (xchannels+1));
                        // convert data from little endian to big endian based on bytes in each channel sample
                        if (bytes_in_each_channel == 4) {
                            data_in_channel = (data_buffer[offset] & 0x00ff) | 
                                                ((data_buffer[offset + 1] & 0x00ff) <<8) | 
                                                ((data_buffer[offset + 2] & 0x00ff) <<16) | 
                                                (data_buffer[offset + 3]<<24);
                        }
                        else if (bytes_in_each_channel == 2) {
                            data_in_channel = (data_buffer[offset] & 0x00ff) |
                                                (data_buffer[offset + 1] << 8);
                        }
                        else if (bytes_in_each_channel == 1) {
                            data_in_channel = data_buffer[offset] & 0x00ff;
                            data_in_channel -= 128; //in wave, 8-bit are unsigned, so shifting to signed
                        }

                        offset += bytes_in_each_channel;		
                        printf("%d ", data_in_channel);

                        // check if value was in range
                        if (data_in_channel < low_limit || data_in_channel > high_limit)
                            printf("**value out of rangen");

                        printf(" | ");
                    }

                    printf("n");
                }
                else {
                    printf("Error reading file. %d bytesn", read);
                    break;
                }

            } // 	for (i =1; i <= num_samples; i++) {

        } // 	if (size_is_correct) { 

    } // if (c == 'Y' || c == 'y') { 
} //  if (header.format_type == 1) { 

printf("Closing file..n");
fclose(ptr);

// cleanup before quitting
free(filename);
return 0;

}

/**
 * Convert seconds into hh:mm:ss format
 * Params:
 *	seconds - seconds value
* Returns: hms - formatted string
**/
char* seconds_to_time(float raw_seconds) {
char *hms;
int hours, hours_residue, minutes, seconds, milliseconds;
hms = (char*) malloc(100);

sprintf(hms, "%f", raw_seconds);

hours = (int) raw_seconds/3600;
hours_residue = (int) raw_seconds % 3600;
minutes = hours_residue/60;
seconds = hours_residue % 60;
milliseconds = 0;

// get the decimal part of raw_seconds to get milliseconds
char *pos;
pos = strchr(hms, '.');
int ipos = (int) (pos - hms);
char decimalpart[15];
memset(decimalpart, ' ', sizeof(decimalpart));
strncpy(decimalpart, &hms[ipos+1], 3);
milliseconds = atoi(decimalpart);	


sprintf(hms, "%d:%d:%d.%d", hours, minutes, seconds, milliseconds);
return hms;
}

#endif
