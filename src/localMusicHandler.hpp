/*
 * OpenSubsonicPlayer (OSSP)
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Local Music Handler
 */

#ifndef _LOCALMUSICHANDLER_H
#define _LOCALMUSICHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    char* song_uid;
    char* album_uid;
    char* artist_uid;
    char* song_name;
    char* album_name;
    char* artist_name;
    char* filepath;
    int has_inbuilt_lyrics;
    long duration;
    long filesize;
    int album_track_number;
    int total_album_tracks;
} OSSP_localMusicHandler_songReq_songs_t;

typedef struct {
    int songCount;
    OSSP_localMusicHandler_songReq_songs_t* songs;
} OSSP_localMusicHandler_songReq_t;

int OSSP_localMusicHandler_checkDatabase();
void OSSP_localMusicHandler_scanMusic();
void OSSP_localMusicHandler_scanDirectory(char* directory);
void OSSP_localMusicHandler_scanFile(char* file);
char* OSSP_localMusicHandler_generateUID(char* text);
void OSSP_localMusicHandler_writeToDb();
void OSSP_localMusicHandler_writeSongToDb(int idx);

int OSSP_localMusicHandler_checkMemoryDb();
OSSP_localMusicHandler_songReq_t* OSSP_localMusicHandler_fetchAllDb();
void OSSP_localMusicHandler_songReq_Deconstructor(OSSP_localMusicHandler_songReq_t* obj);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LOCALMUSICHANDLER_H
