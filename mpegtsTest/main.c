/*
 * main.c
 *
 *  Created on: Jul 26, 2016
 *      Author: alam
 */

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include "VideoContainerGenerator.h"
#include "ffmpegDecoder.h"
#include "createPlaylist.h"


static volatile int32_t startExit = 0;
static void saveClip(unsigned char *buffer, int32_t bufLen, int64_t durationMsec);

struct sigaction sigact;

static void signalHandler(int sig){
    if (sig == SIGINT) {
    	printf("\nBye Bye... \n");
    	startExit = 1;
    }
}

void initSignals(void){
    sigact.sa_handler = signalHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *)NULL);
}

void *playlist = NULL;
static int32_t clipCount = 1;
int32_t main(int argc, char **argv) {
	FILE *fp = NULL;
	int32_t frameCount = 0;
	size_t filesize = 0;
	size_t offset = 0;
	size_t startPos = 0;
	size_t frameSize = 0;
	char *filename = NULL;
	unsigned char *buffer = NULL;
	unsigned char *frame = NULL;
	void *vcg = NULL;
	void *display = NULL;

	int32_t err = 0;

	if (argc < 2) {
		printf("Need h264 file name\n");
		return 1;
	}

	initSignals();
	vcg = initContainer(1920, 1080, VCG_CONTAINER_MPEGTS, VCG_CODEC_ID_NONE,
			VCG_CODEC_ID_H264, 1000, &saveClip);
	if (vcg == NULL) {
		printf("initContainer failed \n");
		return 1;
	}

	display = initDisplay(1920, 1080, AV_PIX_FMT_YUV420P, 1920, 1080);
	if (display == NULL) {
			printf("failed in creating a display\n");
			return 1;
		}

	filename = argv[1];
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("file not found\n");
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if (filesize <= 4) {
		printf("FIle empty\n");
		fclose(fp);
		return 0;
	}

	playlist = initPlayList("/var/www/html/parrot", "test");
	if(playlist == NULL) {
		return 0;
	}

	buffer = (unsigned char *) malloc(filesize);
	frame = (unsigned char *) malloc(filesize);
	fread(buffer, filesize, 1, fp);

	while (offset < filesize && !startExit && !err) {
		if (buffer[offset] == 0 && buffer[offset + 1] == 0
				&& buffer[offset + 2] == 0 && buffer[offset + 3] == 1) {

			frameSize = offset - startPos;
			//printf("\n\nstartPos = %d offset = %d filesize = %d frameSize = %d\n", startPos, offset, filesize, frameSize);
			if (frameSize) {
				memset(frame, 0, filesize);
				memcpy(frame, &buffer[startPos], frameSize);
				err = writeFrame(vcg, frame, frameSize,
						VCG_FRAME_VIDEO_COMPLETE, (40 * frameCount),
						(40 * frameCount));
				err = displayH264Frame(display, frame, frameSize);
				if (frameSize > 100)
					frameCount++;
			}
			startPos = offset;
		}
		offset++;
	}

	frameSize = offset - startPos;
	if (frameSize) {
		memset(frame, 0, filesize);
		memcpy(frame, &buffer[startPos], frameSize);
	}

	closeContainer(vcg);
	closeDisplay(display);
	finalizePlaylist(playlist);
	if (buffer) {
		free(buffer);
	}

	if (frame) {
		free(frame);
	}

	fclose(fp);
	return 0;
}

static void saveClip(unsigned char *buffer, int32_t bufLen, int64_t durationMsec) {
	FILE *fp = NULL;
	char filename[64];

	if (buffer && bufLen > 0) {
		sprintf(filename, "/var/www/html/parrot/%s%d%s", "clip", clipCount++, ".ts");
		fp = fopen(filename, "wb");
		fwrite(buffer, bufLen, 1, fp);
		fclose(fp);
		addFileToPlaylist(playlist, durationMsec, filename, "/var/www/html/parrot");
	}
}

