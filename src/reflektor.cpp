#include <cstdlib>
#include <cstdio>
#include <cstring>
#define _USE_MATH_DEFINES
#define M_PI 3.14159265358979323846
#include <cmath>
#include <complex.h>
#include <portaudio.h>
#include <fftw3.h>
#include <string>
#include <iostream>
#include <sys/time.h>

/*
** Note that many of the older ISA sound cards on PCs do NOT support
** full duplex audio (simultaneous record and playback).
** And some only support full duplex at lower sample rates.
*/
#define SAMPLE_RATE (44100)
#define PA_SAMPLE_TYPE paFloat32 | paNonInterleaved;
#define FRAMES_PER_BUFFER (4096)
#define PORT 7681

static int gNumNoInputs = 0;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static fftwf_complex left_out[FRAMES_PER_BUFFER], right_out[FRAMES_PER_BUFFER];
static fftwf_plan lp, rp;

static int fftwCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags, void *userData) {
  float **input_ptr_ary = (float **)inputBuffer;
  float *left_in = input_ptr_ary[0];
  float *right_in = input_ptr_ary[1];

  if (lp == NULL && rp == NULL) {
    lp = fftwf_plan_dft_r2c_1d(FRAMES_PER_BUFFER, left_in, left_out,
                               FFTW_MEASURE);
    rp = fftwf_plan_dft_r2c_1d(FRAMES_PER_BUFFER, right_in, right_out,
                               FFTW_MEASURE);
  }

  (void)timeInfo; /* Prevent unused variable warnings. */
  (void)statusFlags;
  (void)userData;

  if (inputBuffer == NULL) {
    gNumNoInputs += 1;
  }

  /* Hanning window function */
  for (uint i = 0; i < framesPerBuffer; i++) {
    double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (framesPerBuffer - 1)));
    left_in[i] = multiplier * (left_in[i] + 1.0);
    right_in[i] = multiplier * (right_in[i] + 1.0);
  }

  fftwf_execute(lp);
  fftwf_execute(rp);

  // second half of bins are useless
  for (uint i = 0; i < (framesPerBuffer / 2) - 1; i++) {
    printf("%-5u Hz: \tL: %f + i%f, \tR: %-5f + i%f\n",
           i * SAMPLE_RATE / FRAMES_PER_BUFFER, left_out[i][0], left_out[i][0],
           right_out[i][0], right_out[i][1]);
  }
  return paContinue;
}

void setupAudio(PaStream *stream) {
  PaStreamParameters inputParameters;
  PaError err;

  err = Pa_Initialize();
  if (err != paNoError)
    Pa_Terminate();

  /* default input device */
  inputParameters.device = Pa_GetDefaultInputDevice();

  if (inputParameters.device == paNoDevice) {
    fprintf(stderr, "Error: No default input device.\n");
    Pa_Terminate();
  }

  inputParameters.channelCount = 2; /* stereo input */
  inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  inputParameters.suggestedLatency =
      Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE,
                      FRAMES_PER_BUFFER, 0,
                      /* paClipOff, */ /* we won't output out of range samples
                                          so don't bother clipping them */
                      fftwCallback, NULL);
  if (err != paNoError)
    Pa_Terminate();

  err = Pa_StartStream(stream);
  if (err != paNoError)
    Pa_Terminate();
}

/*******************************************************************/
int main(void) {
  PaStream *stream = nullptr;
  setupAudio(stream);

  while (true) {
  };

  Pa_CloseStream(stream);
  Pa_Terminate();
  std::cout << "Quiting!" << std::endl;
  return 0;
}
