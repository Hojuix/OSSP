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
void OSSP_localMusicHandler_writeArtistToDb(int idx);
void OSSP_localMusicHandler_writeAlbumToDb(int idx);

int OSSP_localMusicHandler_checkMemoryDb();

void OSSP_localMusicHandler_songReq_Deconstructor(OSSP_localMusicHandler_songReq_t* obj);


void OSSP_localMusicHandler_scanForUniqueArtists();


OSSP_localMusicHandler_songReq_t* OSSP_localMusicHandler_fetchAllDb();





typedef struct {
    char* artist_uid;
    char* artist_name;
} OSSP_localMusicHandler_artistReq_artist_t;
typedef struct {
    int artist_count;
    OSSP_localMusicHandler_artistReq_artist_t* artists;
} OSSP_localMusicHandler_artistReq_t;
OSSP_localMusicHandler_artistReq_t* OSSP_localMusicHandler_fetchAllArtistsFromDb();








typedef struct {
    char* album_uid;
    char* album_name;
} OSSP_localMusicHandler_albumReq_albums_t;

typedef struct {
    char* artist_name;
    int album_count;
    OSSP_localMusicHandler_albumReq_albums_t* albums;
} OSSP_localMusicHandler_albumReq_t;

void OSSP_localMusicHandler_scanForUniqueAlbums();

OSSP_localMusicHandler_artistReq_t* OSSP_localMusicHandler_fetchAllArtists(void);
OSSP_localMusicHandler_albumReq_t* OSSP_localMusicHandler_fetchAllAlbumsByArtistUid(char* artist_uid);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LOCALMUSICHANDLER_H
