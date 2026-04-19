#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../external/cJSON.h"
#include "logger.h"
#include "utils.h"

/*
 * ACRONYMS:
 * OSS_Psoj -> Opensubsonic_PullStringOutofJson
 * OSS_Pioj -> Opensubsonic_PullIntOutofJson
 * OSS_Ploj -> Opensubsonic_PullLongOutofJson
 * OSS_Pboj -> Opensubsonic_PullBoolOutofJson
 */

void OSS_Psoj(char** dest, cJSON* obj, char* child) {
    if (obj != NULL) {
        cJSON* childObj = cJSON_GetObjectItem(obj, child);
        if (cJSON_IsString(childObj) && childObj->valuestring != NULL) {
            *dest = strdup(childObj->valuestring);
        }
    }
}

void OSS_Pioj(int* dest, cJSON* obj, char* child) {
    if (obj != NULL) {
        cJSON* childObj = cJSON_GetObjectItem(obj, child);
        if (cJSON_IsNumber(childObj)) {
            *dest = childObj->valueint;
        }
    }
}

void OSS_Ploj(long* dest, cJSON* obj, char* child) {
    if (obj != NULL) {
        cJSON* childObj = cJSON_GetObjectItem(obj, child);
        if (cJSON_IsNumber(childObj)) {
            *dest = childObj->valueint;
        }
    }
}

void OSS_Pboj(bool* dest, cJSON* obj, char* child) {
    if (obj != NULL) {
        cJSON* childObj = cJSON_GetObjectItem(obj, child);
        if (cJSON_IsBool(childObj)) {
            if (cJSON_IsTrue(childObj)) {
                *dest = true;
            }
        }
    }
}

