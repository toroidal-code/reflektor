
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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#define _USE_MATH_DEFINES
#define M_PI 3.14159265358979323846
#include <cmath>
#include <complex.h>
#include <portaudio.h>
#include <libwebsockets.h>
#include <fftw3.h>
#include <string>
#include <iostream>
#include <sys/time.h>


typedef enum libwebsocket_callback_reasons callback_reasons;
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
// static float in[FRAMES_PER_BUFFER];
static fftwf_complex left_out[FRAMES_PER_BUFFER], right_out[FRAMES_PER_BUFFER];
static fftwf_plan lp, rp;

const static int PROTOCOL_HTTP = 0;

static int callback_http(struct libwebsocket_context *context,
                         struct libwebsocket *wsi,
                         callback_reasons reason, void *user,
                         void *in, size_t len) {
  switch (reason) {
    case LWS_CALLBACK_SERVER_WRITEABLE: {
      std::stringstream universal_response;
      universal_response << "[";
      std::string comma = "";
      for (int i = 0;
           i < ((FRAMES_PER_BUFFER / 2)  - 1) / 4;
           i++) {
        universal_response << comma << "{ \"x\": " << i * SAMPLE_RATE / FRAMES_PER_BUFFER << ", \"y\": " << left_out[i][0] << "}";
      comma = ",";
      }
      universal_response << "]";
      std::string universal_response_string = universal_response.str();
      //std::cout << "sending:" << universal_response_string << std::endl;
      libwebsocket_write(wsi, (unsigned char *)universal_response_string.c_str(), universal_response_string.length(), LWS_WRITE_TEXT);
      break;
    }
  }

    return 0;
}

struct per_session_data__http { int fd; };

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
  } 

  /* Hanning window function */
  for (i = 0; i < framesPerBuffer; i++) {
    double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (framesPerBuffer - 1)));
    left_in[i] = multiplier * left_in[i];
    right_in[i] = multiplier * right_in[i];
  }

  fftwf_execute(lp);
  fftwf_execute(rp);

  // printf("[");
  // for (i = 0; 
  //      i < (framesPerBuffer / 2) - 1;
  //      i++) { // second half of bins are useless
  //   // printf("%-5u Hz: L: %f + i%f, ",
  //   //        i * SAMPLE_RATE / FRAMES_PER_BUFFER, creal(left_out[i]),
  //   //        cimag(left_out[i]));
  //   // printf("R: %f + i%f\n", creal(right_out[i]), cimag(right_out[i]));
  //   printf("(%u, %f),", i * SAMPLE_RATE / FRAMES_PER_BUFFER, right_out[i][0]);
  // }
  // printf("]\n");

  return paContinue;
}

void setupAudio(PaStream *stream){
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

// list of supported protocols and callbacks
static struct libwebsocket_protocols protocols[] = {
    // first protocol must always be HTTP handler
    {
        "http-only",        // name
        callback_http,      // callback
        0,                   // per_session_data_size
    },
    {
        NULL, NULL, 0, 0,       // end of list
    }
};

/*******************************************************************/
int main(void) {
  // server url will be http://localhost:7681
  int port = 8080;
  const char *interface = NULL;
  struct libwebsocket_context *context;
  uint oldus = 0;


  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.port = 7681;
  info.iface = NULL;
  info.protocols = protocols;
  #ifndef LWS_NO_EXTENSIONS
  info.extensions = libwebsocket_get_internal_extensions();
  #endif
  info.ssl_cert_filepath = NULL;
  info.ssl_private_key_filepath = NULL;
  info.gid = -1;
  info.uid = -1;
  info.options = 0;     // no special options

  // create libwebsocket context representing this server
  context = libwebsocket_create_context(&info);


    if (context == NULL) {
        fprintf(stderr, "libwebsocket init failed\n");
        return -1;
    }
    
    printf("starting server...\n"); 

  PaStream *stream = nullptr;
  setupAudio(stream);

  int n = 0;
  while (n >= 0 ) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    /*
     * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
     * live websocket connection using the DUMB_INCREMENT protocol,
     * as soon as it can take more packets (usually immediately)
     */

    if (((unsigned int)tv.tv_usec - oldus) > 50000) {
      libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_HTTP]);
      oldus = tv.tv_usec;
    }

    /*
     * If libwebsockets sockets are all we care about,
    * you can use this api which takes care of the poll()
     * and looping through finding who needed service.
     *
     * If no socket needs service, it'll return anyway after
     * the number of ms in the second argument.
     */

    n = libwebsocket_service(context, 50);

  }

  Pa_CloseStream(stream);
  Pa_Terminate();
  libwebsocket_context_destroy(context);
  std::cout << "Quiting!" << std::endl;
  return 0;
}
