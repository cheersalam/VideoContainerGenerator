/*
 * VideoContainerGenerator.h
 *
 *  Created on: Jul 26, 2016
 *      Author: alam
 */

#ifndef INC_VIDEOCONTAINERGENERATOR_H_
#define INC_VIDEOCONTAINERGENERATOR_H_
#include <stdio.h>
#include <stdint.h>
#include "vcgFfmpegIncludes.h"

typedef enum VCG_CONTAINER_FMT_T {
	VCG_CONTAINER_UNKNOWN_FMT=0,
	VCG_CONTAINER_MPEGTS,
	VCG_CONTAINER_MP4,
	VCG_CONTAINER_FLV
}VCG_CONTAINER_FMT_T;

typedef enum VCG_FRAME_TYPE_T {
    VCG_FRAME_TYPE_ERROR=0,
    VCG_FRAME_AUDIO_COMPLETE,
    VCG_FRAME_VIDEO_COMPLETE
}VCG_FRAME_TYPE_T;

typedef enum VCG_FRAME_T {
    VCG_FRAME_NONE=0,
    VCG_FRAME_I,
    VCG_FRAME_P,
    VCG_FRAME_B
}VCG_FRAME_T;

typedef enum VCG_NAL_TYPE_T {
    VCG_NAL_UNKNOWN=0,
    VCG_NAL_SLICE=1,
    VCG_NAL_IDR=5,
    VCG_NAL_SEI,
    VCG_NAL_SPS,
    VCG_NAL_PPS
}VCG_NAL_TYPE_T;

typedef enum VCG_CODEC_ID_T {
    VCG_CODEC_ID_NONE=0,
    VCG_CODEC_ID_H264,
    VCG_CODEC_ID_AAC
}VCG_CODEC_ID_T;

typedef struct VCG_CONTAINER_DATA_T {
	char *url;
    unsigned char *clip;
    unsigned char *sps;
    unsigned char *pps;
    unsigned char *frameBuf;
	VCG_CONTAINER_FMT_T fmt;
	VCG_CODEC_ID_T audioCodec;
	VCG_CODEC_ID_T videoCodec;
	int32_t width;
	int32_t height;
	int32_t audioid;
	int32_t videoid;
	int32_t clipDurationInSec; //in seconds
	int32_t frameCount;
	int32_t spsLen;
	int32_t ppsLen;
	int64_t videoTimeBaseDen;
	size_t maxClipSize;
	size_t curClipSize;
	size_t curClipPos; //file position
	struct timeval startTime;
	AVFormatContext *containerCtx;
	AVCodecContext *videoCodecContext;
	AVCodecContext *audioCodecContext;
}VCG_CONTAINER_DATA_T;



void *initContainer(int32_t width, int32_t height, VCG_CONTAINER_FMT_T fmt, VCG_CODEC_ID_T audioCodec, VCG_CODEC_ID_T videoCodec, int32_t clipDurationInSec);
int32_t writeFrame(void *vcg, unsigned char *buffer, size_t buffLen, VCG_FRAME_TYPE_T frameType, int64_t pts, int64_t dts);
void closeContainer(void *vcg, char *filename);

#endif /* INC_VIDEOCONTAINERGENERATOR_H_ */
