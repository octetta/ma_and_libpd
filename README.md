# ma_and_libpd
Show usage of miniaudio in the audio callback for libpd.

Expanded on Dan's example that used portaudio.

libpd info at https://github.com/libpd/libpd

Miniaudio info at https://miniaud.io/

You'll have to build libpd with static libraries and
adjust the Makefile to point to the headers/libs.

This code runs under Linux too, just remove the framework
entries from the compile line.

usage:

```
./st0 test.pd . # runs with default playback device

./st0 # shows playback / capture devices and exits

./st0 test.pd . 0 # selects playback device 0

./st0 test.pd . 0 1 # select playback device 0 and enables capture device 1
# note the test.pd file doesn't have a [adc~] object, so this doesn't really
# do anything obvious.
```

Here's a screen shot of the patch being used in this sample.

![test.pd](https://github.com/octetta/ma_and_libpd/blob/main/test_pd.png)

----

I've crammed a bunch of PD files from obiwannabe.co.uk here to test with.

I can't find a live version of this site, so it's scraped from

https://web.archive.org/web/20070513224738/http://www.obiwannabe.co.uk/html/music/6SS/six-simple-synthesisers.html
