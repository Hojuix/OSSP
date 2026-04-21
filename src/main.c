/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 2025
 * License: GNU General Public License 3.0
 * Info: OSSP Entry Point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "libopensubsonic/crypto.h"
#include "libopensubsonic/httpclient.h"
#include "libopensubsonic/endpoint_ping.h"
#include "configHandler.h"
//#include "player/player.h"
#include "discordrpc.h"

//#include "localRadioDBHandler.h"
//#include "libopensubsonic/endpoint_getInternetRadioStations.h"
#include "libopensubsonic/httpclient.h"
//#include "socket/socket.h"
//#include "localMusicHandler.hpp"

static int rc = 0;
configHandler_config_t* configObj = NULL;
int checkConfigFile();
int validateConnection();

int main(int argc, char** argv) {
    // Read config file
    rc = configHandler_Read(&configObj);
    if (rc != 0) {
        printf("Could not read config file!\n");
        return 1;
    }

    // Run check on the config file
    rc = checkConfigFile();
    if (rc != 0) {
        return 1;
    }

    // Generate opensubsonic login
    opensubsonic_crypto_generateLogin();

    // Check the config for the newly generated crypto salt/token
    if (configObj->internal_opensubsonic_loginSalt == NULL ||
        configObj->internal_opensubsonic_loginToken == NULL) {
        printf("Internal Failure: Could not generate OpenSubsonic Login Salt/Token.\n");
        return 1;
    }

    // Attempt connection
    rc = validateConnection();
    if (rc != 0) {
        return 1;
    }

    // Connection attempt was successful, initialize player
    /*
    pthread_t pthr_player;
    pthread_create(&pthr_player, NULL, OSSPlayer_ThrdInit, NULL);
*/

    // Launch Discord RPC
    OSSP_discordrpc_t* discordrpc = OSSP_discordrpc_Constructor();
    if (discordrpc == NULL) {
        printf("malloc() failed.\n");
        return 1;
    }
    discordrpc->state = DISCORDRPC_STATE_IDLE;
    rc = OSSP_discordrpc_Init();
    if (rc != 0) {
        printf("[OSSP] Discord RPC failed to initialize.\n");
        return 1;
    }
    OSSP_discordrpc_update(discordrpc);
    OSSP_discordrpc_Deconstructor(discordrpc);

    //localMusicHandler_scan();
/*
    // Launch socket server
    socketHandler_init();
*/
    // Cleanup and exit
    configHandler_Free(&configObj);
    
    return 0;
}

int checkConfigFile() {
    // Check for OpenSubsonic username and password
    if (strlen(configObj->opensubsonic_username) == 0 ||
        strlen(configObj->opensubsonic_password) == 0) {
        printf("OpenSubsonic username/password is empty.\n");
        return 1;
    }

    // Check for OpenSubsonic protocol and server
    if (strlen(configObj->opensubsonic_protocol) == 0 ||
        strlen(configObj->opensubsonic_server) == 0) {
        printf("OpenSubsonic protocol/server is empty.\n");
        return 1;
    }

    // Check for ListenBrainz scrobble config
    if (configObj->listenbrainz_enable) {
        if (strlen(configObj->listenbrainz_token) == 0) {
            printf("ListenBrainz scrobbling is enabled, but token is empty.\n");
            return 1;
        }
    }

    // Check for LastFM scrobble config
    if (configObj->lastfm_enable) {
        if (strlen(configObj->lastfm_username) == 0 ||
            strlen(configObj->lastfm_password) == 0) {
            printf("LastFM scrobbling is enabled, but username/password is empty.\n");
            return 1;
        }
        if (strlen(configObj->lastfm_api_key) == 0 ||
            strlen(configObj->lastfm_api_secret) == 0) {
            printf("LastFM scrobbling is enabled, but API key/secret is empty.\n");
            return 1;
        }
        if (strlen(configObj->lastfm_api_session_key) == 0) {
            printf("LastFM scrobbling is enabled, but API session key is empty.\n");
            return 1;
        }
    }

    return 0;
}

int validateConnection() {
    static int rc = 0;
    printf("Attempting to connect to /ping at %s://%s...\n", configObj->opensubsonic_protocol, configObj->opensubsonic_server);
    
    OSSP_httpCli_UrlObj_t* pingUrlObj = OSSP_httpCli_UrlObj_Constructor();
    if (pingUrlObj == NULL) {
        printf("malloc() failed.\n");
        return 1;
    }
    pingUrlObj->endpoint = OPENSUBSONIC_ENDPOINT_PING;
    rc = OSSP_httpCli_createURL(pingUrlObj);
    if (rc != 0) {
        return 1;
    }
    
    rc = OSSP_httpCli_sendReq(pingUrlObj);
    OSSP_endpoint_ping_t* pingStr = (OSSP_endpoint_ping_t*)pingUrlObj->returnStruct;
    if (rc == 1) {
        if (pingUrlObj->returnStruct != NULL) {
            OSSP_endpoint_ping_Deconstructor(pingStr);
        }
        OSSP_httpCli_UrlObj_Deconstructor(pingUrlObj);
        return 1;
    }
    OSSP_httpCli_UrlObj_Deconstructor(pingUrlObj);

    if (!pingStr->error) {
        printf("Connection to %s://%s successful.\n", configObj->opensubsonic_protocol, configObj->opensubsonic_server);
        printf("Server: %s %s.\n", pingStr->serverType, pingStr->serverVersion);
        OSSP_endpoint_ping_Deconstructor(pingStr);
        return 0;
    } else {
        printf("Connection to %s://%s failed:\n", configObj->opensubsonic_protocol, configObj->opensubsonic_server);
        printf("Code %d - %s\n", pingStr->errorCode, pingStr->errorMessage);
        OSSP_endpoint_ping_Deconstructor(pingStr);
        return 1;
    }
}
