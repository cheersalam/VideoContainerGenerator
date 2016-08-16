/*
 * VideoContainerGenerator.c
 *
 *  Created on: Jul 26, 2016
 *      Author: alam
 */

#include "VideoContainerGenerator.h"
#include "vcgFfmpegIncludes.h"

#define MAX_FRAME_SIZE ((1024) * (1024))
static const int32_t CLIP_BASE_SIZE = (10 * 1024 * 1024); //10 MB
static const int32_t CLIP_EXTEND_SIZE = (1024 * 1024);   // 1MB

static int32_t addStream(VCG_CONTAINER_DATA_T *data, VCG_CODEC_ID_T codecId,
        int32_t streamId);
static int32_t muxInit(VCG_CONTAINER_DATA_T *data);
static int32_t startSegment(VCG_CONTAINER_DATA_T *data);
static int32_t allocateMemoryforClip(VCG_CONTAINER_DATA_T *data,
        size_t newClipSize);
static void stopAndClear(VCG_CONTAINER_DATA_T *data);
static int32_t stopSegment(VCG_CONTAINER_DATA_T *data);

void *initContainer(int32_t width, int32_t height, VCG_CONTAINER_FMT_T fmt,
        VCG_CODEC_ID_T audioCodec, VCG_CODEC_ID_T videoCodec,
        int32_t clipDurationInSec, SAVECLIP_CB cb) {
    VCG_CONTAINER_DATA_T *data = NULL;

    //Register FFMPEG
    av_register_all();
    avcodec_register_all();
    avformat_network_init();
    //av_log_set_level(AV_LOG_QUIET);

    data = (VCG_CONTAINER_DATA_T *) malloc(sizeof(VCG_CONTAINER_DATA_T));
    if (NULL == data) {
        return NULL;
    }

    memset(data, 0, sizeof(data));
    data->width = width;
    data->height = height;
    data->fmt = fmt;
    data->clipDurationInSec = clipDurationInSec;
    data->videoCodec = videoCodec;
    data->audioCodec = audioCodec;
    data->saveClipCallback = cb;

    startSegment(data);
    return data;
}

static int32_t getNALUnit(unsigned char *buffer, size_t buffLen, size_t *offset) {
    size_t startPos = *offset;
    size_t pos = *offset;
    int32_t foundStart = 0;
    while (pos < buffLen) {
        if (buffer[pos] == 0 && buffer[pos + 1] == 0 && buffer[pos + 2] == 0
                && buffer[pos + 3] == 1) {
            if (foundStart == 0) {
                foundStart = 1;
                startPos = pos;
            } else {
                break;
            }
        }
        pos++;
    }
    *offset = startPos;
//    printf("getNALUnit offset = %zu len = %zu\n", *offset, pos - startPos);
    return pos - startPos;
}

void savesps(VCG_CONTAINER_DATA_T *vcg, unsigned char *buffer, size_t len) {
    VCG_CONTAINER_DATA_T *data = vcg;
    if (!buffer || len <= 0) {
        return;
    }

    if (data->sps) {
        if (!memcmp(data->sps, buffer, len)) {
            return;
        }
        free(data->sps);
    }
    data->sps = (unsigned char *) malloc(len);
    memcpy(data->sps, buffer, len);
    data->spsLen = len;
}

void savepps(VCG_CONTAINER_DATA_T *vcg, unsigned char *buffer, size_t len) {
    VCG_CONTAINER_DATA_T *data = vcg;
    if (!buffer || len <= 0) {
        return;
    }

    if (data->pps) {
        if (!memcmp(data->pps, buffer, len)) {
            return;
        }
        free(data->pps);
    }
    data->pps = (unsigned char *) malloc(len);
    memcpy(data->pps, buffer, len);
    data->ppsLen = len;
}

static int32_t savePacketToContainer(VCG_CONTAINER_DATA_T *data,
        unsigned char *buffer, size_t buffLen, VCG_FRAME_T frameType,
        int64_t pts, int64_t dts) {
    int32_t err = 0;
    size_t packetLen = 0;
    AVPacket packet;
    AVRational videoTime = { 0 };
    av_init_packet(&packet);

    if (frameType == VCG_FRAME_I) {
        /*now allocate packet*/
        packetLen = buffLen + data->spsLen + data->ppsLen;
        err = av_new_packet(&packet, packetLen);
        if (err < 0) {
            return -1;
        }
        memcpy(&packet.data[0], data->sps, data->spsLen);
        memcpy(&packet.data[data->spsLen], data->pps, data->ppsLen);
        memcpy(&packet.data[data->spsLen + data->ppsLen], buffer, buffLen);
        packet.size = packetLen;
        packet.flags |= AV_PKT_FLAG_KEY;
    }
    if (frameType == VCG_FRAME_P) {
        /*now allocate packet*/
        err = av_new_packet(&packet, buffLen);
        if (err < 0) {
            return -1;
        }
        memcpy(&packet.data[0], buffer, buffLen);
        packet.size = buffLen;
    }
    packet.stream_index = data->videoid;
    if (packet.stream_index == data->videoid) {
        videoTime.num = 1;
        videoTime.den = data->videoTimeBaseDen;
        packet.pts = av_rescale_q(pts, videoTime,
                data->containerCtx->streams[data->videoid]->time_base);
        packet.dts = av_rescale_q(dts, videoTime,
                data->containerCtx->streams[data->videoid]->time_base);
    }
    return av_interleaved_write_frame(data->containerCtx, &packet);
}

static void writeVideoPacket(VCG_CONTAINER_DATA_T *data, unsigned char *buffer,
        size_t buffLen, int64_t pts, int64_t dts) {
    size_t offset = 0;
    size_t nalLen = 0;

    while ((nalLen = getNALUnit(buffer, buffLen, &offset)) > 0) {
        if (VCG_NAL_SLICE == (buffer[offset + 4] & 0x1F)) {
            //printf("P buffer received with len  = %zu\n", nalLen);
            savePacketToContainer(data, &buffer[offset], nalLen, VCG_FRAME_P,
                    pts, dts);
            data->frameCount++;
        } else if (VCG_NAL_SPS == (buffer[offset + 4] & 0x1F)) {
            //printf("SPS buffer received with len  = %zu\n", nalLen);
            savesps(data, &buffer[offset], nalLen);
            //printf("SPS buffer received with len  = %d\n", nalLen);
        } else if (VCG_NAL_PPS == (buffer[offset + 4] & 0x1F)) {
            printf("PPS buffer received with len  = %zu\n", nalLen);
            savepps(data, &buffer[offset], nalLen);
            //printf("PPS buffer received with len  = %d\n", nalLen);
        } else if (VCG_NAL_IDR == (buffer[offset + 4] & 0x1F)) {
            //printf("IDR buffer received with len  = %zu\n", nalLen);
            printf("pts  = %zu lapse = %zu\n", pts, data->timeLapsedInMsec);
            if (pts == 0 || pts - data->timeLapsedInMsec >= 1000) {
                if (data->isSegmentStarted) {
                    stopSegment(data);
                }
                startSegment(data);
                data->isSegmentStarted = 1;
                data->timeLapsedInMsec = pts;
            }
            savePacketToContainer(data, buffer, nalLen, VCG_FRAME_I, pts, dts);
            data->frameCount++;
        } else {
            printf("UNKNOWN NAL %x\n", buffer[offset + 4]);
        }
        offset += nalLen;
    }
}

int32_t writeFrame(void *vcg, unsigned char *buffer, size_t buffLen,
        VCG_FRAME_TYPE_T frameType, int64_t pts, int64_t dts) {
    VCG_CONTAINER_DATA_T *data = vcg;
    if (!buffer && buffLen <= 0) {
        return -1;
    }
    if (frameType == VCG_FRAME_VIDEO_COMPLETE) {
        //printf("Video frame received = %d size = %d\n", data->frameCount++, buffLen);
        writeVideoPacket(data, buffer, buffLen, pts, dts);
    }
    return 0;
}

static int32_t startSegment(VCG_CONTAINER_DATA_T *data) {
    int32_t err = 0;
    printf("startSegment......\n");
    err = muxInit(data);
    data->videoTimeBaseDen =
            data->containerCtx->streams[data->videoid]->time_base.den;
    if (err < 0) {
        return -1;
    }
    return avformat_write_header(data->containerCtx, NULL);
}

static int32_t stopSegment(VCG_CONTAINER_DATA_T *data) {
    int32_t err = 0;
    printf("stopSegment......\n");
    if (data->containerCtx) {
        av_write_trailer(data->containerCtx);
        if (data->saveClipCallback) {
            data->saveClipCallback(data->clip, data->curClipSize);
            data->curClipPos = 0;
        }
    }
}

void closeContainer(void *object) {
    VCG_CONTAINER_DATA_T *data = object;
    stopSegment(data);
    stopAndClear(data);
    free(data);
    data = NULL;
}

static int32_t allocateMemoryforClip(VCG_CONTAINER_DATA_T *data,
        size_t newClipSize) {
    void *newPtr = NULL;
    if (data) {
        //alam: I know what I am doing
        newPtr = realloc(data->clip, newClipSize);
    }
    if (NULL == newPtr) {
        return -1;
    }

    data->clip = newPtr;
    data->maxClipSize = newClipSize;
    return 0;
}

static int readPacket(void *opaque, unsigned char *buf, int buf_size) {
    VCG_CONTAINER_DATA_T *data = (VCG_CONTAINER_DATA_T *) opaque;
    int bytes_read = 0;

    if (NULL == data) {
        return -1;
    }

    if (data->curClipPos + buf_size > data->curClipSize) {
        bytes_read = data->curClipSize - data->curClipPos;
    } else {
        bytes_read = buf_size;
    }
    memcpy(buf, &data->clip[data->curClipPos], bytes_read);
    return bytes_read;
}

static int writePacket(void *opaque, unsigned char *buf, int buf_size) {
    int32_t err = 0;
    VCG_CONTAINER_DATA_T *data = (VCG_CONTAINER_DATA_T *) opaque;
    size_t newClipSize = 0;

    //printf("writePacket %d\n", buf_size);
    if (NULL == data) {
        return -1;
    }

    if (data->maxClipSize < data->curClipSize + buf_size) {
        newClipSize = data->maxClipSize + CLIP_EXTEND_SIZE;
        err = allocateMemoryforClip(data, newClipSize);
        if (err == -1) {
            return 0;
        }
    }
    memcpy(&data->clip[data->curClipPos], buf, buf_size);
    data->curClipSize += buf_size;
    data->curClipPos += buf_size;
    return buf_size;
}

static int64_t seekPacket(void *opaque, int64_t offset, int whence) {
    VCG_CONTAINER_DATA_T *data = (VCG_CONTAINER_DATA_T *) opaque;
    int64_t new_offset = -1;

    printf("seekPacket %d\n", whence);
    if (NULL == data) {
        return -1;
    }

    switch (whence) {
    case SEEK_CUR:
        if (data->curClipPos + offset <= (int) data->curClipSize) { //In negative???
            new_offset = data->curClipPos + offset;
        }
        break;
    case SEEK_SET:
        if (offset <= (int) data->curClipSize) {
            new_offset = offset;
        }
        break;
    case SEEK_END:
        if (offset <= (int) data->curClipSize) {
            new_offset = data->curClipSize - offset;
        }
        break;
    default:
        break;
    }
    if (new_offset != -1) {
        data->curClipPos = new_offset;
    }
    return new_offset;
}

static int32_t muxInit(VCG_CONTAINER_DATA_T *data) {
    AVOutputFormat *outputFmt = NULL;
    int32_t err = 0;
    int32_t mediaId = 0;

    if (VCG_CONTAINER_MPEGTS == data->fmt) {
        outputFmt = av_guess_format("mpegts", NULL, NULL);
    }

    if (NULL == outputFmt) {
        return -1;
    }

    err = avformat_alloc_output_context2(&data->containerCtx, outputFmt, NULL,
    NULL);
    if (err < 0) {
        return -1;
    }

    if (data->videoCodec != VCG_CODEC_ID_NONE) {
        data->videoid = mediaId++;
        err = addStream(data, data->videoCodec, data->videoid);
        if (err < 0) {
            return -1;
        }
    }

    if (!data->frameBuf) {
        data->frameBuf = av_malloc(MAX_FRAME_SIZE);
        if (!data->frameBuf) {
            return -1;
        }
    }

    if (!data->clip) {
        memset(&data->clip[0], 0, data->maxClipSize);
        data->curClipSize = 0;
    }
    else { 
        allocateMemoryforClip(data, CLIP_BASE_SIZE);
    }

    /*Assign buffer to container_context*/
    data->containerCtx->pb = avio_alloc_context(data->frameBuf, MAX_FRAME_SIZE,
    AVIO_FLAG_WRITE, data, readPacket, writePacket, seekPacket);

    return 0;
}

static int32_t addStream(VCG_CONTAINER_DATA_T *data, VCG_CODEC_ID_T codecId,
        int32_t streamId) {
    AVStream *stream = NULL;
    AVCodec *codec = NULL;

    stream = avformat_new_stream(data->containerCtx, NULL);
    if (!stream) {
        fprintf(stderr, "Could not allocate stream\n");
        return -1;
    }

    stream->id = streamId;
    stream->index = streamId;
    //avcodec_get_context_defaults3(stream->codecpar, codec);

    switch (codecId) {
    case VCG_CODEC_ID_H264:
        stream->codecpar->codec_id = AV_CODEC_ID_H264;
        stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        stream->codecpar->width = data->width;
        stream->codecpar->height = data->height;
        stream->time_base.num = 1;
        stream->time_base.den = 1000;
        //stream->codecpar-> ->pix_fmt = AV_PIX_FMT_YUV420P;
        //stream->codecpar->flags |= CODEC_FLAG_GLOBAL_HEADER;
        //stream->time_base = (AVRational ) { 1, 30 };
        //data->videoCodecContext = codecContext;
    }

    if (data->containerCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        data->containerCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    return 0;
}

static void stopAndClear(VCG_CONTAINER_DATA_T *data) {
    int32_t i = 0;
    if (data->clip) {
        free(data->clip);
        data->clip = NULL;
    }

    if (data->pps) {
        free(data->pps);
        data->pps = NULL;
    }
    if (data->sps) {
        free(data->sps);
        data->sps = NULL;
    }

    if (data->containerCtx) {
        for (i = 0; i < data->containerCtx->nb_streams; i++) {
            //avcodec_close(data->containerCtx->streams[i]->codec);
            //avformat_free_context(data->containerCtx->streams[i]);
        }
        if (data->frameBuf) {
            av_free(data->frameBuf);
            data->frameBuf = NULL;
        }

        if (data->containerCtx->pb) {
            av_free(data->containerCtx->pb);
            data->containerCtx->pb = NULL;
        }
        avformat_free_context(data->containerCtx);
    }
}
