/*
 * OpenSubsonicPlayer (OSSP)
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Local Music Database Reader
 */

// TODO Clean up this include mess
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include "../libopensubsonic/utils.h"
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/dict.h>
    #include <libavformat/avio.h>
    #include "../external/sqlite3/sqlite3.h"
    #include "../external/md5.h"
    #include "../libopensubsonic/utils.h"
}
#include "../configHandler.h"
#include "general.hpp"
#include "reader.hpp"

#include "../libopensubsonic/utils.h"

// Android System Log Hook
#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "OSSP", __VA_ARGS__)
#endif

extern OSSP_config_t* configObj;
static sqlite3* sqlite_db = NULL;
static char* sqlite_errorMsg = NULL;

/*
 * SQLite Database File Pointer Functions
 */
/*
 * Open Local Music Database, Return 0 if error, else return TODO
 */
int OSSP_LocalMusic_Reader_OpenDatabase() {
    static int rc = 0;
    char* dbPath = NULL;

// TODO Same thing here, got to figure out how to centralize these paths
#if defined(__ANDROID__)
    rc = asprintf(&dbPath, "/sdcard/OSSP/localmusic.db");
#else
    // Assume UNIX platform (*BSD/Linux/macOS).
    rc = asprintf(&dbPath, "%s/.config/ossp/localmusic.db", getenv("HOME"));
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
    } else {
        printf("OPENED DB!!!\n");
    }

    return 1;
}

/*
 * Close the Local Music Database file pointer. Returns 0 on success, else returns 1
 */
int OSSP_LocalMusic_Reader_CloseDatabase() {
    static int rc = 0;

    if (sqlite_db == NULL) {
        printf("[OSSP_LocalMusic] Database was already closed.\n");
        return 0;
    } else {
        rc = sqlite3_close(sqlite_db);
        if (rc != SQLITE_OK) {
            printf("[OSSP_LocalMusic] Database could not be closed, sqlite3_close() did not return SQLITE_OK.\n");
            return 1;
        } else {
            sqlite_db = NULL;
            printf("[OSSP_LocalMusic] Database closed.\n");
            return 0;
        }
    }
}

/*
 * Check if the Local Music Database is currently open. Returns 0 if it is, else -1
 */
int OSSP_LocalMusic_Reader_CheckIfDatabaseOpen() {
    // Stage 1 - Check if the database object is NULL
    if (sqlite_db == NULL) {
        return -1;
    }

    return 0;

    // TODO (Eventually) Stage 2 - Perform test action on database: https://sqlite.org/c3ref/db_status.html
}

/*
 * Query Count Functions
 * All of these functions return -1 on error
 */

// Query how many entries there are in the 'all_artists' table
int OSSP_LocalMusic_Reader_GetArtistCount() {
    static int rc = 0;
    int artist_entries = -1;
    const char* querySQLQ = "SELECT COUNT(*) AS total_entries FROM all_artists;";
    sqlite3_stmt* sqlite_stmt;

    rc = OSSP_LocalMusic_Reader_CheckIfDatabaseOpen();
    if (rc < 0) {
        printf("[OSSP_LocalMusic] OSSP_LocalMusic_Reader_GetArtistCount() - Database is not open.\n");
        return -1;
    }

    rc = sqlite3_prepare_v2(sqlite_db, querySQLQ, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        // TODO show db error
        return -1;
    }

    rc = sqlite3_step(sqlite_stmt);
    if (rc == SQLITE_ROW) {
        artist_entries = sqlite3_column_int(sqlite_stmt, 0);
    } else {
        // TODO Show db error
        return -1;
    }

    sqlite3_finalize(sqlite_stmt);
    return artist_entries;
}

int OSSP_LocalMusic_Reader_GetAlbumCount() {
    //
}

int OSSP_LocalMusic_Reader_GetSongCount() {
    //
}

/*
 * IDK TODO
 */
void OSSP_LocalMusic_Reader_ReadEntireDb() {
    //
}



OSSP_LocalMusic_Reader_Query_t* OSSP_LocalMusic_Reader_GetAllArtists() {
    static int rc = 0;

    OSSP_LocalMusic_Reader_Query_t* ret = (OSSP_LocalMusic_Reader_Query_t*)malloc(sizeof(OSSP_LocalMusic_Reader_Query_t));
    if (ret == NULL) {
        printf("[OSSP_LocalMusic] failed allocation.\n");
        return NULL;
    }

    // Query how many artists are in the database
    int artist_count = OSSP_LocalMusic_Reader_GetArtistCount();
    if (artist_count <= 0) {
        printf("[OSSP_LocalMusic] Artist count of %d returned from OSSP_LocalMusic_Reader_GetArtistCount().\n", artist_count);
        ret->count = 0;
        ret->GetAllArtists_artists = NULL;
        return ret; // TODO make a constructor for this future massive data structure
    }

    ret->count = artist_count;
    ret->GetAllArtists_artists = (OSSP_LocalMusic_Reader_GetAllArtists_t*)malloc(artist_count * sizeof(OSSP_LocalMusic_Reader_GetAllArtists_t));
    if (ret->GetAllArtists_artists == NULL) {
        printf("[OSSP_LocalMusic] failed allocation.\n");
        ret->count = 0;
        return ret;
    }

    sqlite3_stmt* sqlite_stmt;
    const char* SQLQ = "SELECT * FROM all_artists;";

    rc = sqlite3_prepare_v2(sqlite_db, SQLQ, -1, &sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("[OSSP_LocalMusic] Could not read from local music all_artists table: %s\n", sqlite3_errmsg(sqlite_db));
        ret->count = 0;
        free(ret->GetAllArtists_artists);
        ret->GetAllArtists_artists = NULL; // TODO SafeFree?
        return ret;
    }

    int idx = 0;
    while (sqlite3_step(sqlite_stmt) == SQLITE_ROW) {
        if (idx <= ret->count) {
            ret->GetAllArtists_artists[idx].uid = OSSP_SafeStrdup((const char*)sqlite3_column_text(sqlite_stmt, 0));
            ret->GetAllArtists_artists[idx].name = OSSP_SafeStrdup((const char*)sqlite3_column_text(sqlite_stmt, 1));
        }

        idx++;
    }
    sqlite3_finalize(sqlite_stmt);

    return ret;
}

void OSSP_LocalMusic_Reader_Deconstructor(OSSP_LocalMusic_Reader_Query_t** obj) {
    // TODO
}
