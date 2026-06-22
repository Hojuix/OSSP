/*
 * OpenSubsonicPlayer (OSSP)
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: Configuration Handler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "configHandler.h"
#include "external/cJSON.h"

#include "libopensubsonic/logger.h"
#include "libopensubsonic/utils.h"

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "OSSP", __VA_ARGS__)
#endif

OSSP_config_t* OSSP_configHandler_Constructor() {
    OSSP_config_t* obj = malloc(sizeof(OSSP_config_t));
    if (obj == NULL) {
        return NULL;
    }

    obj->opensubsonic_enable = false;
    obj->opensubsonic_server = NULL;
    obj->opensubsonic_username = NULL;
    obj->opensubsonic_password = NULL;

    obj->internal_opensubsonic_version = NULL;
    obj->internal_opensubsonic_clientName = NULL;
    obj->internal_opensubsonic_loginSalt = NULL;
    obj->internal_opensubsonic_loginToken = NULL;
    obj->internal_ossp_version = NULL;

    obj->local_music_enable = false;
    obj->local_music_rootdir = NULL;

    obj->listenbrainz_enable = false;
    obj->listenbrainz_token = NULL;
    obj->lastfm_enable = false;
    obj->lastfm_username = NULL;
    obj->lastfm_password = NULL;
    obj->lastfm_api_key = NULL;
    obj->lastfm_api_secret = NULL;
    obj->lastfm_api_session_key = NULL;

    obj->discordrpc_enable = false;
    obj->discordrpc_show_system_details = false;
    obj->discordrpc_show_cover_art = false;

    obj->audio_equalizer_enable = false;
    obj->audio_equalizer_preset_count = 0;
    obj->audio_equalizer_presets = NULL;

    obj->audio_pitch_enable = false;
    obj->audio_pitch_cents = 0.00;
    obj->audio_pitch_rate = 0.00;
    obj->audio_reverb_enable = false;
    obj->audio_reverb_wetDryMix = 0.00;

    obj->lv2_use_custom_path = false;
    obj->lv2_custom_path = NULL;
    obj->lv2_parax32_filter_name = NULL;
    obj->lv2_parax32_filter_type_left = NULL;
    obj->lv2_parax32_filter_type_right = NULL;
    obj->lv2_parax32_gain_left = NULL;
    obj->lv2_parax32_gain_right = NULL;
    obj->lv2_parax32_quality_left = NULL;
    obj->lv2_parax32_quality_right = NULL;
    obj->lv2_parax32_frequency_left = NULL;
    obj->lv2_parax32_frequency_right = NULL;
    obj->lv2_reverb_filter_name = NULL;

    return obj;
}

void OSSP_configHandler_Deconstructor(OSSP_config_t* obj) {
    OSS_SafeFree(obj->opensubsonic_server);
    OSS_SafeFree(obj->opensubsonic_username);
    OSS_SafeFree(obj->opensubsonic_password);

    OSS_SafeFree(obj->internal_opensubsonic_version);
    OSS_SafeFree(obj->internal_opensubsonic_clientName);
    OSS_SafeFree(obj->internal_opensubsonic_loginSalt);
    OSS_SafeFree(obj->internal_opensubsonic_loginToken);
    OSS_SafeFree(obj->internal_ossp_version);

    OSS_SafeFree(obj->local_music_rootdir);

    OSS_SafeFree(obj->listenbrainz_token);
    OSS_SafeFree(obj->lastfm_username);
    OSS_SafeFree(obj->lastfm_password);
    OSS_SafeFree(obj->lastfm_api_key);
    OSS_SafeFree(obj->lastfm_api_secret);
    OSS_SafeFree(obj->lastfm_api_session_key);

    for (int i = 0; i < obj->audio_equalizer_preset_count; i++) {
        OSS_SafeFree(obj->audio_equalizer_presets[i].name);
        OSS_SafeFree(obj->audio_equalizer_presets[i].audio_equalizer_graph);
    }
    OSS_SafeFree(obj->audio_equalizer_presets);
    
    OSS_SafeFree(obj->lv2_custom_path);
    OSS_SafeFree(obj->lv2_parax32_filter_name);
    OSS_SafeFree(obj->lv2_parax32_filter_type_left);
    OSS_SafeFree(obj->lv2_parax32_filter_type_right);
    OSS_SafeFree(obj->lv2_parax32_gain_left);
    OSS_SafeFree(obj->lv2_parax32_gain_right);
    OSS_SafeFree(obj->lv2_parax32_quality_left);
    OSS_SafeFree(obj->lv2_parax32_quality_right);
    OSS_SafeFree(obj->lv2_parax32_frequency_left);
    OSS_SafeFree(obj->lv2_parax32_frequency_right);
    OSS_SafeFree(obj->lv2_reverb_filter_name);

    OSS_SafeFree(obj);
}

int OSSP_configHandler_readConfig(OSSP_config_t* obj) {
    static int rc = 0;

    // Set internal Opensubsonic related values
    obj->internal_opensubsonic_version = strdup("1.8.0");
    obj->internal_opensubsonic_clientName = strdup("OSSP");
    obj->internal_ossp_version = strdup("v0.5a");

    // Create path to configuration file
    char* config_path = NULL;
#if defined(__ANDROID__)
    rc = asprintf(&config_path, "/sdcard/OSSP/config.json");
#else
    // Assume UNIX platform (*BSD/Linux/macOS).
    rc = asprintf(&config_path, "%s/.config/ossp/config.json", getenv("HOME"));
#endif
    printf("[ConfigHandler] Attempting to open configuration file at %s\n", config_path);

    FILE* config_fd = NULL;
    char* config_buf = NULL;
    long config_fsize = 0;
    long config_fread_rc = 0; // Needs to be separate from 'rc' as fread() returns bytes read

    struct stat st;
    if (stat(config_path, &st) == 0) {
        config_fsize = st.st_size;
    } else {
        printf("[ConfigHandler] stat() failed. Configuration file does not exist.\n");
        return 1;
    }

    // Actually open and read in the contents of the config file
    config_fd = fopen(config_path, "rb");
    if (config_fd == NULL) {
        printf("[ConfigHandler] fopen() failed. Configuration file could not be opened.\n");
        // TODO Add special note here about android being a PITA
        free(config_path);
        return 1;
    }
    free(config_path);

    config_buf = (char*)malloc(config_fsize + 1);
    if (config_buf == NULL) {
        printf("[ConfigHandler] malloc() failed. Could not allocate %ld bytes for configuration file.\n", config_fsize + 1);
        fclose(config_fd);
        return 1;
    }

    config_fread_rc = fread(config_buf, 1, config_fsize, config_fd);
    if (config_fread_rc != config_fsize) {
        printf("[ConfigHandler] fread() failed. Only read %ld bytes out of %ld bytes.\n", config_fread_rc, config_fsize);
        fclose(config_fd);
        free(config_buf);
        return 1;
    }

    // Null terminate the buffer
    config_buf[config_fsize] = '\0';
    fclose(config_fd);





    // Parse config JSON
    cJSON* root = cJSON_Parse(config_buf);
    free(config_buf);
    if (root == NULL) {
        printf("[ConfigHandler] cJSON_Parse() failed. Could not parse configuration file.\n");
        return 1;
    }






    cJSON* opensubsonic_root = cJSON_GetObjectItemCaseSensitive(root, "opensubsonic_server");
    if (opensubsonic_root != NULL) {
        OSS_Pboj(&obj->opensubsonic_enable, opensubsonic_root, "enable");
        OSS_Psoj(&obj->opensubsonic_server, opensubsonic_root, "server");
        OSS_Psoj(&obj->opensubsonic_username, opensubsonic_root, "username");
        OSS_Psoj(&obj->opensubsonic_password, opensubsonic_root, "password");
    } else {
        printf("[ConfigHandler] 'opensubsonic_server' section missing from configuration file.\n");
    }

    cJSON* local_music_root = cJSON_GetObjectItemCaseSensitive(root, "local_music");
    if (local_music_root != NULL) {
        OSS_Pboj(&obj->local_music_enable, local_music_root, "enable");
        OSS_Psoj(&obj->local_music_rootdir, local_music_root, "root_directory");
    } else {
        printf("[ConfigHandler] 'local_music' section missing from configuration file.\n");
    }

    cJSON* scrobbler_root = cJSON_GetObjectItemCaseSensitive(root, "scrobbler");
    if (scrobbler_root != NULL) {
        OSS_Pboj(&obj->listenbrainz_enable, scrobbler_root, "listenbrainz_enable");
        OSS_Psoj(&obj->listenbrainz_token, scrobbler_root, "listenbrainz_token");
        OSS_Pboj(&obj->lastfm_enable, scrobbler_root, "lastfm_enable");
        OSS_Psoj(&obj->lastfm_username, scrobbler_root, "lastfm_username");
        OSS_Psoj(&obj->lastfm_password, scrobbler_root, "lastfm_password");
        OSS_Psoj(&obj->lastfm_api_key, scrobbler_root, "lastfm_api_key");
        OSS_Psoj(&obj->lastfm_api_secret, scrobbler_root, "lastfm_api_secret");
        OSS_Psoj(&obj->lastfm_api_session_key, scrobbler_root, "lastfm_session_key");
    } else {
        printf("[ConfigHandler] 'scrobbler' section missing from configuration file.\n");
    }

    cJSON* discord_rpc_root = cJSON_GetObjectItemCaseSensitive(root, "discord_rpc");
    if (discord_rpc_root != NULL) {
        OSS_Pboj(&obj->discordrpc_enable, discord_rpc_root, "enable");
        OSS_Pboj(&obj->discordrpc_show_system_details, discord_rpc_root, "show_system_details");
        OSS_Pboj(&obj->discordrpc_show_cover_art, discord_rpc_root, "show_cover_art");
    } else {
        printf("[ConfigHandler] 'discord_rpc' section missing from configuraton file.\n");
    }

    // I am genuinely incredibly sorry for what you are about to read...
    cJSON* audio_root = cJSON_GetObjectItemCaseSensitive(root, "audio");
    if (audio_root != NULL) {
        cJSON* equalizer_root = cJSON_GetObjectItemCaseSensitive(audio_root, "equalizer");
        if (equalizer_root != NULL) {
            OSS_Pboj(&obj->audio_equalizer_enable, equalizer_root, "enable");
            cJSON* equalizer_presets_array = cJSON_GetObjectItemCaseSensitive(equalizer_root, "presets");
            if (equalizer_presets_array != NULL) {
                obj->audio_equalizer_preset_count = cJSON_GetArraySize(equalizer_presets_array);
                obj->audio_equalizer_presets = malloc(obj->audio_equalizer_preset_count * sizeof(OSSP_config_eqPreset_t));
                if (obj->audio_equalizer_presets != NULL) {
                    for (int i = 0; i < obj->audio_equalizer_preset_count; i++) {
                        cJSON* equalizer_preset = cJSON_GetArrayItem(equalizer_presets_array, i);
                        if (equalizer_preset != NULL) {
                            obj->audio_equalizer_presets[i].name = NULL;
                            obj->audio_equalizer_presets[i].follow_pitch = false;
                            OSS_Psoj(&obj->audio_equalizer_presets[i].name, equalizer_preset, "name");
                            OSS_Pboj(&obj->audio_equalizer_presets[i].follow_pitch, equalizer_preset, "follow_pitch");
                            cJSON* equalizer_preset_graph = cJSON_GetObjectItemCaseSensitive(equalizer_preset, "graph");
                            if (equalizer_preset_graph != NULL) {
                                obj->audio_equalizer_presets[i].graph_count = cJSON_GetArraySize(equalizer_preset_graph);
                                obj->audio_equalizer_presets[i].audio_equalizer_graph = malloc(obj->audio_equalizer_presets[i].graph_count * sizeof(OSSP_config_eqGraph_t));
                                if (obj->audio_equalizer_presets[i].audio_equalizer_graph != NULL) {
                                    for (int j = 0; j < obj->audio_equalizer_presets[i].graph_count; j++) {
                                        obj->audio_equalizer_presets[i].audio_equalizer_graph[j].position = 0;
                                        obj->audio_equalizer_presets[i].audio_equalizer_graph[j].bandwidth = 0.00;
                                        obj->audio_equalizer_presets[i].audio_equalizer_graph[j].frequency = 0;
                                        obj->audio_equalizer_presets[i].audio_equalizer_graph[j].gain = 0.00;
                                        obj->audio_equalizer_presets[i].audio_equalizer_graph[j].bypass = false;

                                        cJSON* equalizer_preset_graph_item = cJSON_GetArrayItem(equalizer_preset_graph, j);
                                        if (equalizer_preset_graph_item != NULL) {
                                            OSS_Pioj(&obj->audio_equalizer_presets[i].audio_equalizer_graph[j].position, equalizer_preset_graph_item, "frequency");
                                            OSS_Pdoj(&obj->audio_equalizer_presets[i].audio_equalizer_graph[j].bandwidth, equalizer_preset_graph_item, "bandwidth");
                                            OSS_Pioj(&obj->audio_equalizer_presets[i].audio_equalizer_graph[j].frequency, equalizer_preset_graph_item, "frequency");
                                            OSS_Pdoj(&obj->audio_equalizer_presets[i].audio_equalizer_graph[j].gain, equalizer_preset_graph_item, "gain");
                                            OSS_Pboj(&obj->audio_equalizer_presets[i].audio_equalizer_graph[j].bypass, equalizer_preset_graph_item, "bypass");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        cJSON* pitch_root = cJSON_GetObjectItemCaseSensitive(audio_root, "pitch");
        if (pitch_root != NULL) {
            OSS_Pboj(&obj->audio_pitch_enable, pitch_root, "enable");
            OSS_Pdoj(&obj->audio_pitch_cents, pitch_root, "cents");
            OSS_Pdoj(&obj->audio_pitch_rate, pitch_root, "rate");
        } else {
            printf("[ConfigHandler] 'pitch' section missing from configuration file.\n");
        }

        cJSON* reverb_root = cJSON_GetObjectItemCaseSensitive(audio_root, "reverb");
        if (reverb_root != NULL) {
            OSS_Pboj(&obj->audio_reverb_enable, reverb_root, "enable");
            OSS_Pdoj(&obj->audio_reverb_wetDryMix, reverb_root, "wet_dry_mix");
        } else {
            printf("[ConfigHandler] 'reverb' section missing from configuration file.\n");
        }

        cJSON* lv2_root = cJSON_GetObjectItemCaseSensitive(audio_root, "lv2");
        if (lv2_root != NULL) {
            OSS_Pboj(&obj->lv2_use_custom_path, lv2_root, "use_custom_lv2_path");
            OSS_Psoj(&obj->lv2_custom_path, lv2_root, "custom_lv2_path");

            cJSON* lsp_para_x32_lr_root = cJSON_GetObjectItemCaseSensitive(lv2_root, "lsp_para_x32_lr");
            if (lsp_para_x32_lr_root != NULL) {
                OSS_Psoj(&obj->lv2_parax32_filter_name, lsp_para_x32_lr_root, "filter_name");
                OSS_Psoj(&obj->lv2_parax32_filter_type_left, lsp_para_x32_lr_root, "filter_type_left");
                OSS_Psoj(&obj->lv2_parax32_filter_type_right, lsp_para_x32_lr_root, "filter_type_right");
                OSS_Psoj(&obj->lv2_parax32_gain_left, lsp_para_x32_lr_root, "gain_left");
                OSS_Psoj(&obj->lv2_parax32_gain_right, lsp_para_x32_lr_root, "gain_right");
                OSS_Psoj(&obj->lv2_parax32_quality_left, lsp_para_x32_lr_root, "quality_left");
                OSS_Psoj(&obj->lv2_parax32_quality_right, lsp_para_x32_lr_root, "quality_right");
                OSS_Psoj(&obj->lv2_parax32_frequency_left, lsp_para_x32_lr_root, "frequency_left");
                OSS_Psoj(&obj->lv2_parax32_frequency_right, lsp_para_x32_lr_root, "frequency_right");
            } else {
                printf("[ConfigHandler] 'lsp_para_x32_lr' section missing from configuration file.\n");
            }

            cJSON* calf_reverb_root = cJSON_GetObjectItemCaseSensitive(lv2_root, "calf_reverb");
            if (calf_reverb_root != NULL) {
                OSS_Psoj(&obj->lv2_reverb_filter_name, calf_reverb_root, "filter_name");
            } else {
                printf("[ConfigHandler] 'calf_reverb' section missing from configuration file.\n");
            }
        } else {
            printf("[ConfigHandler] 'lv2' section missing from configuration file.\n");
        }
    } else {
        printf("[ConfigHandler] 'audio' section missing from configuration file.\n");
    }

    cJSON_Delete(root);
    printf("[ConfigHandler] Successfully parsed configuration file.\n");
    return 0;
cleanup:
    printf("[ConfigHandler] Something fatal happened while parsing configuration file.\n");
    return 1;
}
