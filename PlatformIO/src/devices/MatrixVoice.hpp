#pragma once
#include <Arduino.h>
#include <device.h>

#include "everloop.h"
#include "everloop_image.h"
#include "microphone_array.h"
#include "microphone_core.h"
#include "voice_memory_map.h"
#include "wishbone_bus.h"
#include <thread>
extern "C" {
  #include "speex_resampler.h"
}

// This is used to be able to change brightness, while keeping the colors appear
// the same Called gamma correction, check this
// https://learn.adafruit.com/led-tricks-gamma-correction/the-issue
const uint8_t PROGMEM gamma8[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
    2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,
    8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,
    14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,
    21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,
    31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,
    43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,
    58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,
    75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,
    96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119,
    120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146,
    148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177,
    180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252,
    255};

const uint16_t defaultSampleRate = 48000;
const uint16_t defaultSampleRateSetting = 163; // 48000 = 163, 44100=177

int err;
SpeexResamplerState *resampler = speex_resampler_init(1, defaultSampleRate, defaultSampleRate, 0, &err);

class MatrixVoice : public Device
{
public:
  MatrixVoice();
  void init();
	void updateColors(int colors);
	void updateBrightness(int brightness);
  void muteOutput(bool mute);
  void setVolume(uint16_t volume);
  void setWriteMode(int sampleRate, int bitDepth, int numChannels); 
	bool readAudio(uint8_t *data, size_t size);
  void writeAudio(uint8_t *data, size_t size, size_t *bytes_written);
  void ampOutput(int output);
	int readSize = 512;
	int writeSize = 1024;
	int width = 2;
	int rate = 16000;

private:
  matrix_hal::WishboneBus wb;
  matrix_hal::Everloop everloop;
	matrix_hal::MicrophoneArray *mics;
  matrix_hal::EverloopImage image1d;
  void playBytes(uint8_t* input, uint32_t length);
	void interleave(const int16_t * in_L, const int16_t * in_R, int16_t * out, const size_t num_samples);
  int sampleRate, bitDepth, numChannels;
	int brightness = 15;
	uint16_t GetFIFOStatus();
	bool FIFOFlush();
};

MatrixVoice::MatrixVoice()
{
};

bool MatrixVoice::FIFOFlush() {
  uint16_t value = 0x0001;
  wb.SpiWrite(matrix_hal::kConfBaseAddress + 12, (const uint8_t *)(&value), sizeof(uint16_t));
  value = 0x0000;
  wb.SpiWrite(matrix_hal::kConfBaseAddress + 12, (const uint8_t *)(&value), sizeof(uint16_t));
  return true;
}

void MatrixVoice::init()
{
	Serial.println("Matrix Voice Initialized");
  wb.Init();
  everloop.Setup(&wb);
	mics = new matrix_hal::MicrophoneArray();
  mics->Setup(&wb);
  mics->SetSamplingRate(rate);
  matrix_hal::MicrophoneCore mic_core(*mics);
  mic_core.Setup(&wb);  

  FIFOFlush();
  uint16_t value = defaultSampleRateSetting;
  wb.SpiWrite(matrix_hal::kConfBaseAddress + 9, (const uint8_t *)(&value), sizeof(uint16_t));
};

void MatrixVoice::updateBrightness(int brightness) {
	// all values below 10 is read as 0 in gamma8, we map 0 to 10
	MatrixVoice::brightness = brightness * 90 / 100 + 10;
}

void MatrixVoice::updateColors(int colors) {
	int r = 0;
	int g = 0;
	int b = 0;
	int w = 0;	
  switch (colors) {
    case COLORS_HOTWORD:
			r = hotword_colors[0];
			g = hotword_colors[1];
			b = hotword_colors[2];
			w = hotword_colors[3];		
    break;
    case COLORS_WIFI_CONNECTED:
			r = wifi_conn_colors[0];
			g = wifi_conn_colors[1];
			b = wifi_conn_colors[2];
			w = wifi_conn_colors[3];		
    break;
    case COLORS_IDLE:
			r = idle_colors[0];
			g = idle_colors[1];
			b = idle_colors[2];
			w = idle_colors[3];		
    break;
    case COLORS_WIFI_DISCONNECTED:
			r = wifi_disc_colors[0];
			g = wifi_disc_colors[1];
			b = wifi_disc_colors[2];
			w = wifi_disc_colors[3];		
    break;
    case COLORS_OTA:
			r = ota_colors[0];
			g = ota_colors[1];
			b = ota_colors[2];
			w = ota_colors[3];		
    break;
  }
	r = floor(MatrixVoice::brightness * r / 100);
	r = pgm_read_byte(&gamma8[r]);
	g = floor(MatrixVoice::brightness * g / 100);
	g = pgm_read_byte(&gamma8[g]);
	b = floor(MatrixVoice::brightness * b / 100);
	b = pgm_read_byte(&gamma8[b]);
	w = floor(MatrixVoice::brightness * w / 100);
	w = pgm_read_byte(&gamma8[w]);
	for (matrix_hal::LedValue &led : image1d.leds) {
		led.red = r;
		led.green = g;
		led.blue = b;
		led.white = w;
	}
	everloop.Write(&image1d);
}

void MatrixVoice::muteOutput(bool mute) {
  int16_t muteValue = mute ? 1 : 0;
  wb.SpiWrite(matrix_hal::kConfBaseAddress+10,(const uint8_t *)(&muteValue), sizeof(uint16_t));
}

void MatrixVoice::setVolume(uint16_t volume) {
	uint16_t outputVolume = (100 - volume) * 25 / 100; //25 is minimum volume
	wb.SpiWrite(matrix_hal::kConfBaseAddress+8,(const uint8_t *)(&outputVolume), sizeof(uint16_t));
};

void MatrixVoice::ampOutput(int output) {
  wb.SpiWrite(matrix_hal::kConfBaseAddress+11,(const uint8_t *)(&output), sizeof(uint16_t));
};

void MatrixVoice::setWriteMode(int sampleRate, int bitDepth, int numChannels) {
	MatrixVoice::sampleRate = sampleRate;
	MatrixVoice::bitDepth = bitDepth;
	MatrixVoice::numChannels = numChannels;
	speex_resampler_set_rate(resampler,sampleRate,defaultSampleRate);
	speex_resampler_skip_zeros(resampler);	
}; 

bool MatrixVoice::readAudio(uint8_t *data, size_t size) {
	mics->Read();
	uint16_t voicebuffer[readSize];
	for (uint32_t s = 0; s < readSize; s++) {
	 	voicebuffer[s] = mics->Beam(s);
	}
  memcpy(data, voicebuffer, readSize * width);
	return true;
}

void MatrixVoice::writeAudio(uint8_t *data, size_t size, size_t *bytes_written) {
	*bytes_written = size;
	if (MatrixVoice::sampleRate == defaultSampleRate) {
		if (MatrixVoice::numChannels == 2) {
			uint32_t output_len = size / sizeof(int16_t);
			int16_t output[output_len];
			//Convert 8 bit to 16 bit
			for (int i = 0; i < size; i += 2) {
				output[i/2] = ((data[i] & 0xff) | (data[i + 1] << 8)); 
			}

			playBytes((uint8_t *)output, output_len * sizeof(int16_t));
		} else {
			uint32_t mono_len = size / sizeof(int16_t);
			int16_t mono[mono_len];
			//Convert 8 bit to 16 bit
			for (int i = 0; i < size; i += 2) {
				mono[i/2] = ((data[i] & 0xff) | (data[i + 1] << 8)); 
			}

			uint32_t stereo_len = mono_len * 2;
			int16_t stereo[stereo_len];
			MatrixVoice::interleave(mono, mono, stereo, mono_len);
			playBytes((uint8_t *)stereo, stereo_len * sizeof(int16_t));
		}
	} else {	
		uint32_t in_len = size / sizeof(int16_t);
		int16_t input[in_len];
		//Convert 8 bit to 16 bit
		for (int i = 0; i < size; i += 2) {
				input[i/2] = ((data[i] & 0xff) | (data[i + 1] << 8));
		}

		if (MatrixVoice::numChannels == 2) {
			uint32_t out_len = in_len * (float)(defaultSampleRate) / (float)(MatrixVoice::sampleRate) + 1;
			int16_t output[out_len];
			speex_resampler_process_interleaved_int(resampler, input, &in_len, output, &out_len); 
				
			//play it!
			playBytes((uint8_t *)output, out_len * sizeof(int16_t));
		} else {
				uint32_t mono_len = in_len * (float)(defaultSampleRate) / (float)(MatrixVoice::sampleRate) + 1;
				int16_t mono[mono_len];
				speex_resampler_process_int(resampler, 0, input, &in_len, mono, &mono_len);

				uint32_t stereo_len = mono_len * 2;
				int16_t stereo[stereo_len];
				MatrixVoice::interleave(mono, mono, stereo, mono_len);

				//play it!                         
				MatrixVoice::playBytes((uint8_t *)stereo, stereo_len * sizeof(int16_t));
		}
	}
};

void MatrixVoice::interleave(const int16_t * in_L, const int16_t * in_R, int16_t * out, const size_t num_samples)
{
    for (size_t i = 0; i < num_samples; ++i)
    {
        out[i * 2] = in_L[i];
        out[i * 2 + 1] = in_R[i];
    }
}

const uint32_t kFIFOSize = 4096;

uint16_t MatrixVoice::GetFIFOStatus() {
  uint16_t write_pointer;
  uint16_t read_pointer;
  wb.SpiRead(matrix_hal::kDACBaseAddress + 0x802, (uint8_t *)&read_pointer, sizeof(uint16_t));
  wb.SpiRead(matrix_hal::kDACBaseAddress + 0x803, (uint8_t *)&write_pointer, sizeof(uint16_t));

  if (write_pointer >= read_pointer)
    return write_pointer - read_pointer;
  else
    return kFIFOSize - read_pointer + write_pointer;
}

void MatrixVoice::playBytes(uint8_t* input, uint32_t length) {
	while ( length > 0) {
		uint32_t size = MatrixVoice::writeSize;
		if ( length < MatrixVoice::writeSize)
			size = length;

		uint16_t fifo_status = GetFIFOStatus();
		while( (kFIFOSize - fifo_status) < size) {
			int sleep = int(size * 1.0 / (float)(defaultSampleRate) * 1000/(2*sizeof(int16_t)))/4;
      		std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
			fifo_status = GetFIFOStatus();
    	}

		wb.SpiWrite(matrix_hal::kDACBaseAddress, (const uint8_t *)input, size);
		input += size;
		length -= size;
	}
}