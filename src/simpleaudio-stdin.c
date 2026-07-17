/*
 * simpleaudio-stdin.c
 *
 * Raw stdin backend for simpleaudio.
 *
 * Copyright (C) 2011-2020 Kamal Mostafa <kamal@whence.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "simpleaudio.h"
#include "simpleaudio_internal.h"

typedef enum {
    SA_STDIN_FORMAT_S16LE,
    SA_STDIN_FORMAT_F32LE,
} sa_stdin_format_t;

typedef struct {
    sa_stdin_format_t input_format;
    unsigned int input_samplesize;
    unsigned char pending[4];
    unsigned int pending_len;
} sa_stdin_handle_t;

static int
sa_stdin_is_little_endian( void )
{
    const uint16_t one = 1;
    return *((const uint8_t *)&one) == 1;
}

static void
sa_stdin_convert_s16le_to_float( const unsigned char *in, float *out,
	size_t nframes )
{
    size_t i;
    for ( i=0; i<nframes; i++ ) {
	uint16_t u = (uint16_t)in[i*2] | ((uint16_t)in[i*2+1] << 8);
	int16_t s = (int16_t)u;
	out[i] = (float)s / 32768.0f;
    }
}

static ssize_t
sa_stdin_read_bytes( unsigned char *buf, size_t nbytes )
{
    ssize_t n;

    do {
	n = read(STDIN_FILENO, buf, nbytes);
    } while ( n < 0 && errno == EINTR );

    if ( n < 0 )
	perror("stdin");

    return n;
}

static ssize_t
sa_stdin_read( simpleaudio *sa, void *buf, size_t nframes )
{
    sa_stdin_handle_t *h = (sa_stdin_handle_t *)sa->backend_handle;
    const size_t input_framesize = sa->channels * h->input_samplesize;
    const size_t want_nbytes = nframes * input_framesize;
    size_t have_nbytes = h->pending_len;
    ssize_t nread;

    unsigned char *raw = malloc(want_nbytes + sizeof(h->pending));
    if ( !raw ) {
	perror("malloc");
	return -1;
    }

    if ( h->pending_len )
	memcpy(raw, h->pending, h->pending_len);

    nread = sa_stdin_read_bytes(raw + have_nbytes, want_nbytes - have_nbytes);
    if ( nread < 0 ) {
	free(raw);
	return -1;
    }

    have_nbytes += nread;
    size_t out_nframes = have_nbytes / input_framesize;
    h->pending_len = have_nbytes % input_framesize;
    if ( h->pending_len )
	memcpy(h->pending, raw + out_nframes * input_framesize, h->pending_len);

    if ( out_nframes == 0 ) {
	free(raw);
	return 0;
    }

    switch ( h->input_format ) {
	case SA_STDIN_FORMAT_S16LE:
	    if ( sa->format == SA_SAMPLE_FORMAT_FLOAT )
		sa_stdin_convert_s16le_to_float(raw, (float *)buf, out_nframes);
	    else
		memcpy(buf, raw, out_nframes * sa->channels * sa->samplesize);
	    break;
	case SA_STDIN_FORMAT_F32LE:
	    if ( !sa_stdin_is_little_endian() ) {
		fprintf(stderr, "stdin: f32le input is only supported on little-endian hosts\n");
		free(raw);
		return -1;
	    }
	    if ( sa->format == SA_SAMPLE_FORMAT_FLOAT )
		memcpy(buf, raw, out_nframes * sa->channels * sa->samplesize);
	    else {
		fprintf(stderr, "stdin: f32le input cannot be read as s16 samples\n");
		free(raw);
		return -1;
	    }
	    break;
	default:
	    assert(0);
    }

    free(raw);
    return out_nframes;
}

static ssize_t
sa_stdin_write( simpleaudio *sa, void *buf, size_t nframes )
{
    fprintf(stderr, "stdin: playback is not supported\n");
    return -1;
}

static void
sa_stdin_close( simpleaudio *sa )
{
    free(sa->backend_handle);
}

static int
sa_stdin_open_stream(
		simpleaudio *sa,
		const char *backend_device,
		sa_direction_t sa_stream_direction,
		sa_format_t sa_format,
		unsigned int rate, unsigned int channels,
		char *app_name, char *stream_name )
{
    if ( sa_stream_direction != SA_STREAM_RECORD ) {
	fprintf(stderr, "stdin: playback is not supported\n");
	return 0;
    }

    if ( channels != 1 ) {
	fprintf(stderr, "stdin: only 1-channel input is supported\n");
	return 0;
    }

    if ( !backend_device )
	backend_device = "s16le";

    sa_stdin_handle_t *h = calloc(1, sizeof(sa_stdin_handle_t));
    if ( !h ) {
	perror("malloc");
	return 0;
    }

    if ( strcmp(backend_device, "s16le") == 0 ) {
	h->input_format = SA_STDIN_FORMAT_S16LE;
	h->input_samplesize = 2;
    } else if ( strcmp(backend_device, "f32le") == 0 || strcmp(backend_device, "f32") == 0 ) {
	h->input_format = SA_STDIN_FORMAT_F32LE;
	h->input_samplesize = 4;
    } else {
	fprintf(stderr, "stdin: unsupported sample format '%s' (expected s16le or f32)\n",
		backend_device);
	free(h);
	return 0;
    }

    sa->rate = rate;
    sa->channels = channels;
    sa->backend_handle = h;
    sa->backend_framesize = sa->channels * sa->samplesize;

    return 1;
}

const struct simpleaudio_backend simpleaudio_backend_stdin = {
    sa_stdin_open_stream,
    sa_stdin_read,
    sa_stdin_write,
    sa_stdin_close,
};
