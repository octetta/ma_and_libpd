/*
  run libpd with audio input/output using miniaudio for a while
  mutated from
  Dan Wilcox <danomatika.com> 2023
  sample in the libpd samples/c
*/
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "z_libpd.h"

// UTIL

#include <unistd.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// cross-platform sleep
// from https://gist.github.com/rafaelglikis/ee7275bf80956a5308af5accb4871135
void sleep_ms(int ms) {
  #ifdef _WIN32
    Sleep(ms);
  #elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
  #else
    usleep(ms * 1000);
  #endif
}

pthread_t printtid = 0;

void pdprint(const char *s) {
  if (printtid == 0) printtid = pthread_self();
  
  int n = strlen(s);
  char hex = 0;
  if (s[n-1] == 0xa) {
    n--;
  }
  for (int i=0; i<n; i++) {
    if (iscntrl(s[i])) {
      hex = 1;
      break;
    }
  }
  if (n == 0) {
    printf("<~ <<>>\n");
  } else if (hex) {
    printf("<~ << ");
    for (int i=0; i<n; i++) {
      printf("%02x ", s[i]);
    }
    puts(">>");
  } else {
    printf("<~ <<%.*s>>\n", n, s);
  }
}

pthread_t noteontid = 0;

void pdnoteon(int ch, int pitch, int vel) {
  if (noteontid == 0) noteontid = pthread_self();
  
  printf("<~ noteon %d %d %d\n", ch, pitch, vel);
}

pthread_t floattid = 0;

void pdfloat(const char *s, float x) {
  if (floattid == 0) floattid = pthread_self();

  printf("<~ float %s %f\n", s, x);
}

#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
//#define MA_DEBUG_OUTPUT
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

int samplerate = 44100;
int buffersize = 512;

pthread_t cbtid = 0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frame_count) {
  int ticks = buffersize / libpd_blocksize();
  if (cbtid == 0) cbtid = pthread_self();
  if (frame_count != buffersize) {
    printf("AUGH\n");
    exit(1);
  }
  libpd_process_short(ticks, pInput, pOutput);
  //libpd_process_short(ticks, NULL, pOutput);
}

ma_device_config config;
ma_device device;

unsigned char custom_data[4096];

ma_context context;

ma_device_info* pPlaybackInfos;
ma_uint32 playbackCount;

ma_device_info* pCaptureInfos;
ma_uint32 captureCount;

int playback = -1;
int pbmax = -1;
int capture = -1;
int capmax = -1;

void list_devices(void) {
  int i;

  puts("playback devices");
  for (i=0; i<playbackCount; i++) {
    printf("%d : %s\n", i, pPlaybackInfos[i].name);
    pbmax = i;
  }
  if (pbmax > 0) {
    playback = 0;
  }

  puts("capture devices");
  for (i=0; i<captureCount; i++) {
    printf("%d : %s\n", i, pCaptureInfos[i].name);
    capmax = i;
  }
}

#define STEP_MS (250)

pthread_t maintid = 0;

int main(int argc, char *argv[]) {
  
  printf("miniaudio version %s\n", ma_version_string());
  
  if (maintid == 0) maintid = pthread_self();

  puts("=> ma_context_init");
  sleep_ms(STEP_MS);

  if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
    printf("failed to get MA context\n");
    exit(1);
  }

  puts("=> ma_context_get_devices");
  sleep_ms(STEP_MS);

  if (ma_context_get_devices(&context,
    &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
    printf("failed to get I/O device list\n");
    exit(1);
  }

  list_devices();

  if (argc < 3) {
    printf("usage: %s file folder [playback capture]\n", argv[0]);
    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return 1;
  }

  int n;
  
  if (argc > 3) {
    n = atoi(argv[3]);
    if (n >= 0 && n <= pbmax) playback = n;
  }

  if (argc > 4) {
    n = atoi(argv[4]);
    if (n >= 0 && n <= capmax) capture = n;
  }

  printf("I/O info with playback : %d capture : %d\n", playback, capture);

  puts("=> ma_device_config_init");
  sleep_ms(STEP_MS);

  if (capture < 0) {
    config = ma_device_config_init(ma_device_type_playback);
  } else {
    config = ma_device_config_init(ma_device_type_duplex);
  }

  config.playback.pDeviceID = &pPlaybackInfos[playback].id;
  if (capture >= 0) {
    config.capture.pDeviceID = &pCaptureInfos[capture].id;
  }
  config.playback.format   = ma_format_s16;
  config.playback.channels = 2;
  config.sampleRate        = samplerate;
  config.dataCallback      = data_callback;
  config.pUserData         = custom_data;

  if (capture >= 0) {
    config.capture.format = ma_format_s16;
    config.capture.channels = 2;
  }

  config.periodSizeInFrames = buffersize;

  puts("=> ma_device_init");
  sleep_ms(STEP_MS);

  if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
    printf("failed to open I/O devices\n");
    ma_device_uninit(&device);
    ma_context_uninit(&context);
    exit(1);
  }
 
  puts("=> libpd hooks");
  sleep_ms(STEP_MS);

  libpd_set_printhook(pdprint);
  //libpd_set_banghook(...);
  //char *s
  libpd_set_floathook(pdfloat);
  //char *s float x
  //libpd_set_doublehook(...);
  //char *s double? x
  //libpd_set_symbolhook(...);
  //libpd_set_listhook(...);
  //libpd_set_messagehook(...);

  // do i need these w/o a midi i/o library?
  //libpd_set_noteonhook(pdnoteon);
  //libpd_set_controlchangehook(...);
  //libpd_set_programchangehook(...);
  //libpd_set_pitchbendhook(...);
  //libpd_set_aftertouchhook(...);
  //libpd_set_polyaftertouchhook(...);
  //libpd_set_midibytehook(...);

  // init pd, match portaudio channels and samplerate

  puts("=> libpd_init");
  sleep_ms(STEP_MS);

  libpd_init();

  libpd_bind("toC");
  
  puts("=> libpd_init_audio");
  sleep_ms(STEP_MS);

  if (capture < 0) {
    libpd_init_audio(0, config.playback.channels, samplerate);
  } else {
    libpd_init_audio(config.capture.channels, config.playback.channels, samplerate);
  }

  // compute audio    [; pd dsp 1(
  libpd_start_message(1); // one entry in list
  libpd_add_float(1.0f);
  libpd_finish_message("pd", "dsp");

  // open patch       [; pd open file folder(
  if (!libpd_openfile(argv[1], argv[2]))
    return 1;

  puts("=> ma_device_start");
  sleep_ms(STEP_MS);

  // start audio processing
  if (ma_device_start(&device) != MA_SUCCESS) {
    printf("Failed to start playback device.\n");
    ma_device_uninit(&device);
    exit(1);
  }

  puts("=> bang start");
  sleep_ms(STEP_MS);

  libpd_bang("start");

  // samples are processed by pd in audio thread (data_callback)
  //
  // simple messages
  // libpd_bang("foo");
  // libpd_double("bar", 1.5);
  // libpd_symbol("baz", "boo");
  //
  // complex message
  // libpd_start_message(maxlen);
  // libpd_add_float(1);
  // libpd_finish_message("pd", "dsp");
  //                      recv   msg
  // means
  //  [; pd dsp 1 (

  puts("=> note loop start");
  sleep_ms(STEP_MS);

  for (int i = 0; i < 40; i++) {
    libpd_float("freq", (i+70)/2);
    if (i  == (40/2)) {
      puts("extra bang start");
      libpd_bang("start");
    }
    sleep_ms(250);
    printf("sleep #%d\n", i+1);
  }

  puts("=> several notes to freq/alt");
  sleep_ms(STEP_MS);

  libpd_float("freq", 63);
  libpd_float("alt", 63-12);
  sleep_ms(250);
  libpd_float("freq", 59);
  libpd_float("alt", 59-12);
  sleep_ms(250);
  libpd_float("freq", 69);
  libpd_float("alt", 69-12);
  sleep_ms(250);
  libpd_float("freq", 67);
  libpd_float("alt", 67-12);
  sleep_ms(250);
  libpd_float("freq", 0);
  libpd_float("alt", 0);

  puts("=> bang end");
  sleep_ms(STEP_MS);

  libpd_bang("end");

  // stop audio processing
  puts("=> ma_device_stop");
  sleep_ms(STEP_MS);
  ma_device_stop(&device);

  puts("=> ma_device_uninit");
  sleep_ms(STEP_MS);

  ma_device_uninit(&device);

  puts("=> ma_context_uninit");
  sleep_ms(STEP_MS);

  ma_context_uninit(&context);

  puts("=> main:return");
  sleep_ms(STEP_MS);

  puts("thread info");
  printf("     main = %p\n", maintid);
  printf("    cbtid = %p\n", cbtid);
  printf(" printtid = %p\n", printtid);
  printf(" floattid = %p\n", floattid);
  printf("noteontid = %p\n", noteontid);

  return 0;
}
