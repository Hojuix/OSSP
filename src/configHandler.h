/*
 * OpenSubsonicPlayer (OSSP)
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Configuration Handler
 */

#ifndef _CONFIG_HANDLER_H
#define _CONFIG_HANDLER_H
#include <stdbool.h>

typedef struct {
    double bandwidth;
    int frequency;                      // Frequency in Hz
    double gain;                        // Gain in db
    bool bypass;                        // Ignore entry
} OSSP_config_eqGraph_t;

typedef struct {
    // Opensubsonic Settings
    bool opensubsonic_enable;
    char* opensubsonic_server;                  // protocol://address:port
    char* opensubsonic_username;
    char* opensubsonic_password;

    // Internal Opensubsonic Settings
    char* internal_opensubsonic_version;        // (Internal) Opensubsonic API Version
    char* internal_opensubsonic_clientName;     // (Internal) Opensubsonic Client Name
    char* internal_opensubsonic_loginSalt;      // (Internal) Opensubsonic Login Salt
    char* internal_opensubsonic_loginToken;     // (Internal) Opensubsonic Login Token
    char* internal_ossp_version;                // (Internal) OSSP Version

    // Local Settings
    bool local_music_enable;                  // Enable Local Music Playback
    char* local_music_rootdir;                // Local Music Root Directory

    // Scrobbler Settings
    bool listenbrainz_enable;           // Enable ListenBrainz Scrobbling
    char* listenbrainz_token;           // ListenBrainz Token
    bool lastfm_enable;                 // Enable LastFM Scrobbling
    char* lastfm_username;              // LastFM Username
    char* lastfm_password;              // LastFM Password
    char* lastfm_api_key;               // LastFM API Key
    char* lastfm_api_secret;            // LastFM API Secret
    char* lastfm_api_session_key;       // LastFM API Session Key (Generated from authorization endpoint)

    // Discord RPC Settings
    bool discordrpc_enable;             // Enable Discord RPC
    bool discordrpc_show_system_details;// Show 'on OS ARCH VERSION' in RPC
    bool discordrpc_show_cover_art;     // Show cover art instead of app icon (Leaks OSS credentials to Discord)

    // Audio Settings
    bool audio_equalizer_enable;
    bool audio_equalizer_follow_pitch;  // Have equalizer align to pitch adjustment
    int audio_equalizer_graph_count;
    OSSP_config_eqGraph_t* audio_equalizer_graph;
    bool audio_pitch_enable;
    double audio_pitch_cents;
    double audio_pitch_rate;
    bool audio_reverb_enable;
    double audio_reverb_wetDryMix;      // Reverb Wet/Dry Mix Percent

    // LV2 Audio Settings
    bool lv2_use_custom_path;
    char* lv2_custom_path;
    char* lv2_parax32_filter_name;      // LV2 LSP Para Equalizer x32 LR LV2 Name
    char* lv2_parax32_filter_type_left; // LV2 LSP Para Equalizer x32 LR Filter type left name
    char* lv2_parax32_filter_type_right;
    char* lv2_parax32_gain_left;
    char* lv2_parax32_gain_right;
    char* lv2_parax32_quality_left;
    char* lv2_parax32_quality_right;
    char* lv2_parax32_frequency_left;
    char* lv2_parax32_frequency_right;
    char* lv2_reverb_filter_name;       // LV2 Calf Reverb LV2 Name
} OSSP_config_t;

OSSP_config_t* OSSP_configHandler_Constructor();
void OSSP_configHandler_Deconstructor(OSSP_config_t* obj);
int OSSP_configHandler_readConfig(OSSP_config_t* obj);

#endif
