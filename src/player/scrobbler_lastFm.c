/*
 * OpenSubSonicPlayer
 * Goldenkrew3000 2026
 * License: GNU General Public License 3.0
 * LastFM Scrobbler
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../libopensubsonic/httpclient.h"
#include "../external/cJSON.h"
#include "../external/md5.h"
#include "../external/libcurl_uriescape.h"
#include "../configHandler.h"
#include "scrobbler_lastFm.h"

// Temp - move away from that fuckahh logger
#include "../libopensubsonic/logger.h"

const char* lastFmScrobbleURL = "https://ws.audioscrobbler.com/2.0/";
static int rc = 0;
extern configHandler_config_t* configObj;

int opensubsonic_scrobble_lastFm(scrobbler_data* scrobblerData) {
    if (scrobblerData->finalize) {
        logger_log_general(__func__, "Performing final scrobble to LastFM.");
    } else {
        logger_log_general(__func__, "Performing in-progress scrobble to LastFM.");
    }
    
    // Fetch the current UNIX timestamp
    time_t currentTime;
    char* currentTime_string;
    currentTime = time(NULL);
    rc = asprintf(&currentTime_string, "%ld", currentTime);
    if (rc == -1) {
        logger_log_error(__func__, "asprintf() failed (Could not make char* of UNIX timestamp).");
        return 1;
    }
    
    // Assemble the signature
    char* sig_plaintext = NULL;
    if (scrobblerData->finalize) {
        rc = asprintf(&sig_plaintext, "album%salbumArtist%sapi_key%sartist%smethodtrack.scrobblesk%stimestamp%strack%s%s",
                      scrobblerData->songAlbum, scrobblerData->songArtist, configObj->lastfm_api_key, scrobblerData->songArtist,
                      configObj->lastfm_api_session_key, currentTime_string, scrobblerData->songTitle, configObj->lastfm_api_secret);
    } else {
        rc = asprintf(&sig_plaintext, "album%salbumArtist%sapi_key%sartist%smethodtrack.updateNowPlayingsk%stimestamp%strack%s%s",
                      scrobblerData->songAlbum, scrobblerData->songArtist, configObj->lastfm_api_key, scrobblerData->songArtist,
                      configObj->lastfm_api_session_key, currentTime_string, scrobblerData->songTitle, configObj->lastfm_api_secret);
    }
    if (rc == -1) {
        logger_log_error(__func__, "asprintf() failed (Could not assemble plaintext signature).");
        return 1;
    }
    
    uint8_t sig_md5_bytes[16];
    char* sig_md5_text = NULL; // TODO do I have to free this? Also is be used in crypto.c
    md5String(sig_plaintext, sig_md5_bytes);
    free(sig_plaintext);
    rc = asprintf(&sig_md5_text, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                  sig_md5_bytes[0], sig_md5_bytes[1], sig_md5_bytes[2], sig_md5_bytes[3],
                  sig_md5_bytes[4], sig_md5_bytes[5], sig_md5_bytes[6], sig_md5_bytes[7],
                  sig_md5_bytes[8], sig_md5_bytes[9], sig_md5_bytes[10], sig_md5_bytes[11],
                  sig_md5_bytes[12], sig_md5_bytes[13], sig_md5_bytes[14], sig_md5_bytes[15]);
    if (rc == -1) {
        logger_log_error(__func__, "asprintf() failed (Could not assemble md5 signature).");
        return 1;
    }
    
    // URI encode strings
    char* uri_songTitle = lcue_uriescape(scrobblerData->songTitle, (unsigned int)strlen(scrobblerData->songTitle));
    char* uri_songArtist = lcue_uriescape(scrobblerData->songArtist, (unsigned int)strlen(scrobblerData->songArtist));
    char* uri_songAlbum = lcue_uriescape(scrobblerData->songAlbum, (unsigned int)strlen(scrobblerData->songAlbum));
    if (uri_songTitle == NULL || uri_songArtist == NULL || uri_songAlbum == NULL) {
        logger_log_error(__func__, "lcue_uriescape() error (Could not URI escape required strings).");
        free(currentTime_string);
        free(sig_md5_text);
        if (uri_songTitle != NULL) { free(uri_songTitle); }
        if (uri_songArtist != NULL) { free(uri_songArtist); }
        if (uri_songAlbum != NULL) { free(uri_songAlbum); }
        return 1;
    }
    
    // Assemble the payload
    char* payload = NULL;
    if (scrobblerData->finalize) {
        rc = asprintf(&payload,
                      "%s?method=track.scrobble&api_key=%s&timestamp=%s&track=%s&artist=%s&album=%s&albumArtist=%s&sk=%s&api_sig=%s&format=json",
                      lastFmScrobbleURL, configObj->lastfm_api_key, currentTime_string, uri_songTitle,
                      uri_songArtist, uri_songAlbum, uri_songArtist,
                      configObj->lastfm_api_session_key, sig_md5_text);
    } else {
        rc = asprintf(&payload,
                      "%s?method=track.updateNowPlaying&api_key=%s&timestamp=%s&track=%s&artist=%s&album=%s&albumArtist=%s&sk=%s&api_sig=%s&format=json",
                      lastFmScrobbleURL, configObj->lastfm_api_key, currentTime_string, uri_songTitle,
                      uri_songArtist, uri_songAlbum, uri_songArtist,
                      configObj->lastfm_api_session_key, sig_md5_text);
    }
    free(currentTime_string);
    free(sig_md5_text);
    free(uri_songTitle);
    free(uri_songAlbum);
    free(uri_songArtist);
    if (rc == -1) {
        logger_log_error(__func__, "asprintf() failed (Could not assemble payload).");
        return 1;
    }
    
    // Send scrobble and receive response
    opensubsonic_httpClientRequest_t* httpReq;
    opensubsonic_httpClient_prepareRequest(&httpReq);
    
    httpReq->requestUrl = strdup(payload);
    free(payload);
    httpReq->scrobbler = SCROBBLER_LASTFM;
    httpReq->method = HTTP_METHOD_POST;
    opensubsonic_httpClient_request(&httpReq);
    
    if (httpReq->responseCode != HTTP_CODE_SUCCESS) {
        logger_log_error(__func__, "HTTP POST did not return success (%d).", httpReq->responseCode);
        opensubsonic_httpClient_cleanup(&httpReq);
        // TODO return error
    }
    
    // Parse the scrobble response
    cJSON* root = cJSON_Parse(httpReq->responseMsg);
    opensubsonic_httpClient_cleanup(&httpReq);
    if (root == NULL) {
        logger_log_error(__func__, "Error parsing JSON.");
        // TODO return error
    }
    
    cJSON* inner_root = NULL;
    cJSON* scrobbles_root = NULL; // Parent of inner_root, only used on final scrobble
    if (scrobblerData->finalize) {
        // Make an object from scrobbles
        scrobbles_root = cJSON_GetObjectItemCaseSensitive(root, "scrobbles");
        if (scrobbles_root == NULL) {
            logger_log_error(__func__, "Error parsing JSON - scrobbles does not exist.");
            cJSON_Delete(root);
            // TODO return error
        }
        
        // Make an object from scrobble
        inner_root = cJSON_GetObjectItemCaseSensitive(scrobbles_root, "scrobble");
        if (inner_root == NULL) {
            logger_log_error(__func__, "Error parsing JSON - scrobble does not exist.");
            cJSON_Delete(root);
            // TODO return error
        }
    } else {
        // Make an object from nowplaying
        inner_root = cJSON_GetObjectItemCaseSensitive(root, "nowplaying");
        if (inner_root == NULL) {
            logger_log_error(__func__, "Error parsing JSON - nowplaying does not exist.");
            cJSON_Delete(root);
            // TODO return error
        }
    }
    
    // Make an object from artist, track, albumArtist, and album, and fetch codes
    cJSON* artist_root = cJSON_GetObjectItemCaseSensitive(inner_root, "artist");
    if (artist_root == NULL) {
        logger_log_error(__func__, "Error parsing JSON - artist does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* artist_corrected = cJSON_GetObjectItemCaseSensitive(artist_root, "corrected");
    if (cJSON_IsString(artist_corrected) && artist_corrected->valuestring != NULL) {
        if (*(artist_corrected->valuestring) == '\0') {
            logger_log_error(__func__, "Error parsing JSON - artist/corrected is empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        char* endptr;
        int corrected = (int)strtol(artist_corrected->valuestring, &endptr, 10);
        if (*endptr != '\0') {
            logger_log_error(__func__, "Error parsing JSON - artist/corrected strtol/endptr is not empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        if (corrected == 1) {
            logger_log_important(__func__, "Warning - Artist has been autocorrected.");
        }
    } else {
        logger_log_error(__func__, "Error parsing JSON - artist/corrected does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* track_root = cJSON_GetObjectItemCaseSensitive(inner_root, "track");
    if (track_root == NULL) {
        logger_log_error(__func__, "Error parsing JSON - track does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* track_corrected = cJSON_GetObjectItemCaseSensitive(track_root, "corrected");
    if (cJSON_IsString(track_corrected) && track_corrected->valuestring != NULL) {
        if (*(track_corrected->valuestring) == '\0') {
            logger_log_error(__func__, "Error parsing JSON - track/corrected is empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        char* endptr;
        int corrected = (int)strtol(track_corrected->valuestring, &endptr, 10);
        if (*endptr != '\0') {
            logger_log_error(__func__, "Error parsing JSON - track/corrected strtol/endptr is not empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        if (corrected == 1) {
            logger_log_important(__func__, "Warning - Track has been autocorrected.");
        }
    } else {
        logger_log_error(__func__, "Error parsing JSON - track/corrected does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* albumArtist_root = cJSON_GetObjectItemCaseSensitive(inner_root, "albumArtist");
    if (albumArtist_root == NULL) {
        logger_log_error(__func__, "Error parsing JSON - albumArtist does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* albumArtist_corrected = cJSON_GetObjectItemCaseSensitive(albumArtist_root, "corrected");
    if (cJSON_IsString(albumArtist_corrected) && albumArtist_corrected->valuestring != NULL) {
        if (*(albumArtist_corrected->valuestring) == '\0') {
            logger_log_error(__func__, "Error parsing JSON - albumArtist/corrected is empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        char* endptr;
        int corrected = (int)strtol(albumArtist_corrected->valuestring, &endptr, 10);
        if (*endptr != '\0') {
            logger_log_error(__func__, "Error parsing JSON - albumArtist/corrected strtol/endptr is not empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        if (corrected == 1) {
            logger_log_important(__func__, "Warning - Album Artist has been autocorrected.");
        }
    } else {
        logger_log_error(__func__, "Error parsing JSON - albumArtist/corrected does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* album_root = cJSON_GetObjectItemCaseSensitive(inner_root, "album");
    if (album_root == NULL) {
        logger_log_error(__func__, "Error parsing JSON - album does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* album_corrected = cJSON_GetObjectItemCaseSensitive(album_root, "corrected");
    if (cJSON_IsString(album_corrected) && album_corrected->valuestring != NULL) {
        if (*(album_corrected->valuestring) == '\0') {
            logger_log_error(__func__, "Error parsing JSON - album/corrected is empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        char* endptr;
        int corrected = (int)strtol(album_corrected->valuestring, &endptr, 10);
        if (*endptr != '\0') {
            logger_log_error(__func__, "Error parsing JSON - album/corrected strtol/endptr is not empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        if (corrected == 1) {
            logger_log_important(__func__, "Warning - Album has been autocorrected.");
        }
    } else {
        logger_log_error(__func__, "Error parsing JSON - album/corrected does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    // Make an object from ignoredMessage, and check return code
    cJSON* ignoredMessage_root = cJSON_GetObjectItemCaseSensitive(inner_root, "ignoredMessage");
    if (ignoredMessage_root == NULL) {
        logger_log_error(__func__, "Error parsing JSON - ignoredMessage does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }
    
    cJSON* ignoredMessage_code = cJSON_GetObjectItemCaseSensitive(ignoredMessage_root, "code");
    if (cJSON_IsString(ignoredMessage_code) && ignoredMessage_code->valuestring != NULL) {
        if (*(ignoredMessage_code->valuestring) == '\0') {
            logger_log_error(__func__, "Error parsing JSON - ignoredMessage/code is empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        char* endptr;
        int code = (int)strtol(ignoredMessage_code->valuestring, &endptr, 10);
        if (*endptr != '\0') {
            logger_log_error(__func__, "Error parsing JSON - ignoredMessage/code strtol/endptr is not empty.");
            cJSON_Delete(root);
            // TODO return error
        }
        if (code == 0) {
            if (!scrobblerData->finalize) {
                logger_log_general(__func__, "In progress scrobble was successful.");
            } else {
                logger_log_general(__func__, "Final scrobble 1/2 was successful.");
            }
        } else if (code == 1) {
            logger_log_error(__func__, "Artist was ignored.");
        } else if (code == 2) {
            logger_log_error(__func__, "Track was ignored.");
        } else if (code == 3) {
            logger_log_error(__func__, "Timestamp was too old.");
        } else if (code == 4) {
            logger_log_error(__func__, "Timestamp was too new.");
        } else if (code == 5) {
            logger_log_error(__func__, "Daily scrobble limit exceeded.");
        } else {
            logger_log_error(__func__, "Unknown error code received (%d)", code);
        }
    } else {
        logger_log_error(__func__, "Error parsing JSON - ignoredMessage/code does not exist.");
        cJSON_Delete(root);
        // TODO return error
    }

    if (scrobblerData->finalize) {
        cJSON* attr_root = cJSON_GetObjectItemCaseSensitive(scrobbles_root, "@attr");
        if (attr_root == NULL) {
            logger_log_error(__func__, "Error parsing JSON - @attr does not exist.");
            cJSON_Delete(root);
            // TODO return error
        }
        
        cJSON* attr_ignored = cJSON_GetObjectItemCaseSensitive(attr_root, "ignored");
        if (cJSON_IsNumber(attr_ignored)) {
            if (attr_ignored->valueint != 0) {
                logger_log_important(__func__, "Warning - @attr/ignored is not 0 (%d).", attr_ignored->valueint);
            }
        } else {
            logger_log_error(__func__, "Error parsing JSON - @attr/ignored does not exist.");
            cJSON_Delete(root);
            // TODO return error
        }
        
        cJSON* attr_accepted = cJSON_GetObjectItemCaseSensitive(attr_root, "accepted");
        if (cJSON_IsNumber(attr_accepted)) {
            if (attr_accepted->valueint != 1) {
                logger_log_important(__func__, "Warning - @attr/accepted is not 1 (%d).", attr_accepted->valueint);
            }
        } else {
            logger_log_error(__func__, "Error parsing JSON - @attr/accepted does not exist");
            cJSON_Delete(root);
            // TODO return error
        }
        
        // At this point, attr_ignored and attr_accepted are both known to be valid
        if (attr_ignored->valueint == 0 && attr_accepted->valueint == 1) {
            logger_log_general(__func__, "Final scrobble 2/2 was successful.");
        } else {
            logger_log_important(__func__, "Final scobble 2/2 was not successful (ignored: %d, accepted: %d",
                                 attr_ignored->valueint, attr_accepted->valueint);
        }
    }
    
    cJSON_Delete(root);
}

void opensubsonic_scrobble_init(scrobbler_data* scrobblerData) {
    scrobblerData->finalize = 0;
    scrobblerData->songTitle = NULL;
    scrobblerData->songAlbum = NULL;
    scrobblerData->songArtist = NULL;
}

void opensubsonic_scrobble_free(scrobbler_data* scrobblerData) {
    if (scrobblerData->songTitle != NULL) { free(scrobblerData->songTitle); }
    if (scrobblerData->songAlbum != NULL) { free(scrobblerData->songAlbum); }
    if (scrobblerData->songArtist != NULL) { free(scrobblerData->songArtist); }
}
