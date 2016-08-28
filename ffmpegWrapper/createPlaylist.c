#include "createPlaylist.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define PLAYLIST_HEADER "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-MEDIA-SEQUENCE:%d\n#EXT-X-ALLOW-CACHE:NO\n#EXT-X-TARGETDURATION:%d\n\n"

void *initPlayList(char *path, char *playlistName) {
	PLAYLIST_T *playlist = NULL;
	playlist = (PLAYLIST_T *) malloc(sizeof(PLAYLIST_T));
	if (playlist != NULL) {
		memset(playlist, 0, sizeof(PLAYLIST_T));
		strncpy(playlist->path, path, VS_PATH_MAX - 64);
		strncpy(playlist->playlistName, playlistName, VS_NAME_MAX);
		CIRCLEQ_INIT(&playlist->circleqHead);
		printf("Playlist initialised\n");
	}
	return playlist;
}

int32_t addFileToPlaylist(void *data, int64_t durationms, char *fileName, char *path) {
	PLAYLIST_T *playlist = data;
	MEDIA_FILES_T *necwFile = NULL;
	MEDIA_FILES_T *readFileList = NULL;

	if (data == NULL || fileName == NULL || durationms == 0) {
		printf("addFileToPlaylist Invalid parametes\n");
		return -1;
	}

	newFile = (MEDIA_FILES_T *) malloc(sizeof(MEDIA_FILES_T));
	if (newFile) {
		newFile->durationInMsec = durationms;
		newFile->sequence = playlist->filesInaList++;
		strncpy(newFile->filename, fileName, VS_NAME_MAX);
		strncpy(newFile->path, path, VS_PATH_MAX - 64);
		CIRCLEQ_INSERT_TAIL(&playlist->circleqHead, newFile, entries);
	}
	cratePlaylist(data, 1);
	CIRCLEQ_FOREACH(readFileList, &playlist->circleqHead, entries)
	{
		//printf("Playlist: %s\n", readFileList->filename);
	}
	return 0;
}

void finalizePlaylist(void *data) {
	PLAYLIST_T *playlist = data;

	if (data == NULL) {
		printf("addFileToPlaylist Invalid parametes\n");
		return;
	}
	while (CIRCLEQ_FIRST(&playlist->circleqHead) != (void *)&playlist->circleqHead ) {
		//printf("Deleting from list: %s\n", playlist->circleqHead.cqh_first->filename);
		CIRCLEQ_REMOVE(&playlist->circleqHead, CIRCLEQ_FIRST(&playlist->circleqHead), entries);
	}
}

int32_t cratePlaylist(void *data, int32_t isLive) {
	int32_t playlistTargetDuration = 0;
	int32_t mediaSequence = 0;
	int32_t livePlaylistCount = 0;
	int64_t playlistDuration = 0;
	char fileFullPath[VS_PATH_MAX];
	char fileFullPathTmp[VS_PATH_MAX];
	FILE *fp = NULL;
	double fileTargetDuration = 0;
	MEDIA_FILES_T *readFileList = NULL;
	MEDIA_FILES_T *livePlaylist[10];
	PLAYLIST_T *playlist = data;

	sprintf(&fileFullPath[0], "%s/%s.m3u8", playlist->path, playlist->playlistName);
	sprintf(&fileFullPathTmp[0], "%s%s", fileFullPath, "tmp");
	fp = fopen(fileFullPathTmp, "w");
	if (fp == NULL) {
		printf("Error in opening file %s\n", fileFullPathTmp);
		return -1;
	}

	if (isLive) {
		fprintf(fp, PLAYLIST_HEADER, 10, 10);
		//Size of the HLS playlist should be atleast 3 times the size of target duration.
		CIRCLEQ_FOREACH_REVERSE(readFileList, &playlist->circleqHead, entries)
		{
			if(livePlaylistCount > 10) {
				printf("Something is wrong. Live playlist cannot be completed. segment file durations are not proper\n");
				break;
			}
			fileTargetDuration = ((double) readFileList->durationInMsec / 1000.0);
			mediaSequence = readFileList->sequence;
			if (ceil(fileTargetDuration) > playlistTargetDuration) {
				playlistTargetDuration = ceil(fileTargetDuration);
			}
			playlistDuration += readFileList->durationInMsec;
			livePlaylist[livePlaylistCount++] = readFileList;

			if (((double) playlistDuration / 1000.0) >= (double)(playlistTargetDuration * 3) ) {
				break;
			}
		}

		//Now adding segments information in playlist
		while(livePlaylistCount > 0) {
			livePlaylistCount--;
			//memset(fileFullPath, 0, VS_PATH_MAX);
			//sprintf(&fileFullPath[0], "%s/%s", livePlaylist[livePlaylistCount]->path, livePlaylist[livePlaylistCount]->filename);
			fprintf(fp, "#EXTINF:%f,\n%s\n", (double) livePlaylist[livePlaylistCount]->durationInMsec / 1000.0, livePlaylist[livePlaylistCount]->filename);
		}
		fseek(fp, 0, SEEK_SET);
		fprintf(fp, PLAYLIST_HEADER, mediaSequence, playlistTargetDuration);

	}
	fclose(fp);
	unlink(fileFullPath);
	rename(fileFullPathTmp, fileFullPath);

	return 0;

}
