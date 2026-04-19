/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: OpenSubsonic /ping endpoint parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../external/cJSON.h"
#include "utils.h"
#include "endpoint_ping.h"

OSSP_endpoint_ping_t* OSSP_endpoint_ping_Constructor() {
    printf("[LibOpenSubsonic] Running /ping Constructor.\n");
    OSSP_endpoint_ping_t* obj = malloc(sizeof(OSSP_endpoint_ping_t));
    if (obj == NULL) {
        return NULL;
    }
    obj->status = NULL;
    obj->version = NULL;
    obj->serverType = NULL;
    obj->serverVersion = NULL;
    obj->openSubsonicCapable = false;
    obj->error = false;
    obj->errorCode = 0;
    obj->errorMessage = NULL;
    return obj;
}

void OSSP_endpoint_ping_Deconstructor(OSSP_endpoint_ping_t* obj) {
    printf("[LibOpenSubsonic] Running /ping Deconstructor.\n");
    if (obj->status != NULL) { free(obj->status); }
    if (obj->version != NULL) { free(obj->version); }
    if (obj->serverType != NULL) { free(obj->serverType); }
    if (obj->serverVersion != NULL) { free(obj->serverVersion); }
    if (obj->errorMessage != NULL) { free(obj->errorMessage); }
    if (obj != NULL) { free(obj); }
}

int OSSP_endpoint_ping_Parse(OSSP_httpCli_UrlObj_t* obj) {
    OSSP_endpoint_ping_t* structObj = obj->returnStruct;

    // Parse the JSON
    cJSON* root = cJSON_Parse(obj->resBody);
    if (root == NULL) {
        printf("[LibOpenSubsonic] (%s) Error parsing JSON.\n", __func__);
        return 1;
    }

    // Make an object from subsonic-response
    cJSON* subsonic_root = cJSON_GetObjectItemCaseSensitive(root, "subsonic-response");
    if (subsonic_root == NULL) {
        printf("[LibOpenSubsonic] (%s) Error handling JSON - subsonic-response does not exist.\n", __func__);
        cJSON_Delete(root);
        return 1;
    }
    
    OSS_Psoj(&structObj->status, subsonic_root, "status");
    OSS_Psoj(&structObj->version, subsonic_root, "version");
    OSS_Psoj(&structObj->serverType, subsonic_root, "type");
    OSS_Psoj(&structObj->serverVersion, subsonic_root, "serverVersion");
    OSS_Pboj(&structObj->openSubsonicCapable, subsonic_root, "openSubsonic");

    // Check if an error is present
    cJSON* subsonic_error = cJSON_GetObjectItemCaseSensitive(subsonic_root, "error");
    if (subsonic_error == NULL) {
        // Error did not occur, return
        cJSON_Delete(root);
        return 0;
    }
    structObj->error = true;

    // From this point on, error has occured, capture error information
    OSS_Pioj(&structObj->errorCode, subsonic_error, "code");
    OSS_Psoj(&structObj->errorMessage, subsonic_error, "message");
    printf("[LibOpenSubsonic] (%s) Error noted in JSON - Code %d: %s\n", __func__, structObj->errorCode, structObj->errorMessage);

    cJSON_Delete(root);
    return 1;
}
