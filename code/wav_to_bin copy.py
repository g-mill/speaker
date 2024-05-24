from pydub import AudioSegment
import numpy as np
import struct
from scipy.io.wavfile import write as wav_write, read as wav_read

def mp3_to_wav(mp3_filename, wav_filename):
    # Convert MP3 to WAV
    audio = AudioSegment.from_mp3(mp3_filename)
    # audio = audio.set_frame_rate(44100).set_channels(1).set_sample_width(4)  # Ensure 44.1kHz, mono, and signed 32-bit
    audio = audio.set_frame_rate(44100).set_channels(1).set_sample_width(2)  # Ensure 44.1kHz, mono, and signed 16-bit
    audio.export(wav_filename, format="wav")


def dump_wav_samples_to_bin_scipy(wav_filename, binary_filename):
    # Read the WAV file
    sample_rate, samples = wav_read(wav_filename)
    
    # Check if the sample rate is 44.1 kHz
    if sample_rate != 44100:
        raise ValueError("Sample rate must be 44.1 kHz")

    with open(binary_filename, "wb") as bin_file:
        for sample in samples:
            # pack the sample as a 16-bit signed integer ('h') and write it to the file, use 'i' for 32-bit
            bin_file.write(struct.pack('<h', sample))
    
    print("sample rate is:", sample_rate, "number of samples read:", len(samples))


def text_to_wav(text_filename, wav_filename):
    buf = []
    with open(text_filename, 'r') as text_file:
        lines = text_file.readlines()
        print(len(lines))
        for line in lines:
            line = line.strip()
            buf.append(int(line))
    # wav_write(wav_filename, 44100, np.array(buf).astype(np.int32))
    wav_write(wav_filename, 44100, np.array(buf).astype(np.int16))


def main():
    mp3_filename = "samples/angry_birds.mp3"
    wav_filename = "samples/angry_birds.wav"
    binary_filename = "samples/16-bit_bin/birds.bin"

    mp3_to_wav(mp3_filename, wav_filename)
    dump_wav_samples_to_bin_scipy(wav_filename, binary_filename)


if __name__ == '__main__':
    main()