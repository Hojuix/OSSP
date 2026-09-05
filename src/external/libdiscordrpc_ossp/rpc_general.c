/*
 * Discord RPC Library
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 */

#include <stdio.h>
#include <stdlib.h> // free()
#include "../cJSON.h"
#include "utils.h"
#include "rpc_unix.h"
#include "rpc_general.h"

int Rpc_General_Initialize(char* client_id) {
    static int rc = 0;
    // ifdef unix TODO - do i even support anything else, non-unix is trash anyway
    rc = Rpc_Unix_Initialize(client_id);
    return rc;
}

Discord_RPC_SendActivity_t* Rpc_General_SetActivity_Constructor() {
    Discord_RPC_SendActivity_t* obj = malloc(sizeof(Discord_RPC_SendActivity_t));
    if (obj == NULL) {
#ifdef DISCORDRPC_DEBUG
        printf("[Rpc_General_SetActivity_Constructor] malloc() failed.\n");
#endif
        return NULL;
    }
    obj->state = NULL;
    obj->state_url = NULL;
    obj->details = NULL;
    obj->details_url = NULL;
    obj->activity_type = -1;
    obj->timestamp_start = -1;
    obj->timestamp_end = -1;
    obj->large_image = NULL;
    obj->large_text = NULL;
    obj->large_url = NULL;
    obj->small_image = NULL;
    obj->small_text = NULL;
    obj->small_url = NULL;
    return obj;
}

void Rpc_General_SetActivity_Deconstructor(Discord_RPC_SendActivity_t** obj_ptr) {
    Discord_RPC_SendActivity_t* obj = *obj_ptr;
    Rpc_Util_SafeFree((void*)&obj->state);
    Rpc_Util_SafeFree((void*)&obj->state_url);
    Rpc_Util_SafeFree((void*)&obj->details);
    Rpc_Util_SafeFree((void*)&obj->details_url);
    Rpc_Util_SafeFree((void*)&obj->large_image);
    Rpc_Util_SafeFree((void*)&obj->large_text);
    Rpc_Util_SafeFree((void*)&obj->large_url);
    Rpc_Util_SafeFree((void*)&obj->small_image);
    Rpc_Util_SafeFree((void*)&obj->small_text);
    Rpc_Util_SafeFree((void*)&obj->small_url);
    Rpc_Util_SafeFree((void*)&obj);
}

int Rpc_General_SetActivity(Discord_RPC_SendActivity_t* activity) {
    // TODO: Technically those cJSON_Create* are not safe...
    cJSON* root = cJSON_CreateObject();
    cJSON* args_root = cJSON_CreateObject();
    cJSON* activity_root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "args", args_root);
    cJSON_AddItemToObject(args_root, "activity", activity_root);

    cJSON_AddStringToObject(root, "cmd", "SET_ACTIVITY");
    char* nonce = Rpc_Util_GenerateUUID();
    if (nonce != NULL) {
        cJSON_AddStringToObject(root, "nonce", nonce);
        // Now yes I am aware that this seems involved for a simple free and set to NULL, and I am also
        // aware that simply sending the pointer and freeing it would free it (but not clear it out here)
        // Basically: I know this looks weird, but I also know what I am doing
        Rpc_Util_SafeFree((void*)&nonce);
    } else {
#ifdef DISCORDRPC_DEBUG
        // As per Discord documentation, the nonce should be different everytime. But as a failure measure
        // (of which if this fails, we have bigger issues anyway), I'll set it to a default value anyway
        cJSON_AddStringToObject(root, "nonce", "0");
        printf("[Rpc_General_SetActivity] Rpc_Util_GenerateUUID() returned NULL.\n");
#endif
    }

    // Personally, I don't think sending a PID to Discord is a great idea (they store _everything_)
    cJSON_AddNumberToObject(args_root, "pid", Rpc_Util_GenerateRandomPID());

    if (activity->state != NULL) {
        cJSON_AddStringToObject(activity_root, "state", activity->state);
    }
    if (activity->state_url != NULL) {
        cJSON_AddStringToObject(activity_root, "state_url", activity->state_url);
    }
    if (activity->details != NULL) {
        cJSON_AddStringToObject(activity_root, "details", activity->details);
    }
    if (activity->details_url != NULL) {
        cJSON_AddStringToObject(activity_root, "details_url", activity->details_url);
    }
    if (activity->activity_type != -1) {
        cJSON_AddNumberToObject(activity_root, "type", DISCORDRPC_ACTIVITY_TYPE_LISTENING);
    }

    if (activity->timestamp_start != -1 || activity->timestamp_end != -1) {
        cJSON* timestamps_root = cJSON_CreateObject();
        cJSON_AddItemToObject(activity_root, "timestamps", timestamps_root);
        if (activity->timestamp_start != -1) {
            cJSON_AddNumberToObject(timestamps_root, "start", activity->timestamp_start);
        }
        if (activity->timestamp_end != -1) {
            cJSON_AddNumberToObject(timestamps_root, "end", activity->timestamp_end);
        }
    }

    if (activity->large_image != NULL || activity->large_text != NULL || activity->large_url != NULL ||
        activity->small_image != NULL || activity->small_text != NULL || activity->small_url != NULL) {
        cJSON* assets_root = cJSON_CreateObject();
        cJSON_AddItemToObject(activity_root, "assets", assets_root);

        if (activity->large_image != NULL) {
            cJSON_AddStringToObject(assets_root, "large_image", activity->large_image);
        }
        if (activity->large_text != NULL) {
            cJSON_AddStringToObject(assets_root, "large_text", activity->large_text);
        }
        if (activity->large_url != NULL) {
            cJSON_AddStringToObject(assets_root, "large_url", activity->large_url);
        }
        if (activity->small_image != NULL) {
            cJSON_AddStringToObject(assets_root, "small_image", activity->small_image);
        }
        if (activity->small_text != NULL) {
            cJSON_AddStringToObject(assets_root, "small_text", activity->small_text);
        }
        if (activity->small_url != NULL) {
            cJSON_AddStringToObject(assets_root, "small_url", activity->small_url);
        }
    }

#ifdef DISCORDRPC_DEBUG
    char* setactivity_formatted = cJSON_Print(root);
    if (setactivity_formatted != NULL) {
        printf("Set Activity Payload:\n%s\n", setactivity_formatted);
        Rpc_Util_SafeFree((void*)&setactivity_formatted);
    } else {
        printf("[Rpc_General_SetActivity] setactivity_formatted cJSON_Print() failed.\n");
    }
#endif

    char* payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload != NULL) {
        Rpc_Unix_SendFrame(payload);
        Rpc_Util_SafeFree((void*)&payload);
    }

    return 0;
}
