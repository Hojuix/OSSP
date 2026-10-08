/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 / gk3k / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Discord RPC Handler
 */

// TODO Add checks on whether stuff is null

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "external/libdiscordrpc_ossp/rpc_general.h"
#include "configHandler.h"
#include "libopensubsonic/utils.h"
#include "discordrpc.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/sysctl.h>
#endif // defined(__APPLE__) && defined(__MACH__)

#if defined(__linux__)
#include <sys/utsname.h>
#endif // defined(__linux__)

extern OSSP_config_t* configObj;
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

// TODO update error codes

int OSSP_discordrpc_Init() {
    printf("[DiscordRPC] Initializing.\n");

    if (configObj->discordrpc_appid == NULL) {
        printf("[DiscordRPC] Discord RPC is enabled, but App ID is null.\n");
        return -1;
    }

    rc = Rpc_General_Initialize(configObj->discordrpc_appid);
    if (rc != 0) {
        printf("Could not connect to Discord RPC.\n");
        return 1;
    }
    printf("Connected to Discord RPC.\n");

    // Fetch OS String for RPC (Heap-allocated)
    discordrpc_osString = OSSP_discordrpc_getOS();
    if (discordrpc_osString == NULL) {
        printf("[DiscordRPC] (%s) asprintf() or strdup() failed.\n", __func__);
        return 1;
    }
    return 0;
}

void OSSP_discordrpc_update(OSSP_discordrpc_t* obj) {
    // TODO handle osString being NULL
    printf("[DiscordRPC] Updating...\n");

    Discord_RPC_SendActivity_t* activity = Rpc_General_SetActivity_Constructor();

    //DiscordRichPresence presence;
    char* detailsString = NULL;
    char* stateString = NULL;
    //memset(&presence, 0, sizeof(presence));

    if (obj->state == DISCORDRPC_STATE_IDLE) {
        printf("[DiscordRPC] Issuing Idle RPC.\n");
        asprintf(&detailsString, "Idle");
        //presence.details = detailsString;
        activity->details = strdup(detailsString);
    } else if (obj->state == DISCORDRPC_STATE_PLAYING_OPENSUBSONIC ||
           (obj->state == DISCORDRPC_STATE_PLAYING_LOCALFILE)) {


        // Playing a song from an OpenSubsonic server
        printf("[DiscordRPC] Issuing OpenSubsonic/Local File Song RPC.\n");
        asprintf(&detailsString, "%s", obj->songTitle);
        asprintf(&stateString, "by %s", obj->songArtist);
        activity->details = strdup(detailsString);
        activity->state = strdup(stateString);
        if (obj->state == DISCORDRPC_STATE_PLAYING_OPENSUBSONIC) {
            // TODO As of now, local file playback does NOT deal with cover art
            //presence.largeImageKey = obj->coverArtUrl;
            // TODO find out what largeImageKey references in the API
        }
        activity->timestamp_start = (long)(obj->startTime);
        activity->timestamp_end = (long)(obj->startTime) + obj->songLength;
        if (configObj->discordrpc_show_system_details) {
            activity->large_text = strdup(discordrpc_osString);
        }


    } else if (obj->state == DISCORDRPC_STATE_PLAYING_INTERNETRADIO) {
        // Playing an internet radio station
        printf("[DiscordRPC] Issuing Internet Radio RPC.\n");
        asprintf(&detailsString, "%s", obj->songTitle);
        asprintf(&stateString, "Internet radio station");
        //presence.details = detailsString;
        //presence.state = stateString;
        //presence.largeImageKey = obj->coverArtUrl; // TODO also this
        //presence.startTimestamp = (long)(obj->startTime);
        activity->details = strdup(detailsString);
        activity->state = strdup(stateString);
        activity->timestamp_start = (long)(obj->startTime);
        if (configObj->discordrpc_show_system_details) {
            //presence.largeImageText = discordrpc_osString;
            activity->large_text = strdup(discordrpc_osString);
        }
    } else if (obj->state == DISCORDRPC_STATE_PAUSED) {
        // Player is paused
        printf("[DiscordRPC] Issuing Paused RPC.\n");
        asprintf(&detailsString, "Paused");
        //presence.details = detailsString;
        activity->details = strdup(detailsString);
    }

    activity->activity_type = DISCORDRPC_ACTIVITY_TYPE_PLAYING;
    Rpc_General_SetActivity(activity);
    Rpc_General_SetActivity_Deconstructor(&activity);
    //presence.activity_type = DISCORD_ACTIVITY_TYPE_LISTENING;
    //Discord_UpdatePresence(&presence);

    free(detailsString);
    if (stateString != NULL) { free(stateString); }
}

char* OSSP_discordrpc_getOS() {
#if defined(__linux__)
    char* osString = NULL;

    // Attempt to read /etc/os-release
    FILE* fp_osrelease = fopen("/etc/os-release", "r");
    if (!fp_osrelease) {
        printf("[DiscordRPC] Could not open /etc/os-release.\n");
        return NULL;
    }

    char* key_linux_osrelease_name = "NAME";
    char* key_linux_osrelease_version_id = "VERSION_ID";
    char* osrelease_name = NULL;
    char* osrelease_version_id = NULL;
    char line[1024];

    while (fgets(line, sizeof(line), fp_osrelease)) {
        if (strncmp(line, key_linux_osrelease_name, strlen(key_linux_osrelease_name)) == 0) {
            osrelease_name = OSSP_discordrpc_extractDataFromKeyLine(line);
        } else if (strncmp(line, key_linux_osrelease_version_id, strlen(key_linux_osrelease_version_id)) == 0) {
            osrelease_version_id = OSSP_discordrpc_extractDataFromKeyLine(line);
        }
    }

    // Fetch the rest of the data from a uname() call
    // (Yes I know I could use precompiler macros for the architecture, but what if I want to do something else someday too)
    struct utsname uname_buffer;
    int uname_success = -1;
    if (uname(&uname_buffer) != 0) {
        printf("[DiscordRPC] uname() failed.\n");
    } else {
        uname_success = 0;
    }

    // Perform sanity checks
    if (osrelease_name == NULL || osrelease_version_id == NULL) {
        printf("[DiscordRPC] Could not read either OS Release (NAME) or OS Version (VERSION_ID) from /etc/release.\n");
        OSSP_SafeFree((void**)&osrelease_name);
        OSSP_SafeFree((void**)&osrelease_version_id);

        if (uname_success == 0) {
            // Unable to read /etc/os-release, but uname() was successful
            // String: on Unknown Linux (ARCH KERN_VERSION)
            rc = asprintf(&osString, "on Unknown Linux (%s)", uname_buffer.machine);
            if (rc == -1) {
                printf("[DiscordRPC] failed allocation.\n");
                return NULL;
            }
        } else {
            // Unable to read /etc/os-release, and uname() was unsuccessful
            // String: on Unknown Linux
            rc = asprintf(&osString, "on Unknown Linux");
            if (rc == -1) {
                printf("[DiscordRPC] failed allocation.\n");
                return NULL;
            }
        }
    } else {
        if (uname_success == 0) {
            // Read /etc/os-release, and uname() was successful
            // String: on OSNAME OSVERSION (ARCH KERN_VERSION)
            rc = asprintf(&osString, "on %s %s (%s)", osrelease_name, osrelease_version_id,
                uname_buffer.machine);
            if (rc == -1) {
                OSSP_SafeFree((void**)&osrelease_name);
                OSSP_SafeFree((void**)&osrelease_version_id);
                return NULL;
            }
        } else {
            // Read /etc/os-release, but uname() was unsuccessful
            // String: on OSNAME OSVERSION
            rc = asprintf(&osString, "on %s %s", osrelease_name, osrelease_version_id);
            if (rc == -1) {
                OSSP_SafeFree((void**)&osrelease_name);
                OSSP_SafeFree((void**)&osrelease_version_id);
                return NULL;
            }
        }
    }

    // Afaik Discord has a 128 byte limit on any entry (which the OS string is one), so prevent going over that.
    if (strlen(osString) >= 128) {
        printf("[DsicrdRPC] OS String is too long, setting to 'on Unknown Linux'.\n");
        OSSP_SafeFree((void**)&osString);
        rc = asprintf(&osString, "on Unknown Linux");
        if (rc == -1) {
            printf("[DiscordRPC] failed allocation.\n");
            return NULL;
        }
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

char* OSSP_discordrpc_extractDataFromKeyLine(char* line) {
    // I am sorry for this mess, but this is why most of the string altering heavy
    // code in OSSP is handled in C++. I don't know how to cleanly and safely do this
    // in C. But the following code works by finding both the double quotes, and doing
    // pointer math to get the number of bytes. Then the data between those two pointers
    // is copied into a new variable, as a null terminated string. Again, I'm sorry >_<
    char* string_beginning = strchr(line, '"');
    if (string_beginning == NULL) {
        printf("[DiscordRPC/extractDataFromKeyLine] Could not find beginning of string.\n");
        return NULL;
    }
    string_beginning += sizeof(char); // Pointer to first character
    char* end_quote = strchr(string_beginning, '"'); // Pointer to last '"', ptr-1 is the last character
    if (end_quote == NULL) {
        printf("[DiscordRPC/extractDataFromKeyLine] Could not find end of string.\n");
        return NULL;
    }
    int bytes = end_quote - string_beginning;
    if (bytes > strlen(line)) {
        printf("[DiscordRPC/extractDataFromKeyLine] Pointer distance between line beginning and end is too big (%d bytes).\n", bytes);
        return NULL;
    }
    char* clean_data = malloc((bytes + sizeof(char)) * sizeof(char));
    if (clean_data == NULL) {
        printf("[DiscordRPC/extractDataFromKeyLine] failed allocation.\n");
        return NULL;
    }
    memcpy(clean_data, string_beginning, bytes * sizeof(char));
    clean_data[bytes] = '\0';
    return clean_data;
}
