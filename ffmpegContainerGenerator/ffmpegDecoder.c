/*
 * ffmpegDecoder.c
 *
 *  Created on: Aug 14, 2016
 *      Author: alam
 */
#include "ffmpegDecoder.h"

static int32_t allocatePicBuffer(unsigned char **buf, AVFrame **pic,
		int32_t width, int32_t height, enum AVPixelFormat pix_fmt,
		int32_t *buf_size);

void *initDisplay(int32_t srcWidth, int32_t srcHeight,
		enum AVPixelFormat srcPixel, int32_t dstWidth, int32_t dstHeight) {
	SDL_DISPLAY_T *display = NULL;
	int32_t err = 0;
	int flags = SDL_SWSURFACE | SDL_RESIZABLE /*|SDL_FULLSCREEN*/;

	display = (SDL_DISPLAY_T *) malloc(sizeof(SDL_DISPLAY_T));
	if (display == NULL) {
		printf("malloc failed");
		return NULL;
	}
	memset(display, 0, sizeof(SDL_DISPLAY_T));

	display->width = srcWidth;
	display->height = srcHeight;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		return NULL;
	}

	display->surface = SDL_SetVideoMode(srcWidth, srcHeight, 24, flags);
	if (!display->surface) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		return NULL;
	}

	display->overlay = SDL_CreateYUVOverlay(srcWidth, srcHeight, SDL_YV12_OVERLAY,
			display->surface);

	display->imageCtx = sws_getContext(srcWidth, srcHeight, srcPixel, dstWidth,
			dstHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	display->videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!display->videoCodec) {
		fprintf(stderr, "avcodec_find_decoder failed\n");
		return NULL;;
	}

	display->videoDecodeCtx = avcodec_alloc_context3(display->videoCodec);
	err = avcodec_open2(display->videoDecodeCtx, display->videoCodec, NULL);
	if (err < 0) {
		fprintf(stderr, "avcodec_open2 failed\n");
		return NULL;
	}
	display->pFrame=av_frame_alloc();
	/*err = allocatePicBuffer(&display->bufYUV, &display->picYUV, srcWidth,
			srcHeight, AV_PIX_FMT_YUV420P, &display->sizeYUV);
	if (0 != err) {
		fprintf(stderr, "allocatePicBuffer failed\n");
		return NULL;
	}

	err = allocatePicBuffer(&display->bufRGB, &display->picRGB, srcWidth,
			srcHeight, AV_PIX_FMT_RGB24, &display->sizeRGB);
	if (0 != err) {
		fprintf(stderr, "allocatePicBuffer failed\n");
		return NULL;
	}*/

	return display;
}

int32_t allocatePicBuffer(unsigned char **buf, AVFrame **pic, int32_t width,
		int32_t height, enum AVPixelFormat pix_fmt, int32_t *buf_size) {
	int32_t picSize = 0;
	unsigned char *frameBuf = NULL;
	AVFrame *framePic = NULL;

	//picSize = avpicture_get_size(pix_fmt, width, height);
	picSize = av_image_get_buffer_size(pix_fmt, width, height, 1);
	frameBuf = (unsigned char *) (av_malloc(picSize));
	framePic = av_frame_alloc();
	if (NULL == frameBuf || NULL == framePic) {
		return -1;
	}

	//avpicture_fill((AVPicture *) framePic, frameBuf, pix_fmt, width, height);
	//av_image_fill_arrays()
	*pic = framePic;
	*buf = frameBuf;
	*buf_size = picSize;
	return 0;;
}


int32_t checkSDLStatus() {
	SDL_Event       event;
	SDL_PollEvent(&event);
	switch(event.type) {
		case SDL_QUIT:
	    {
	    	SDL_Quit();
	    	printf("Display quitting...\n");
	    	return -1;
	    	break;
	    }
	    default:
	      break;
	}
	return 1;
}

int32_t displayH264Frame(void *data, unsigned char *buffer, size_t buffLen) {
	SDL_DISPLAY_T *display = data;
	int32_t err = 0;
	int32_t gotPic = 0;
	int32_t height = 0;
	AVPacket packet;

	err = checkSDLStatus();
	if (err < 0) {
		return -1;
	}

	av_init_packet(&packet);
	err = av_new_packet(&packet, buffLen);
	if (err < 0) {
		return -1;
	}
	packet.size = buffLen;
	memcpy(&packet.data[0], buffer, buffLen);
	err = avcodec_decode_video2(display->videoDecodeCtx, display->pFrame,
			&gotPic, &packet);

	if (err < 0) {
		fprintf(stderr, "avcodec_decode_video2 failed\n");
		av_free_packet(&packet);
		return 0;
	}

	if (gotPic > 0) {
		SDL_LockYUVOverlay(display->overlay);
		AVFrame pict;
		pict.data[0] = display->overlay->pixels[0];
		pict.data[1] = display->overlay->pixels[2];
		pict.data[2] = display->overlay->pixels[1];

		pict.linesize[0] = display->overlay->pitches[0];
		pict.linesize[1] = display->overlay->pitches[2];
		pict.linesize[2] = display->overlay->pitches[1];

		height = sws_scale(display->imageCtx,
				(const uint8_t* const *) display->pFrame->data,
				display->pFrame->linesize, 0, display->height, pict.data,
				pict.linesize);

		if (height <= 0) {
			av_free_packet(&packet);
			sws_freeContext(display->imageCtx);
			return -1;
		}
		SDL_UnlockYUVOverlay(display->overlay);

		display->rect.x = 0;
		display->rect.y = 0;
		display->rect.w = display->width;
		display->rect.h = display->height;
		SDL_DisplayYUVOverlay(display->overlay, &display->rect);
	}

	av_free_packet(&packet);
	return 0;
}

void closeDisplay(void *data) {
	SDL_DISPLAY_T *display = data;

	if(!display) {
		return;
	}

	if (display->picRGB) {
		av_frame_free(&display->picRGB);
		display->picRGB = NULL;
	}

	if (display->picYUV) {
		av_frame_free(&display->picYUV);
		display->picYUV = NULL;
	}

	if (display->bufRGB) {
		av_free(display->bufRGB);
		display->bufRGB = NULL;
	}

	if (display->bufYUV) {
		av_free(display->bufYUV);
		display->bufYUV = NULL;
	}

	if (display->pFrame) {
		av_frame_free(&display->pFrame);
		display->pFrame = NULL;
	}

	if (display->imageCtx) {
		sws_freeContext(display->imageCtx);
		display->imageCtx = NULL;
	}

	if (display->overlay) {
		SDL_FreeYUVOverlay(display->overlay);
	}

	if(display->videoDecodeCtx) {
		//avcodec_close(&display->videoCodec);
		avcodec_free_context(&display->videoDecodeCtx);
		display->videoDecodeCtx = NULL;
	}
	SDL_Quit();
	free(display);
	display = NULL;
}
