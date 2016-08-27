#include <stdio.h>
#include <stdint.h>
#include <sys/queue.h>

#define VS_PATH_MAX (4096)
#define VS_NAME_MAX (256)

typedef struct MEDIA_FILES_T {
	char filename[VS_NAME_MAX];
	char path[VS_PATH_MAX];
	int64_t durationInMsec;
	int32_t sequence;
	CIRCLEQ_ENTRY(MEDIA_FILES_T) entries;
}MEDIA_FILES_T;

CIRCLEQ_HEAD(circleq, MEDIA_FILES_T);

typedef struct PLAYLIST_T {
	char playlistName[VS_NAME_MAX];
	char path[VS_PATH_MAX];
	int32_t filesInaList;
	struct circleq circleqHead;
}PLAYLIST_T;


void *initPlayList(char *path, char *playlistName);
int32_t addFileToPlaylist(void *data, int64_t durationms, char *fileName, char *path);
void finalizePlaylist(void *data);
