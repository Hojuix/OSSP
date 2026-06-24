/*
 * OpenSubsonicPlayer (OSSP)
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Local Music Handler
 */

/*
 * Note: I am very sorry for the messy / unorganized code, but this is the first time I have ever
 * done anything like this. I am going with safety over efficiency, which makes this code (relatively)
 * slow. But it works (seemingly very well)!
 */

/*
 * Note: Yes, I could just read in the entire database into memory when the application starts, but
 * with a not-so-big music library, this could easily take up a lot of memory. OSSP itself can easily
 * run under 100mb of total memory, so it makes sense to dynamically pull from the database file instead.
 * This also gives us some nice sqlite functions to very easily search by UIDs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/dict.h>
    #include <libavformat/avio.h>
    #include "external/sqlite3/sqlite3.h"
    #include "external/md5.h"
    #include "libopensubsonic/utils.h"
}
#include <iostream>
#include <regex>
#include <vector>
#include <deque>
#include <algorithm>
#include "configHandler.h"
#include "localMusicHandler.hpp"

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "OSSP", __VA_ARGS__)
#endif

// I have separate variables to check if names have been pulled via avformat so I can explicitly
// set them and not have to rely on checking string lengths (safer in my opinion)
class OSSP_localMusicHandler_SongObject {
    public:
        std::string song_uid;
        std::string album_uid;
        std::string artist_uid;
        std::string song_name;
        std::string album_name;
        std::string artist_name;
        std::string filepath;
        bool has_song_name;
        bool has_album_name;
        bool has_artist_name;
        int has_inbuilt_lyrics;
        long duration;
        long filesize;
        int album_track_number;
        int total_album_tracks;
        bool found_has_inbuilt_lyrics;
        bool found_album_track_number;
        bool found_total_album_tracks;
};
std::deque<OSSP_localMusicHandler_SongObject> OSSP_localMusicHandler_songObject;

class OSSP_localMusicHandler_ArtistObject {
    public:
        std::string artist_uid;
        std::string artist_name;
};
std::vector<OSSP_localMusicHandler_ArtistObject> OSSP_localMusicHandler_artistObject;

class OSSP_localMusicHandler_AlbumObject {
    public:
        std::string artist_uid;
        std::string artist_name;
        std::string album_uid;
        std::string album_name;
};
std::vector<OSSP_localMusicHandler_AlbumObject> OSSP_localMusicHandler_albumObject;

extern OSSP_config_t* configObj;
static sqlite3* sqlite_db = NULL;
static char* sqlite_errorMsg = NULL;
std::vector<std::string> OSSP_localMusicHandler_files;



/*
// P.S. Sqlite searching directly is fucking useless
// Have to use something on top, and just load base shit into memory completely I think
// Plus how many fucking songs would it take to actually become an issue?
std::deque<localMusicHandler_AudioObject> localMusicHandler_songReqDeque;
localMusicHandler_songReq_t* localMusicHandler_test() {
    // Test
    sqlite3_stmt* sqlite_stmt;
    //const char* sqlQuery = "SELECT * FROM local_songs WHERE artistTitle LIKE ?;";
    const char* sqlQuery = "SELECT * FROM local_songs;";

    if (sqlite3_prepare_v2(sqlite_db, sqlQuery, -1, &sqlite_stmt, NULL) != SQLITE_OK) {
        printf("fuck\n");
        //return;
    }
    //sqlite3_bind_text(sqlite_stmt, 1, "ch", -1, SQLITE_STATIC);

    // %text% to prevent SQL injection --> %% for % with asprintf, %%%s%%. fucking ridiculous but its it
    // Also rename these fuckass fields. 'artistTitle' what am I high??

    static int rc = 0;
    while ((rc = sqlite3_step(sqlite_stmt)) == SQLITE_ROW) {
        printf("here\n");
        localMusicHandler_AudioObject audioObject;
        // TODO THIS IS NOT SAFE HOLY FUCK TESTING ONLY LIKE MEGA ONLY
        // Could load directly into struct if I know how many songs there will be
        // Seems to be only able to do that by issuing yet another SQL request
        audioObject.path = (char*)sqlite3_column_text(sqlite_stmt, 6);
        audioObject.songTitle = (char*)sqlite3_column_text(sqlite_stmt, 1);
        audioObject.uid = (char*)sqlite3_column_text(sqlite_stmt, 0);
        audioObject.artistTitle = (char*)sqlite3_column_text(sqlite_stmt, 4);
        audioObject.duration = (long)sqlite3_column_text(sqlite_stmt, 10);
        localMusicHandler_songReqDeque.push_back(audioObject);
    }

    if (sqlite3_step(sqlite_stmt) != SQLITE_DONE) {
        printf("[LocalMusicHandler] Execution error: %s\n", sqlite3_errmsg(sqlite_db));
    }

    sqlite3_finalize(sqlite_stmt);

    // Load into actual struct
    int songCount = localMusicHandler_songReqDeque.size();
    localMusicHandler_songReq_t* songReq = (localMusicHandler_songReq_t*)malloc(sizeof(localMusicHandler_songReq_t));
    songReq->songCount = songCount;
    songReq->songs = (localMusicHandler_songReq_songs_t*)malloc(sizeof(localMusicHandler_songReq_songs_t) * songCount);
    for (int i = 0; i < songCount; i++) {
        localMusicHandler_AudioObject audioObject;
        audioObject = localMusicHandler_songReqDeque.front();
        localMusicHandler_songReqDeque.pop_front();
        songReq->songs[i].uid = strdup(audioObject.uid.c_str());
        songReq->songs[i].title = strdup(audioObject.songTitle.c_str());
        songReq->songs[i].path = strdup(audioObject.path.c_str());
        songReq->songs[i].artist = strdup(audioObject.artistTitle.c_str());
        songReq->songs[i].duration = audioObject.duration;
    }

    return songReq;
}*/


/*
 * START OF NEW CODE
 */





// Check if database exists. Returns 0 if database doesn't exist, else return size.
int OSSP_localMusicHandler_checkDatabase() {
    static int rc = 0;
    char* dbPath = NULL;

#if defined(__ANDROID__)
    rc = asprintf(&dbPath, "/sdcard/OSSP/localmusic.db");
#endif
    if (rc == -1) {
        printf("[OSSP_LocalMusicHandler] asprintf() failed.\n");
        rc = 0;
        goto cleanup;
    }

    struct stat st;
    if (stat(dbPath, &st) == 0) {
        if (st.st_size != 0) {
            // Local music database exists, return size
            rc = (int)st.st_size;
        } else {
            rc = 0;
        }
    } else {
        rc = 0;
    }

cleanup:
    OSS_SafeFree(dbPath);
    return rc;
}



void OSSP_localMusicHandler_scanMusic() {
    static int rc = 0;

    printf("[OSSP_LocalMusicHandler] Initiating scan.\n");

    // TODO clear all variables here

    // Scan the music directory recursively to find all files
    printf("[OSSP_LocalMusicHandler] Scanning directory %s for files.\n", configObj->local_music_rootdir);
    OSSP_localMusicHandler_scanDirectory(configObj->local_music_rootdir);
    if (OSSP_localMusicHandler_files.size() == 0) {
        printf("[OSSP_LocalMusicHandler] No files found in configured local music directory.\n");
        // TODO handle no music found
    }
    printf("[OSSP_LocalMusicHandler] Found %d files.\n", OSSP_localMusicHandler_files.size());

    // Scan each file ----
    for (int i = 0; i < OSSP_localMusicHandler_files.size(); i++) {
        // This function scans the file, pulls all the relevent metadata, and appends it to the song vector
        OSSP_localMusicHandler_scanFile((char*)OSSP_localMusicHandler_files[i].c_str()); // TODO Const issue
    }

    OSSP_localMusicHandler_scanForUniqueAlbums();

    // Scan all scanned songs and find all unique artists
    // This fills the uniqueArtists (type OSSP_localMusicHandler_ArtistObject) object
    OSSP_localMusicHandler_scanForUniqueArtists();

    // Write all data to database
    OSSP_localMusicHandler_writeToDb();


    // TESTING
    OSSP_localMusicHandler_fetchAllAlbumsByArtistUid("local_156bf8162314ed8ddcc97a46f6dd515d");
    OSSP_localMusicHandler_fetchAllArtists();
}

void OSSP_localMusicHandler_scanDirectory(char* directory) {
    struct dirent* dp;
    DIR* dir = opendir(directory);
    char path[2048];

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            snprintf(path, sizeof(path), "%s/%s", directory, dp->d_name);

            struct stat statbuf;
            stat(path, &statbuf);

            if (S_ISDIR(statbuf.st_mode)) {
                OSSP_localMusicHandler_scanDirectory(path);
            } else if (S_ISREG(statbuf.st_mode)) {
                OSSP_localMusicHandler_files.push_back(path);
            }
        }
    }

    closedir(dir);
}

void OSSP_localMusicHandler_scanFile(char* file) {
    static int rc = 0;
    AVFormatContext* ctx = NULL;
    AVDictionaryEntry* tag = NULL;
    OSSP_localMusicHandler_SongObject songObject;
    char* album_uid_raw = NULL;
    char* song_uid_raw = NULL;
    char* artist_uid = NULL;
    char* album_uid = NULL;
    char* song_uid = NULL;
    char* file_copy = NULL;
    char* base_filename = NULL;
    bool fileHasAudioStream = false;

    rc = avformat_open_input(&ctx, file, NULL, NULL);
    if (rc < 0) {
        printf("[OSSP_LocalMusicHandler] Could not open file %s.\n", file);
        return; // Return as there is nothing to cleanup
    }

    // Ignore files that are not audio
    /*
     * Notable file types include:
     * lrc -> Lyric files
     * image2 / png_pipe -> Images (cover images for example)
     * mov,mp4,m4a,3gp,3g2,mj2 -> MP4 files
     */
    if (
        strcmp(ctx->iformat->name, "mp3") != 0 &&
        strcmp(ctx->iformat->name, "ogg") != 0 &&
        strcmp(ctx->iformat->name, "flac") != 0
    ) {
        printf("[OSSP_LocalMusicHandler] File %s of format %s is not registed as audio.\n", file, ctx->iformat->name);
        goto cleanup;
    }

    // Explicitly setting everything to a known value for safety (Not sure if C++ does this automatically)
    songObject.song_uid = "";
    songObject.album_uid = "";
    songObject.artist_uid = "";
    songObject.song_name = "";              // User-facing metadata
    songObject.album_name = "";             // User-facing metadata
    songObject.artist_name = "";            // User-facing metadata
    songObject.filepath = "";
    songObject.has_song_name = false;
    songObject.has_album_name = false;
    songObject.has_artist_name = false;
    songObject.has_inbuilt_lyrics = 0;
    songObject.duration = 0;
    songObject.filesize = 0;
    songObject.album_track_number = 0;      // User-facing metadata
    songObject.total_album_tracks = 0;      // User-facing metadata
    songObject.found_album_track_number = false;
    songObject.found_total_album_tracks = false;

    // Get the stream information (Used for querying the duration)
    if (avformat_find_stream_info(ctx, NULL) < 0) {
        printf("[OSSP_LocalMusicHandler] Could not find stream info for file %s\n", file);
        goto cleanup;
    }

    // Check if there is an audio stream in the file
    if (ctx->nb_streams < 1) {
        printf("[OSSP_LocalMusicHandler] File %s has no audio streams.\n", file);
        goto cleanup;
    } else {
        for (int i = 0; i < ctx->nb_streams; i++) {
            enum AVMediaType media_type = ctx->streams[i]->codecpar->codec_type;
            if (media_type == AVMEDIA_TYPE_AUDIO) {
                fileHasAudioStream = true;
            }
        }
    }
    if (!fileHasAudioStream) {
        printf("[OSSP_LocalMusicHandler] File %s has %d streams but none are audio.\n", file, ctx->nb_streams);
        goto cleanup;
    }

    // Set file path (This path is not a file URI)
    songObject.filepath = file;

    // Get file size (Using libav for this since it's already in use here)
    if (ctx->pb) {
        uint64_t fsize = avio_size(ctx->pb);
        if (fsize > 0) {
            songObject.filesize = fsize;
        }
    }

    // Get stream duration
    if (ctx->duration != AV_NOPTS_VALUE) {
        uint64_t duration = ctx->duration;
        songObject.duration = (long)((double)duration / AV_TIME_BASE);
    }

    // Extract metadata
    /*
     * Depending on the file, the audio metadata is either stored in ctx->metadata or ctx->streams[x]->metadata
     * I honestly haven't found a reliable method of determining which one it will be, so scan both ways for safety
     * Also, I set booleans instead of checking the contents of variables to know if data has been found as data
     * coming in from these files is user-facing, so '0' could be a possible value for 'totaltracks', and without
     * separate variables for found fields, this could get very messy
     */
    // Scan type 1 - ctx->metadata
    while ((tag = av_dict_get(ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (strcasecmp(tag->key, "title") == 0) {
            songObject.has_song_name = true;
            songObject.song_name = tag->value;
        } else if (strcasecmp(tag->key, "album") == 0) {
            songObject.has_album_name = true;
            songObject.album_name = tag->value;
        } else if (strcasecmp(tag->key, "artist") == 0) {
            // In ID3, multiple artists are stored as 'Artist A;Artist B'. Replace ';' with ', '
            songObject.has_artist_name = true;
            songObject.artist_name = std::regex_replace(tag->value, std::regex(";"), ", ");
        } else if (strcasecmp(tag->key, "track") == 0) {
            songObject.found_album_track_number = true;
            songObject.album_track_number = std::stoi(tag->value);
        } else if (strcasecmp(tag->key, "totaltracks") == 0) {
            songObject.found_total_album_tracks = true;
            songObject.total_album_tracks = std::stoi(tag->value);
        } else if (strcasecmp(tag->key, "lyrics") == 0) {
            songObject.has_inbuilt_lyrics = 1;
        }
    }

    // Scan type 2 - ctx->streams[x]->metadata
    for (int i = 0; i < ctx->nb_streams; i++) {
        enum AVMediaType media_type = ctx->streams[i]->codecpar->codec_type;
        if (media_type == AVMEDIA_TYPE_AUDIO) {
            while ((tag = av_dict_get(ctx->streams[i]->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
                if (strcasecmp(tag->key, "title") == 0) {
                    if (!songObject.has_song_name) {
                        songObject.has_song_name = true;
                        songObject.song_name = tag->value;
                    }
                } else if (strcasecmp(tag->key, "album") == 0) {
                    if (!songObject.has_album_name) {
                        songObject.has_album_name = true;
                        songObject.album_name = tag->value;
                    }
                } else if (strcasecmp(tag->key, "artist") == 0) {
                    // In ID3, multiple artists are stored as 'Artist A;Artist B'. Replace ';' with ', '
                    if (!songObject.has_artist_name) {
                        songObject.has_artist_name = true;
                        songObject.artist_name = std::regex_replace(tag->value, std::regex(";"), ", ");
                    }
                } else if (strcasecmp(tag->key, "track") == 0) {
                    if (!songObject.found_album_track_number) {
                        songObject.found_album_track_number = true;
                        songObject.album_track_number = std::stoi(tag->value);
                    }
                } else if (strcasecmp(tag->key, "totaltracks") == 0) {
                    if (!songObject.found_total_album_tracks) {
                        songObject.found_total_album_tracks = true;
                        songObject.total_album_tracks = std::stoi(tag->value);
                    }
                } else if (strcasecmp(tag->key, "lyrics") == 0) {
                    if (songObject.has_inbuilt_lyrics == 0) {
                        songObject.has_inbuilt_lyrics = 1;
                    }
                }
            }
        }
    }

    // If either the album or artist name are unknown, set to default
    if (!songObject.has_album_name) {
        songObject.album_name = "Unknown Album";
    }
    if (!songObject.has_artist_name) {
        songObject.artist_name = "Unknown Artist";
    }

    // If the song name is unknown, set it to the filename
    if (!songObject.has_song_name) {
        // Use POSIX basename() to get the filename. basename() modifies string so copy
        file_copy = strdup(file);
        if (!file_copy) {
            goto cleanup;
        }
        base_filename = basename(file_copy);
        songObject.song_name = base_filename;
    }

    // Generate UIDs
    // TODO Deal with potential duplicates
    rc = asprintf(&album_uid_raw, "%s%s",
        songObject.artist_name.c_str(), songObject.album_name.c_str());
    if (rc == -1) {
        printf("[OSSP_LocalMusicHandler] asprintf() failed.\n");
        goto cleanup;
    }
    rc = asprintf(&song_uid_raw, "%s%s%s",
        songObject.artist_name.c_str(), songObject.album_name.c_str(), songObject.song_name.c_str());
    if (rc == -1) {
        printf("[OSSP_LocalMusicHandler] asprintf() failed.\n");
        goto cleanup;
    }

    artist_uid = OSSP_localMusicHandler_generateUID((char*)songObject.artist_name.c_str()); // Unsafe cast from const char*
    if (artist_uid == NULL) {
        goto cleanup;
    }
    album_uid = OSSP_localMusicHandler_generateUID(album_uid_raw);
    if (album_uid == NULL) {
        goto cleanup;
    }
    song_uid = OSSP_localMusicHandler_generateUID(song_uid_raw);
    if (song_uid == NULL) {
        goto cleanup;
    }

    songObject.artist_uid = artist_uid;
    songObject.album_uid = album_uid;
    songObject.song_uid = song_uid;
    OSSP_localMusicHandler_songObject.push_back(songObject);

cleanup:
    OSS_SafeFree(album_uid_raw);
    OSS_SafeFree(song_uid_raw);
    OSS_SafeFree(artist_uid);
    OSS_SafeFree(album_uid);
    OSS_SafeFree(song_uid);
    OSS_SafeFree(file_copy);
    avformat_close_input(&ctx);
    return;
}

char* OSSP_localMusicHandler_generateUID(char* text) {
    static int rc = 0;
    char* output = NULL;
    uint8_t md5_raw_output[16] = { 0x00 };
    
    // Generate raw MD5 byte array
    md5String(text, md5_raw_output);

    // Convert MD5 byte array to string
    rc = asprintf(&output, "local_%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        md5_raw_output[0], md5_raw_output[1], md5_raw_output[2], md5_raw_output[3],
        md5_raw_output[4], md5_raw_output[5], md5_raw_output[6], md5_raw_output[7],
        md5_raw_output[8], md5_raw_output[9], md5_raw_output[10], md5_raw_output[11],
        md5_raw_output[12], md5_raw_output[13], md5_raw_output[14], md5_raw_output[15]);
    if (rc == -1) {
        printf("[OSSP_LocalMusicHandler] asprintf() failed.\n");
        return NULL;
    }

    return output;
}

void OSSP_localMusicHandler_writeToDb() {
    static int rc = 0;
    char* dbPath = NULL;

#if defined(__ANDROID__)
    rc = asprintf(&dbPath, "/sdcard/OSSP/localmusic.db");
#endif
    if (rc == -1) {
        printf("[OSSP_LocalMusicHandler] asprintf() failed.\n");
        rc = 0;
        //goto cleanup;
    }

    rc = sqlite3_open(dbPath, &sqlite_db);
    if (rc) {
        printf("[OSSP_LocalMusicHandler] Could not create database: %s\n", sqlite3_errmsg(sqlite_db));
        //goto cleanup;
    }
    
    const char* dropSongTableSQLQ = "DROP TABLE IF EXISTS local_songs;";
    rc = sqlite3_exec(sqlite_db, dropSongTableSQLQ, 0, 0, &sqlite_errorMsg);
    if (rc != SQLITE_OK) {
        //
    }

    const char* dropArtistTableSQLQ = "DROP TABLE IF EXISTS all_artists;";
    rc = sqlite3_exec(sqlite_db, dropArtistTableSQLQ, 0, 0, &sqlite_errorMsg);
    if (rc != SQLITE_OK) {
        //
    }

    const char* dropAlbumTableSQLQ = "DROP TABLE IF EXISTS all_albums;";
    rc = sqlite3_exec(sqlite_db, dropAlbumTableSQLQ, 0, 0, &sqlite_errorMsg);
    if (rc != SQLITE_OK) {
        //
    }

    const char* createSongTableSQLQ = "CREATE TABLE local_songs(song_uid TEXT, album_uid TEXT, artist_uid TEXT, song_name TEXT, album_name TEXT, artist_name TEXT, has_inbuilt_lyrics INT, duration INT, filesize INT, album_track_number INT, total_album_tracks INT, filepath TEXT)";
    rc = sqlite3_exec(sqlite_db, createSongTableSQLQ, 0, 0, &sqlite_errorMsg);
    if (rc != SQLITE_OK) {
        //
    }

    const char* createArtistTableSQLQ = "CREATE TABLE all_artists(uid TEXT, name TEXT)";
    rc = sqlite3_exec(sqlite_db, createArtistTableSQLQ, 0, 0, &sqlite_errorMsg);
    if (rc != SQLITE_OK) {
        //
    }

    const char* createAlbumTableSQLQ = "CREATE TABLE all_albums(artist_uid TEXT, artist_name TEXT, album_uid TEXT, album_name TEXT)";
    rc = sqlite3_exec(sqlite_db, createAlbumTableSQLQ, 0, 0, &sqlite_errorMsg);
    if (rc != SQLITE_OK) {
        //
    }

    // Add all scanned songs to database
    for (int i = 0; i < OSSP_localMusicHandler_songObject.size(); i++) {
        OSSP_localMusicHandler_writeSongToDb(i);
    }

    // Add all artists to database
    for (int i = 0; i < OSSP_localMusicHandler_artistObject.size(); i++) {
        OSSP_localMusicHandler_writeArtistToDb(i);
    }

    // Add all albums to database
    for (int i = 0; i < OSSP_localMusicHandler_albumObject.size(); i++) {
        OSSP_localMusicHandler_writeAlbumToDb(i);
    }

cleanup:
    OSS_SafeFree(dbPath);
    return;
}

void OSSP_localMusicHandler_writeSongToDb(int idx) {
    static int rc = 0;
    const char* addMusicSQLQ = "INSERT INTO local_songs VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* sqlite_stmt;

    rc = sqlite3_prepare_v2(sqlite_db, addMusicSQLQ, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        //
    }

    sqlite3_bind_text(sqlite_stmt, 1, OSSP_localMusicHandler_songObject[idx].song_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 2, OSSP_localMusicHandler_songObject[idx].album_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 3, OSSP_localMusicHandler_songObject[idx].artist_uid.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(sqlite_stmt, 4, OSSP_localMusicHandler_songObject[idx].song_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 5, OSSP_localMusicHandler_songObject[idx].album_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 6, OSSP_localMusicHandler_songObject[idx].artist_name.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int64(sqlite_stmt, 7, OSSP_localMusicHandler_songObject[idx].has_inbuilt_lyrics);
    sqlite3_bind_int64(sqlite_stmt, 8, OSSP_localMusicHandler_songObject[idx].duration);
    sqlite3_bind_int64(sqlite_stmt, 9, OSSP_localMusicHandler_songObject[idx].filesize);
    sqlite3_bind_int64(sqlite_stmt, 10, OSSP_localMusicHandler_songObject[idx].album_track_number);
    sqlite3_bind_int64(sqlite_stmt, 11, OSSP_localMusicHandler_songObject[idx].total_album_tracks);
    
    sqlite3_bind_text(sqlite_stmt, 12, OSSP_localMusicHandler_songObject[idx].filepath.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(sqlite_stmt) != SQLITE_DONE) {
        printf("[LocalMusicHandler] Execution error: %s\n", sqlite3_errmsg(sqlite_db));
    }

    sqlite3_finalize(sqlite_stmt);
}

void OSSP_localMusicHandler_writeArtistToDb(int idx) {
    static int rc = 0;
    const char* addMusicSQLQ = "INSERT INTO all_artists VALUES(?, ?)";
    sqlite3_stmt* sqlite_stmt;

    rc = sqlite3_prepare_v2(sqlite_db, addMusicSQLQ, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        //
    }

    sqlite3_bind_text(sqlite_stmt, 1, OSSP_localMusicHandler_artistObject[idx].artist_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 2, OSSP_localMusicHandler_artistObject[idx].artist_name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(sqlite_stmt) != SQLITE_DONE) {
        printf("[OSSP_LocalMusicHanlder] OSSP_localMusicHandler_writeArtistToDb() execution error: %s\n", sqlite3_errmsg(sqlite_db));
    }

    sqlite3_finalize(sqlite_stmt);
}

void OSSP_localMusicHandler_writeAlbumToDb(int idx) {
    static int rc = 0;
    const char* addMusicSQLQ = "INSERT INTO all_albums VALUES(?, ?, ?, ?)";
    sqlite3_stmt* sqlite_stmt;

    rc = sqlite3_prepare_v2(sqlite_db, addMusicSQLQ, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        //
    }

    sqlite3_bind_text(sqlite_stmt, 1, OSSP_localMusicHandler_albumObject[idx].artist_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 2, OSSP_localMusicHandler_albumObject[idx].artist_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 3, OSSP_localMusicHandler_albumObject[idx].album_uid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sqlite_stmt, 4, OSSP_localMusicHandler_albumObject[idx].album_name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(sqlite_stmt) != SQLITE_DONE) {
        printf("[OSSP_LocalMusicHanlder] OSSP_localMusicHandler_writeAlbumToDb() execution error: %s\n", sqlite3_errmsg(sqlite_db));
    }

    sqlite3_finalize(sqlite_stmt);
}

/*
 * Database to App Interface
 */

/*
 * The general idea:
 * Maybe just read in what is read out for a start
 * And do custom functions from there
 *
 * Okay and if the database was refreshed this time, just have a function to check if the std::deque contains
 * some data. Its not like its going to only contain a bit, its all or nothing
 */

int OSSP_localMusicHandler_checkMemoryDb() {
    return OSSP_localMusicHandler_songObject.size();
}



void OSSP_localMusicHandler_songReq_Deconstructor(OSSP_localMusicHandler_songReq_t* obj) {
    //
}



void OSSP_localMusicHandler_scanForUniqueArtists() {
    // Scan the --- and make a list of all the unique artists, paired with the artist UIDs (of course)

    // Copy all the artists into a new std::vector
    for (int i = 0; i < OSSP_localMusicHandler_songObject.size(); i++) {
        OSSP_localMusicHandler_ArtistObject artistObj;
        artistObj.artist_uid = OSSP_localMusicHandler_songObject[i].artist_uid;
        artistObj.artist_name = OSSP_localMusicHandler_songObject[i].artist_name;
        OSSP_localMusicHandler_artistObject.push_back(artistObj);
    }

    // Sort and remove all non-unique artists (Not sure about this code tbh, having trust in stackoverflow >_<)
    std::sort(OSSP_localMusicHandler_artistObject.begin(), OSSP_localMusicHandler_artistObject.end(),
        [](const OSSP_localMusicHandler_ArtistObject& a, const OSSP_localMusicHandler_ArtistObject& b) {
        return a.artist_uid < b.artist_uid;
    });

    OSSP_localMusicHandler_artistObject.erase(std::unique(OSSP_localMusicHandler_artistObject.begin(), OSSP_localMusicHandler_artistObject.end(),
        [](const OSSP_localMusicHandler_ArtistObject& a, const OSSP_localMusicHandler_ArtistObject& b) {
        return a.artist_uid == b.artist_uid;
    }), OSSP_localMusicHandler_artistObject.end());
}







void OSSP_localMusicHandler_scanForUniqueAlbums() {
    // ---

    // Copy all the albums (and required related info) into a new std::vector
    for (int i = 0; i < OSSP_localMusicHandler_songObject.size(); i++) {
        OSSP_localMusicHandler_AlbumObject albumObj;
        albumObj.artist_uid = OSSP_localMusicHandler_songObject[i].artist_uid;
        albumObj.artist_name = OSSP_localMusicHandler_songObject[i].artist_name;
        albumObj.album_uid = OSSP_localMusicHandler_songObject[i].album_uid;
        albumObj.album_name = OSSP_localMusicHandler_songObject[i].album_name;
        OSSP_localMusicHandler_albumObject.push_back(albumObj);
    }

    // Same as the artist scanner.
    // Compare the album UID, as it is generated with both the artist and album name into a single field, very easy
    // This way, 2 artists can have the same album name, and it won't get wrongly filtered
    std::sort(OSSP_localMusicHandler_albumObject.begin(), OSSP_localMusicHandler_albumObject.end(),
        [](const OSSP_localMusicHandler_AlbumObject& a, const OSSP_localMusicHandler_AlbumObject& b) {
        return a.album_uid < b.album_uid;
    });

    OSSP_localMusicHandler_albumObject.erase(std::unique(OSSP_localMusicHandler_albumObject.begin(), OSSP_localMusicHandler_albumObject.end(),
        [](const OSSP_localMusicHandler_AlbumObject& a, const OSSP_localMusicHandler_AlbumObject& b) {
        return a.album_uid == b.album_uid;
    }), OSSP_localMusicHandler_albumObject.end());

    // Now sort the vector by artist_uid. This doesn't change anything programatically (that I am aware of, anyway),
    // it just makes the sqlite table a lot easier to read in a viewer
    // The amount of time this sort takes compared to everything else to do with the local music handling
    // is so extremely small, that I am just not worried about it
    std::sort(OSSP_localMusicHandler_albumObject.begin(), OSSP_localMusicHandler_albumObject.end(),
        [](const OSSP_localMusicHandler_AlbumObject& a, const OSSP_localMusicHandler_AlbumObject& b) {
        return a.artist_uid < b.artist_uid;
    });

    printf("[OSSP_LocalMusicHandler] Successfully scanned for albums. Found %d albums.\n", OSSP_localMusicHandler_albumObject.size());
}

/*
 * ----
 */
OSSP_localMusicHandler_songReq_t* OSSP_localMusicHandler_fetchAllDb() {
    OSSP_localMusicHandler_songReq_t* songReq = (OSSP_localMusicHandler_songReq_t*)malloc(sizeof(OSSP_localMusicHandler_songReq_t));
    songReq->songCount = OSSP_localMusicHandler_songObject.size();
    songReq->songs = (OSSP_localMusicHandler_songReq_songs_t*)malloc(songReq->songCount * sizeof(OSSP_localMusicHandler_songReq_songs_t));

    for (int i = 0; i < songReq->songCount; i++) {
        songReq->songs[i].song_uid = strdup(OSSP_localMusicHandler_songObject[i].song_uid.c_str());
        songReq->songs[i].album_uid = strdup(OSSP_localMusicHandler_songObject[i].album_uid.c_str());
        songReq->songs[i].artist_uid = strdup(OSSP_localMusicHandler_songObject[i].artist_uid.c_str());

        songReq->songs[i].song_name = strdup(OSSP_localMusicHandler_songObject[i].song_name.c_str());
        songReq->songs[i].album_name = strdup(OSSP_localMusicHandler_songObject[i].album_name.c_str());
        songReq->songs[i].artist_name = strdup(OSSP_localMusicHandler_songObject[i].artist_name.c_str());

        songReq->songs[i].filepath = strdup(OSSP_localMusicHandler_songObject[i].filepath.c_str());

        songReq->songs[i].has_inbuilt_lyrics = OSSP_localMusicHandler_songObject[i].has_inbuilt_lyrics;
        songReq->songs[i].duration = OSSP_localMusicHandler_songObject[i].duration;
        songReq->songs[i].filesize = OSSP_localMusicHandler_songObject[i].filesize;
        songReq->songs[i].album_track_number = OSSP_localMusicHandler_songObject[i].album_track_number;
        songReq->songs[i].total_album_tracks = OSSP_localMusicHandler_songObject[i].total_album_tracks;
    }

    return songReq;
}

OSSP_localMusicHandler_artistReq_t* OSSP_localMusicHandler_fetchAllArtistsFromDb() {
    OSSP_localMusicHandler_artistReq_t* artistReq = (OSSP_localMusicHandler_artistReq_t*)malloc(sizeof(OSSP_localMusicHandler_artistReq_t));
    artistReq->artist_count = OSSP_localMusicHandler_artistObject.size();
    artistReq->artists = (OSSP_localMusicHandler_artistReq_artist_t*)malloc(artistReq->artist_count * sizeof(OSSP_localMusicHandler_artistReq_artist_t));

    for (int i = 0; i < artistReq->artist_count; i++) {
        artistReq->artists[i].artist_uid = strdup(OSSP_localMusicHandler_artistObject[i].artist_uid.c_str());
        artistReq->artists[i].artist_name = strdup(OSSP_localMusicHandler_artistObject[i].artist_name.c_str());
    }

    return artistReq;
}

char* OSSP_localMusicHandler_fetchArtistNameByUid(char* artist_uid) {
    //
}





OSSP_localMusicHandler_albumReq_t* OSSP_localMusicHandler_fetchAllAlbumsFromDb() {
    //
}











OSSP_localMusicHandler_artistReq_t* OSSP_localMusicHandler_fetchAllArtists(void) {
    printf("[OSSP_LocalMusicHandler] Fetching all artists\n");

    static int rc = 0;
    sqlite3_stmt* sqlite_stmt;
    std::vector<OSSP_localMusicHandler_ArtistObject> temp_artistObj;
    OSSP_localMusicHandler_artistReq_t* artistObj = NULL;

    char* sqlite_query = "SELECT * from all_artists";
    rc = sqlite3_prepare_v2(sqlite_db, sqlite_query, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        // TODO
        return NULL;
    }

    while(sqlite3_step(sqlite_stmt) == SQLITE_ROW) {
        OSSP_localMusicHandler_ArtistObject obj;
        obj.artist_uid = std::string(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 0)));
        obj.artist_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 1)));
        temp_artistObj.push_back(obj);
    }
    sqlite3_finalize(sqlite_stmt);

    if (temp_artistObj.size() == 0) {
        printf("[OSP_LocalMusicHandler] No artists found in database.\n");
        return NULL;
    }

    artistObj = (OSSP_localMusicHandler_artistReq_t*)malloc(sizeof(OSSP_localMusicHandler_artistReq_t));
    artistObj->artist_count = temp_artistObj.size();
    artistObj->artists = (OSSP_localMusicHandler_artistReq_artist_t*)malloc(artistObj->artist_count * sizeof(OSSP_localMusicHandler_artistReq_artist_t));
    for (int i = 0; i < artistObj->artist_count; i++) {
        artistObj->artists[i].artist_uid = strdup(temp_artistObj[i].artist_uid.c_str());
        artistObj->artists[i].artist_name = strdup(temp_artistObj[i].artist_name.c_str());
    }

    printf("[OSSP_LocalMusicHandler] Found %d artists\n", artistObj->artist_count);
    return artistObj;
}

OSSP_localMusicHandler_albumReq_t* OSSP_localMusicHandler_fetchAllAlbumsByArtistUid(char* artist_uid) {
    printf("[OSSP_LocalMusicHandler] Fetching all albums from database by artist %s\n", artist_uid);
    
    static int rc = 0;
    sqlite3_stmt* sqlite_stmt;
    std::vector<OSSP_localMusicHandler_AlbumObject> temp_albumObj; // Temporary place to easily store dynamically sized amounts of heap-allocated information
    OSSP_localMusicHandler_albumReq_t* albumObj = NULL;
    
    char* sqlite_query = "SELECT * FROM all_albums WHERE artist_uid = ?";
    rc = sqlite3_prepare_v2(sqlite_db, sqlite_query, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        // TODO fix
        printf("[OSSP_LocalMusicHandler] error\n");
        return NULL;
    }

    sqlite3_bind_text(sqlite_stmt, 1, artist_uid, -1, SQLITE_TRANSIENT); // Search by artist_uid (passed via arguments)
    while (sqlite3_step(sqlite_stmt) == SQLITE_ROW) {
        OSSP_localMusicHandler_AlbumObject obj;
        // TODO Why are the stmt indices 0-index'd instead of 1-index'd here? (Already tested 1-index'd, doesn't work)
        // As for this datatype conversion, information from https://stackoverflow.com/a/804131
        // Also, sqlite3_column_text is not heap allocated, gets destroyed with either sqlite3_step() or sqlite3_finalize()
        obj.artist_uid = std::string(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 0)));
        obj.artist_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 1)));
        obj.album_uid = std::string(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 2)));
        obj.album_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(sqlite_stmt, 3)));
        temp_albumObj.push_back(obj);
    }
    sqlite3_finalize(sqlite_stmt);

    if (temp_albumObj.size() == 0) {
        printf("[OSSP_LocalMusicHandler] No album found in database by artist %s\n", artist_uid);
        return NULL;
    }

    // Move data to a C-compatible object
    albumObj = (OSSP_localMusicHandler_albumReq_t*)malloc(sizeof(OSSP_localMusicHandler_albumReq_t));
    albumObj->artist_name = strdup(temp_albumObj[0].artist_name.c_str()); // Artist names are same across all indices
    albumObj->album_count = temp_albumObj.size();
    albumObj->albums = (OSSP_localMusicHandler_albumReq_albums_t*)malloc(albumObj->album_count * sizeof(OSSP_localMusicHandler_albumReq_albums_t));
    for (int i = 0; i < albumObj->album_count; i++) {
        albumObj->albums[i].album_uid = strdup(temp_albumObj[i].album_uid.c_str());
        albumObj->albums[i].album_name = strdup(temp_albumObj[i].album_name.c_str());
    }
    
    printf("[OSSP_LocalMusicHandler] Found %d albums by artist %s (%s)\n", albumObj->album_count, albumObj->artist_name, artist_uid);
    return albumObj;
}









void OSSP_localMusicHandler_readFullDb() {
    static int rc = 0;
    char* dbPath = NULL;

#if defined(__ANDROID__)
    rc = asprintf(&dbPath, "/sdcard/OSSP/localmusic.db");
#endif
    if (rc == -1) {
        printf("[OSSP_LocalMusicHandler] asprintf() failed.\n");
        rc = 0;
        //goto cleanup;
    }

    rc = sqlite3_open(dbPath, &sqlite_db);
    if (rc) {
        printf("[OSSP_LocalMusicHandler] Could not create database: %s\n", sqlite3_errmsg(sqlite_db));
        //goto cleanup;
    }
}
