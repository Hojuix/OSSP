/*
 * OpenSubSonicPlayer
 * Goldenkrew3000 2026
 * License: GNU General Public License 3.0
 * LastFM Scrobbler
 */

#ifndef _SCROBBLER_LASTFM_H
#define _SCROBBLER_LASTFM_H

typedef struct {
    int finalize;       // 0 -> In progress, 1 -> Finalize
    char* songTitle;
    char* songAlbum;
    char* songArtist;
} scrobbler_data;

int opensubsonic_scrobble_lastFm(scrobbler_data* scrobblerData);
void opensubsonic_scrobble_init(scrobbler_data* scrobblerData);
void opensubsonic_scrobble_free(scrobbler_data* scrobblerData);

// TODO fix this fuckass naming scheme

#endif
