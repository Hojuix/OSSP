/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 2025
 * License: GNU General Public License 3.0
 */

#ifndef _DISCORDRPC_H
#define _DISCORDRPC_H
#include <time.h>

#define DISCORDRPC_STATE_IDLE 0
#define DISCORDRPC_STATE_PLAYING_OPENSUBSONIC 1
#define DISCORDRPC_STATE_PLAYING_LOCALFILE 2
#define DISCORDRPC_STATE_PLAYING_INTERNETRADIO 3
#define DISCORDRPC_STATE_PAUSED 4

typedef struct {
    int state;
    long songLength;
    time_t startTime;
    char* songTitle;
    char* songArtist;
    char* coverArtUrl;
} OSSP_discordrpc_t;

OSSP_discordrpc_t* OSSP_discordrpc_Constructor();
void OSSP_discordrpc_Deconstructor(OSSP_discordrpc_t* obj);
int OSSP_discordrpc_Init();
void OSSP_discordrpc_update(OSSP_discordrpc_t* obj);
char* OSSP_discordrpc_getOS();

#endif
