#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "../external/cJSON.h"
#include "httpclient.h"
#include "../configHandler.h"

#include "endpoint_ping.h"
#include "endpoint_getStarred.h"
#include "endpoint_getSong.h"
#include "endpoint_getPlaylists.h"
#include "endpoint_getPlaylist.h"
#include "endpoint_getArtists.h"
#include "endpoint_getArtist.h"
#include "endpoint_getLyricsBySongId.h"
#include "endpoint_getAlbumList.h"
#include "endpoint_getAlbum.h"
#include "endpoint_scrobble.h"
#include "endpoint_getInternetRadioStations.h"

static int rc = 0;
extern configHandler_config_t* configObj;

/*
 * URL Constructor/Deconstructor
 */
OSSP_httpCli_UrlObj_t* OSSP_httpCli_UrlObj_Constructor() {    
    OSSP_httpCli_UrlObj_t* obj = malloc(sizeof(OSSP_httpCli_UrlObj_t));
    if (obj == NULL) {
        return NULL;
    }
    obj->endpoint = 0;
    obj->id = NULL;
    obj->type = 0;
    obj->amount = 0;
    obj->submit = false;
    obj->url = NULL;
    obj->reqBody = NULL;
    obj->httpMethod = 0;
    obj->isBodyReq = false;
    obj->resCode = 0;
    obj->resBody = NULL;
    obj->returnStruct = NULL;
    return obj;
}

void OSSP_httpCli_UrlObj_Deconstructor(OSSP_httpCli_UrlObj_t* obj) {
    //logger_log_general(__func__, "Freeing URL object with endpoint ID of %d.", obj->endpoint);
    printf("hjaha %s\n", obj->url);
    if (obj->url != NULL) { free(obj->url); }
    if (obj->id != NULL) { free(obj->id); }
    if (obj->reqBody != NULL) { free(obj->reqBody); }
    if (obj->resBody != NULL) { free(obj->resBody); }
    if (obj != NULL) { free(obj); }
}

// ----
int OSSP_httpCli_createURL(OSSP_httpCli_UrlObj_t* obj) {
    switch (obj->endpoint) {
        case OPENSUBSONIC_ENDPOINT_PING:
            rc = asprintf(&obj->url, "%s://%s/rest/ping?u=%s&t=%s&s=%s&f=json&v=%s&c=%s",
                          configObj->opensubsonic_protocol, configObj->opensubsonic_server, configObj->opensubsonic_username,
                          configObj->internal_opensubsonic_loginToken, configObj->internal_opensubsonic_loginSalt,
                          configObj->internal_opensubsonic_version, configObj->internal_opensubsonic_clientName);
            obj->httpMethod = HTTP_METHOD_GET;
            break;
        default:
            return 1;
            break;
    }



    return 0;
}

// ---
int OSSP_httpCli_sendReq(OSSP_httpCli_UrlObj_t* obj) {
    static int rc = 0;
    OSSP_httpCli_UNIXHttpReq(obj);
    
    if (obj->resBody == NULL) {
        printf("[LibOpenSubsonic] HTTP Request returned no data.\n");
        return 1;
    } else {
        switch (obj->endpoint) {
            case OPENSUBSONIC_ENDPOINT_PING:
                obj->returnStruct = OSSP_endpoint_ping_Constructor();
                if (obj->returnStruct == NULL) {
                    printf("[LibOpenSubsonic] (%s) malloc() failed.\n", __func__);
                    return 1;
                }
                rc = OSSP_endpoint_ping_Parse(obj);
                if (rc == 1) { return 1; } // Errors are already logged in their respective functions
                break;
            default:
                break;
        }
    }

    return 0;
}







/*
// Functions for preparing / freeing a HTTP Request struct
void opensubsonic_httpClient_prepareRequest(opensubsonic_httpClientRequest_t** httpReq) {
    // Allocate struct
    *httpReq = (opensubsonic_httpClientRequest_t*)malloc(sizeof(opensubsonic_httpClientRequest_t));
    
    // Initialize struct variables
    (*httpReq)->requestUrl = NULL;
    (*httpReq)->requestBody = NULL;
    (*httpReq)->method = 0;
    (*httpReq)->isBodyRequired = false;
    (*httpReq)->scrobbler = 0;
    (*httpReq)->responseCode = 0;
    (*httpReq)->responseMsg = NULL;
}

void opensubsonic_httpClient_cleanup(opensubsonic_httpClientRequest_t** httpReq) {
    // Free heap-allocated struct variables
    if ((*httpReq)->requestUrl != NULL) { free((*httpReq)->requestUrl); }
    if ((*httpReq)->requestBody != NULL) { free((*httpReq)->requestBody); }
    if ((*httpReq)->responseMsg != NULL) { free((*httpReq)->responseMsg); }
    
    // Free struct
    free(*httpReq);
}
    */

// Perform HTTP POST for Scrobbling (This function is a wrapper around OS-specific networking functions)
//int opensubsonic_httpClient_request(opensubsonic_httpClientRequest_t** httpReq) {
//    logger_log_general(__func__, "Performing HTTP Request.");

//    UNIX_HttpRequest(httpReq);

 //   return 0;
//}







struct memory {
    char *data;
    size_t size;
};

static size_t write_to_memory(void *ptr, size_t size, size_t nmemb, void *userdata) {
    struct memory *mem = (struct memory *)userdata;
    size_t total_size = size * nmemb;

    mem->data = realloc(mem->data, mem->size + total_size + 1);
    if (!mem->data) return 0;  // Fail on OOM

    memcpy(&(mem->data[mem->size]), ptr, total_size);
    mem->size += total_size;
    mem->data[mem->size] = '\0';  // Null-terminate

    return total_size;
}

void OSSP_httpCli_UNIXHttpReq(OSSP_httpCli_UrlObj_t* obj) {
    CURL* curl_handle = curl_easy_init();
    struct curl_slist* header_list = NULL;
    struct memory chunk = {0};
    long httpCode = 0;

    if (curl_handle) {
        // Set method (GET/POST)
        if (obj->httpMethod == HTTP_METHOD_GET) {
            curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
        } else if (obj->httpMethod == HTTP_METHOD_POST) {
            curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "POST");
            if (obj->isBodyReq == false) {
                header_list = curl_slist_append(header_list, "Content-Length: 0");
            }
        }

        /*
        // Set scrobbler information
        if ((*httpReq)->scrobbler == SCROBBLER_LISTENBRAINZ && (*httpReq)->method == HTTP_METHOD_POST) {
            header_list = curl_slist_append(header_list, "Content-Type: application/json");
            char* authString = NULL;
            rc = asprintf(&authString, "Authorization: Token %s", configObj->listenbrainz_token);
            if (rc == -1) {
                logger_log_error(__func__, "asprintf() error.");
                // TODO handle error
            }
            printf("CODE: %s\n", authString);
            header_list = curl_slist_append(header_list, authString); // TODO Check does this copy the string?
            // TODO free auth string
        }
        
        if ((*httpReq)->isBodyRequired == true && (*httpReq)->scrobbler == 0) {
            header_list = curl_slist_append(header_list, "X-Organization: Hojuix");
            header_list = curl_slist_append(header_list, "X-Application: OSSP");
        }
        
        if (header_list != NULL) {
            curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, header_list);
        }
        
        if ((*httpReq)->isBodyRequired == true) {
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, (*httpReq)->requestBody);
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen((*httpReq)->requestBody));
        }
        */
        
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "OSSP/1.0 (avery@hojuix.org)");
        curl_easy_setopt(curl_handle, CURLOPT_URL, obj->url);
        curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE,0L);

// Do not use SSL verification on iOS due to an SSL issue with using libcurl on iOS
#if defined(__APPLE__) && defined(__MACH__) && defined(XCODE)
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
#else
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 1L);
#endif // defined(__APPLE__) && defined(__MACH__) && defined(XCODE)

        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_to_memory);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        CURLcode res = curl_easy_perform(curl_handle);
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httpCode);
    }

    curl_easy_cleanup(curl_handle);

    if (chunk.data != NULL) {
        obj->resBody = strdup(chunk.data);
        free(chunk.data);
    }
    obj->resCode = (int)httpCode;
}

/*
void UNIX_HttpRequest(opensubsonic_httpClientRequest_t** httpReq) {
    CURL* curl_handle = curl_easy_init();
    struct curl_slist* header_list = NULL;
    struct memory chunk = {0};
    long httpCode = 0;

    if (curl_handle) {
        // Set method
        if ((*httpReq)->method == HTTP_METHOD_GET) {
            curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
        } else if ((*httpReq)->method == HTTP_METHOD_POST) {
            curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "POST");
            if ((*httpReq)->isBodyRequired == false) {
                header_list = curl_slist_append(header_list, "Content-Length: 0");
            }
        }
        
        // Set scrobbler information
        if ((*httpReq)->scrobbler == SCROBBLER_LISTENBRAINZ && (*httpReq)->method == HTTP_METHOD_POST) {
            header_list = curl_slist_append(header_list, "Content-Type: application/json");
            char* authString = NULL;
            rc = asprintf(&authString, "Authorization: Token %s", configObj->listenbrainz_token);
            if (rc == -1) {
                logger_log_error(__func__, "asprintf() error.");
                // TODO handle error
            }
            printf("CODE: %s\n", authString);
            header_list = curl_slist_append(header_list, authString); // TODO Check does this copy the string?
            // TODO free auth string
        }
        
        if ((*httpReq)->isBodyRequired == true && (*httpReq)->scrobbler == 0) {
            header_list = curl_slist_append(header_list, "X-Organization: Hojuix");
            header_list = curl_slist_append(header_list, "X-Application: OSSP");
        }
        
        if (header_list != NULL) {
            curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, header_list);
        }
        
        if ((*httpReq)->isBodyRequired == true) {
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, (*httpReq)->requestBody);
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen((*httpReq)->requestBody));
        }
        
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "OSSP/1.0 (avery@hojuix.org)");
        curl_easy_setopt(curl_handle, CURLOPT_URL, (*httpReq)->requestUrl);
        curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE,0L);

// Do not use SSL verification on iOS due to an SSL issue with using libcurl on iOS
#if defined(__APPLE__) && defined(__MACH__) && defined(XCODE)
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
#else
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 1L);
#endif // defined(__APPLE__) && defined(__MACH__) && defined(XCODE)

        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_to_memory);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        CURLcode res = curl_easy_perform(curl_handle);
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &httpCode);
    }

    curl_easy_cleanup(curl_handle);

    (*httpReq)->responseMsg = strdup(chunk.data);
    (*httpReq)->responseCode = (int)httpCode;
    free(chunk.data);
}
*/