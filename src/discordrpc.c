/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Discord RPC Handler
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "external/discord-rpc/include/discord_rpc.h"
#include "configHandler.h"
#include "discordrpc.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/sysctl.h>
#endif // defined(__APPLE__) && defined(__MACH__)

extern configHandler_config_t* configObj;
const char* discordrpc_appid = "1407025303779278980";
char* discordrpc_osString = NULL;
static int rc = 0;

OSSP_discordrpc_t* OSSP_discordrpc_Constructor() {
    printf("Running discordrpc Constructor.\n");
    OSSP_discordrpc_t* obj = malloc(sizeof(OSSP_discordrpc_t));
    if (obj == NULL) {
        return NULL;
    }
    obj->state = 0;
    obj->songLength = 0;
    obj->startTime = 0;
    obj->songTitle = NULL;
    obj->songArtist = NULL;
    obj->coverArtUrl = NULL;
    return obj;
}

void OSSP_discordrpc_Deconstructor(OSSP_discordrpc_t* obj) {
    printf("Running discordrpc Deconstructor.\n");
    if (obj->songTitle != NULL) { free(obj->songTitle); }
    if (obj->songArtist != NULL) { free(obj->songArtist); }
    if (obj->coverArtUrl != NULL) { free(obj->coverArtUrl); }
    if (obj != NULL) { free(obj); }
}

int OSSP_discordrpc_Init() {
    printf("[DiscordRPC] Initializing.\n");
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(discordrpc_appid, &handlers, 1, NULL);

    // Fetch OS String for RPC (Heap-allocated)
    discordrpc_osString = OSSP_discordrpc_getOS();
    if (discordrpc_osString == NULL) {
        printf("[DiscordRPC] (%s) asprintf() or strdup() failed.\n", __func__);
        return 1;
    }
    return 0;
}

void OSSP_discordrpc_update(OSSP_discordrpc_t* obj) {
    printf("[DiscordRPC] Updating...\n");
    DiscordRichPresence presence;
    char* detailsString = NULL;
    char* stateString = NULL;
    memset(&presence, 0, sizeof(presence));

    if (obj->state == DISCORDRPC_STATE_IDLE) {
        printf("[DiscordRPC] Issuing Idle RPC.\n");
        asprintf(&detailsString, "Idle");
        presence.details = detailsString;
    } else if (obj->state == DISCORDRPC_STATE_PLAYING_OPENSUBSONIC ||
           (obj->state == DISCORDRPC_STATE_PLAYING_LOCALFILE)) {
        // Playing a song from an OpenSubsonic server
        printf("[DiscordRPC] Issuing OpenSubsonic/Local File Song RPC.\n");
        asprintf(&detailsString, "%s", obj->songTitle);
        asprintf(&stateString, "by %s", obj->songArtist);
        presence.details = detailsString;
        presence.state = stateString;
        if (obj->state == DISCORDRPC_STATE_PLAYING_OPENSUBSONIC) {
            // TODO As of now, local file playback does NOT deal with cover art
            presence.largeImageKey = obj->coverArtUrl;
        }
        presence.startTimestamp = (long)(obj->startTime);
        presence.endTimestamp = (long)(obj->startTime) + obj->songLength;
        if (configObj->discordrpc_showSysDetails) {
            presence.largeImageText = discordrpc_osString;
        }
    } else if (obj->state == DISCORDRPC_STATE_PLAYING_INTERNETRADIO) {
        // Playing an internet radio station
        printf("[DiscordRPC] Issuing Internet Radio RPC.\n");
        asprintf(&detailsString, "%s", obj->songTitle);
        asprintf(&stateString, "Internet radio station");
        presence.details = detailsString;
        presence.state = stateString;
        presence.largeImageKey = obj->coverArtUrl;
        presence.startTimestamp = (long)(obj->startTime);
        if (configObj->discordrpc_showSysDetails) {
            presence.largeImageText = discordrpc_osString;
        }
    } else if (obj->state == DISCORDRPC_STATE_PAUSED) {
        // Player is paused
        printf("[DiscordRPC] Issuing Paused RPC.\n");
        asprintf(&detailsString, "Paused");
        presence.details = detailsString;
    }

    presence.activity_type = DISCORD_ACTIVITY_TYPE_LISTENING;
    Discord_UpdatePresence(&presence);

    free(detailsString);
    if (stateString != NULL) { free(stateString); }
}

char* OSSP_discordrpc_getOS() {
#if defined(__linux__)
    // NOTE: Could have made a sysctl function, but this is literally only done here, not worth it
    FILE* fp_ostype = fopen("/proc/sys/kernel/ostype", "r");
    char buf_ostype[16];
    if (!fp_ostype) {
        printf("[DiscordRPC] (%s) Could not perform kernel.ostype sysctl.\n", __func__);
        return NULL;
    }

    FILE* fp_osrelease = fopen("/proc/sys/kernel/osrelease", "r");
    char buf_osrelease[32];
    if (!fp_osrelease) {
        printf("[DiscordRPC] (%s) Could not perform kernel.osrelease sysctl.\n", __func__);
        return NULL;
    }

    FILE* fp_osarch = fopen("/proc/sys/kernel/arch", "r");
    char buf_osarch[16];
    if (!fp_osarch) {
        printf("[DiscordRPC] (%s) Could not perform kernel.arch sysctl.\n", __func__);
        return NULL;
    }

    if (fgets(buf_ostype, sizeof(buf_ostype), fp_ostype) == NULL) {
        printf("[DiscordRPC] (%s) Could not perform kernel.ostype sysctl.\n", __func__);
        fclose(fp_ostype);
        fclose(fp_osrelease);
        fclose(fp_osarch);
        return NULL;
    }
    if (fgets(buf_osrelease, sizeof(buf_osrelease), fp_osrelease) == NULL) {
        printf("[DiscordRPC] (%s) Could not perform kernel.osrelease sysctl.\n", __func__);
        fclose(fp_ostype);
        fclose(fp_osrelease);
        fclose(fp_osarch);
        return NULL;
    }
    if (fgets(buf_osarch, sizeof(buf_osarch), fp_osarch) == NULL) {
        printf("[DiscordRPC] (%s) Could not perform kernel.arch sysctl.\n", __func__);
        fclose(fp_ostype);
        fclose(fp_osrelease);
        fclose(fp_osarch);
        return NULL;
    }
    fclose(fp_ostype);
    fclose(fp_osrelease);
    fclose(fp_osarch);

    // HACK: Since Linux removed the sysctl interface, I have to manually remove newlines from the /proc contents
    buf_ostype[strcspn(buf_ostype, "\n")] = '\0';
    buf_osrelease[strcspn(buf_osrelease, "\n")] = '\0';
    buf_osarch[strcspn(buf_osarch, "\n")] = '\0';

    char* osString = NULL;
    rc = asprintf(&osString, "on %s %s %s", buf_ostype, buf_osarch, buf_osrelease);
    if (rc == -1) {
        printf("[DiscordRPC] (%s) asprintf() failed.\n", __func__);
        return NULL;
    }
    return osString;
#elif defined(__APPLE__) && defined(__MACH__)
    // NOTE: Okay so I _could_ just print 'Darwin' for the OS Type, but on the 0.0001% chance that this is running on
    //       OpenDarwin / PureDarwin, I am fetching the name using a sysctl
    char buf_ostype[16];
    size_t sz_ostype = sizeof(buf_ostype);
    char buf_osrelease[16];
    size_t sz_osrelease = sizeof(buf_osrelease);
    int isArm64 = 0;
    size_t sz_isArm64 = sizeof(isArm64);
    int mib[CTL_MAXNAME];

    mib[0] = CTL_KERN;
    mib[1] = KERN_OSTYPE;
    if (sysctl(mib, 2, buf_ostype, &sz_ostype, NULL, 0) == -1) {
        printf("[DiscordRPC] (%s) Could not perform kern.ostype sysctl.\n", __func__);
        return NULL;
    }

    mib[1] = KERN_OSRELEASE;
    if (sysctl(mib, 2, buf_osrelease, &sz_osrelease, NULL, 0) == -1) {
        printf("[DiscordRPC] (%s) Could not perform kern.osrelease sysctl.\n", __func__);
        return NULL;
    }

    // hw.optional.arm64 does not seem to have a direct mib0/1 route
    size_t mib_len = CTL_MAXNAME;
    if (sysctlnametomib("hw.optional.arm64", mib, &mib_len) != 0) {
        printf("[DiscordRPC] (%s) Could not perform hw.optional.arm64 sysctl.\n", __func__);
        return NULL;
    }
    if (sysctl(mib, mib_len, &isArm64, &sz_isArm64, NULL, 0) != 0) {
        printf("[DiscordRPC] (%s) Could not perform hw.optional.arm64 sysctl.\n", __func__);
        return NULL;
    }

    char* osString = NULL;
    if (isArm64 == 1) {
        rc = asprintf(&osString, "on %s XNU aarch64 %s", buf_ostype, buf_osrelease);
    } else {
        rc = asprintf(&osString, "on %s XNU x86_64 %s", buf_ostype, buf_osrelease);
    }
    if (rc == -1) {
        printf("[DiscordRPC] (%s) asprintf() failed.\n", __func__);
        return NULL;
    }
    return osString;
#else
    // NOTE: This is not a critical error, just let the user know
    printf("[DiscordRPC] (%s) Could not fetch OS details.\n", __func__);
    return strdup("on Unknown");
#endif
}
