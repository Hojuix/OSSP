/*
 * OpenSubsonicPlayer
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 * Info: OpenSubsonic /ping endpoint parser
 */

#ifndef _ENDPOINT_PING_H
#define _ENDPOINT_PING_H
#include <stdbool.h>
#include <stddef.h>
#include "httpclient.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    char* status;
    char* version;
    char* serverType;
    char* serverVersion;
    bool openSubsonicCapable;
    bool error;
    int errorCode;
    char* errorMessage;
} OSSP_endpoint_ping_t;

OSSP_endpoint_ping_t* OSSP_endpoint_ping_Constructor();
void OSSP_endpoint_ping_Deconstructor(OSSP_endpoint_ping_t* obj);
int OSSP_endpoint_ping_Parse(OSSP_httpCli_UrlObj_t* obj);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ENDPOINT_PING_H
