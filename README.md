minimodem - general-purpose software audio FSK modem
Copyright (C) 2011-2020 Kamal Mostafa <kamal@whence.com>

http://www.whence.com/minimodem

Minimodem is a command-line program which decodes (or generates) audio
modem tones at any specified baud rate, using various framing protocols.
It acts a general-purpose software FSK modem, and includes support for
various standard FSK protocols such as Bell103, Bell202, RTTY, TTY/TDD,
NOAA SAME, and Caller-ID.

Minimodem can play and capture audio modem tones in real-time via the
system audio device, or in batched mode via audio files.

Minimodem can be used to transfer data between nearby computers using an
audio cable (or just via sound waves), or between remote computers using
radio, telephone, or another audio communications medium.

## Fork + Additions

Matthew Currie 2026

- Add support for reading samples from stdin; s16le or f32

### Raw stdin

Raw stdin receive is available with:

```
producer | minimodem --rx --stdin-samples s16le --samplerate 48000 1200
producer | minimodem --rx --stdin-samples f32 --samplerate 48000 1200
```

Input must be mono, headerless, little-endian `s16le` or little-endian 32-bit
float samples. Samplerate must match the producer.
