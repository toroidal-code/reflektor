
/** @file pa_fuzz.c
    @ingroup examples_src
    @brief Distort input like a fuzz box.
    @author Phil Burk  http://www.softsynth.com
*/
/*
 * $Id$
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#define M_PI 3.14159265358979323846
#include <math.h>
#include <complex.h>
#include <portaudio.h>
#include <fftw3.h>
/*
** Note that many of the older ISA sound cards on PCs do NOT support
** full duplex audio (simultaneous record and playback).
** And some only support full duplex at lower sample rates.
*/
#define SAMPLE_RATE (44100)
#define PA_SAMPLE_TYPE paFloat32 | paNonInterleaved;
#define FRAMES_PER_BUFFER (4096)

static int gNumNoInputs = 0;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
// static float in[FRAMES_PER_BUFFER];
static fftwf_complex left_out[FRAMES_PER_BUFFER], right_out[FRAMES_PER_BUFFER];
static fftwf_plan lp, rp;

int max_array(fftwf_complex a[], int num_elements) {
  int i, hz;
  float e, max = -32000;
  for (i = 0; i < num_elements; i++) {
    e = creal(a[i]);
    if (e > max) {
      max = e;
      hz = i * SAMPLE_RATE / FRAMES_PER_BUFFER;
    }
  }
  //printf("%d, %f\n", hz, max);
  return (hz);
}

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

  unsigned int i;
  (void)timeInfo; /* Prevent unused variable warnings. */
  (void)statusFlags;
  (void)userData;

  if (inputBuffer == NULL) {
    gNumNoInputs += 1;
  } // else {
    // printf("BEFORE\n");
    // for (i = 0; i < framesPerBuffer; i++) {
    //   printf("%d: %x: %f, ", i, left_in + i, left_in[i]);
    //   printf("%x: %f\n", right_in +i, right_in[i]);
    // }
  // }

  /* Hanning window function */
  for (i = 0; i < framesPerBuffer; i++) {
    double multiplier = 0.5 * (1  - cos(2 * M_PI * i / (framesPerBuffer - 1)));
    left_in[i] = multiplier * left_in[i];
    right_in[i] = multiplier * right_in[i];
  }

  fftwf_execute(lp);
  fftwf_execute(rp);
  
  printf("Max: %dHz\n", max_array(left_out, framesPerBuffer));
  // printf("[");
  // for (i = 0; 
  //      i < (framesPerBuffer / 2) - 1;
  //      i++) { // second half of bins are useless
  //   // printf("%-5u Hz: L: %f + i%f, ",
  //   //        i * SAMPLE_RATE / FRAMES_PER_BUFFER, creal(left_out[i]),
  //   //        cimag(left_out[i]));
  //   // printf("R: %f + i%f\n", creal(right_out[i]), cimag(right_out[i]));
  //   printf("(%u, %f),", i * SAMPLE_RATE / FRAMES_PER_BUFFER, creal(right_out[i]));
  // }
  // printf("]\n");
  return paContinue;
}

/*******************************************************************/
int main(void) {
  PaStreamParameters inputParameters;
  PaStream *stream;
  PaError err;

  err = Pa_Initialize();
  if (err != paNoError)
    goto error;

  /* default input device */
  inputParameters.device = Pa_GetDefaultInputDevice();

  if (inputParameters.device == paNoDevice) {
    fprintf(stderr, "Error: No default input device.\n");
    goto error;
  }

  inputParameters.channelCount = 2; /* stereo input */
  inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  inputParameters.suggestedLatency =
      Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  // lp=fftwf_plan_dft_r2c_1d(FRAMES_PER_BUFFER, in, out, FFTW_ESTIMATE);
  // fftwf_print_plan(p);

  err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE,
                      FRAMES_PER_BUFFER, 0,
                      /* paClipOff, */ /* we won't output out of range samples
                                          so don't bother clipping them */
                      fftwCallback, NULL);
  if (err != paNoError)
    goto error;

  err = Pa_StartStream(stream);
  if (err != paNoError)
    goto error;

  printf("Hit ENTER to stop program.\n");
  getchar();
  err = Pa_CloseStream(stream);
  if (err != paNoError)
    goto error;

  printf("Finished. gNumNoInputs = %d\n", gNumNoInputs);
  Pa_Terminate();
  return 0;

error:
  Pa_Terminate();
  fprintf(stderr, "An error occured while using the portaudio stream\n");
  fprintf(stderr, "Error number: %d\n", err);
  fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
  return -1;
}
