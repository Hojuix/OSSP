#ifndef _LOCALMUSIC_READER_HPP
#define _LOCALMUSIC_READER_HPP

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    char* uid;
    char* name;
} OSSP_LocalMusic_Reader_GetAllArtists_t;

typedef struct {
    int count;
    OSSP_LocalMusic_Reader_GetAllArtists_t* GetAllArtists_artists;
} OSSP_LocalMusic_Reader_Query_t;

int OSSP_LocalMusic_Reader_OpenDatabase();
int OSSP_LocalMusic_Reader_CloseDatabase();
int OSSP_LocalMusic_Reader_CheckIfDatabaseOpen();

int OSSP_LocalMusic_Reader_GetArtistCount();

OSSP_LocalMusic_Reader_Query_t* OSSP_LocalMusic_Reader_GetAllArtists();

void OSSP_LocalMusic_Reader_Deconstructor(OSSP_LocalMusic_Reader_Query_t** obj);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
