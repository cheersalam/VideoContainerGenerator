/*
 * ffmpegDecoder.h
 *
 *  Created on: Aug 14, 2016
 *      Author: alam
 */

#ifndef FFMPEGDECODER_FFMPEGDECODER_H_
#define FFMPEGDECODER_FFMPEGDECODER_H_
#include <stdint.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "vcgFfmpegIncludes.h"

typedef struct  SDL_DISPLAY_T{
	int32_t width;
	int32_t height;
	SDL_Surface *surface;
	SDL_Overlay  *overlay;
	struct SWSContext *imageCtx;
	AVCodec *videoCodec;
	AVCodecContext *videoDecodeCtx;
	AVFrame *picRGB;
	AVFrame *picYUV;
	unsigned char *bufRGB;
	unsigned char *bufYUV;
	int32_t sizeRGB;
	int32_t sizeYUV;
	SDL_Rect rect;
	AVFrame  *pFrame;
}SDL_DISPLAY_T;

void *initDisplay(int32_t srcWidth, int32_t srcHeight,
		enum AVPixelFormat srcPixel, int32_t dstWidth, int32_t dstHeight);

int32_t displayH264Frame(void *data, unsigned char *buffer, size_t buffLen);

void closeDisplay(void *data);

#endif /* FFMPEGDECODER_FFMPEGDECODER_H_ */
