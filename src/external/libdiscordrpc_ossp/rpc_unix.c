/*
 * Discord RPC Library
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 */

#include <stdio.h> // printf
#include <stdlib.h> // getenv()
#include <string.h> // memset()
#include <errno.h>
#include "../cJSON.h"
#include "rpc_general.h"
#include "rpc_unix.h"
#include "utils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>

int server_fd = -1;

/*
 * --
 * Returns -1 on failure. Returns 0 on success.
 */
int Rpc_Unix_Initialize(char* client_id) {
    static int rc = 0;
    struct sockaddr_un server_addr;

    // Locate the IPC Socket
    /*
     * Discord's Flatpak puts it at $XDG_RUNTIME_DIR/.flatpak/com.discordapp.Discord/xdg-run/discord-ipc-0
     * Vesktop's Flatpak puts it at $XDG_RUNTIME_DIR/.flatpak/dev.vencord.Vesktop/xdg-run/discord-ipc-0
     * Discord and Vesktop (native) put it at $XDG_RUNTIME_DIR/discord-ipc-0
     * Though I have also seen it at /tmp/discord-ipc-0
     */
    const char* tmp_path = Rpc_Unix_GetTempPath();
    printf("Temp path: %s\n", tmp_path);

    // scanning blah blah
    char* discord_ipc_path = NULL;
    asprintf(&discord_ipc_path, "/run/user/1000/.flatpak/com.discordapp.Discord/xdg-run/discord-ipc-0"); // TODO





    // Connect to the socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Could not create socket: %s\n", strerror(errno));
        return -1;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, discord_ipc_path, sizeof(server_addr.sun_path) - 1);
    free(discord_ipc_path); discord_ipc_path = NULL; // TODO

    if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Could not connect to socket: %s\n", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Assemble and send handshake
    cJSON* handshake_root = cJSON_CreateObject();
    if (handshake_root == NULL) {
        close(server_fd);
        return -1;
    }
    cJSON_AddNumberToObject(handshake_root, "v", 1);
    cJSON_AddStringToObject(handshake_root, "client_id", client_id);
    char* payload_str = cJSON_PrintUnformatted(handshake_root);
    cJSON_Delete(handshake_root);
    if (payload_str == NULL) {
        close(server_fd);
        return -1;
    }

    Discord_RPCHeader_t handshake_hdr;
    handshake_hdr.type = DISCORDRPC_RPC_TYPE_HANDSHAKE;
    handshake_hdr.size = strlen(payload_str);

    rc = send(server_fd, &handshake_hdr, sizeof(Discord_RPCHeader_t), 0);
    if (rc == -1) {
        free(payload_str); payload_str = NULL;
        close(server_fd);
        return -1;
    }

    rc = send(server_fd, payload_str, strlen(payload_str), 0);
    free(payload_str); payload_str = NULL;
    if (rc == -1) {
        close(server_fd);
        return -1;
    }

    // Receive response from Discord
    uint8_t handshake_resp_hdr_data[8];
    rc = recv(server_fd, &handshake_resp_hdr_data, sizeof(handshake_resp_hdr_data), 0);
    if (rc == -1 || rc != sizeof(handshake_resp_hdr_data)) {
        close(server_fd);
        return -1;
    }
    Discord_RPCHeader_t* handshake_resp_hdr = (Discord_RPCHeader_t*)&handshake_resp_hdr_data;

    char* handshake_resp_data = malloc(handshake_resp_hdr->size * sizeof(char));
    if (handshake_resp_data == NULL) {
        close(server_fd);
        return -1;
    }

    rc = recv(server_fd, handshake_resp_data, handshake_resp_hdr->size, 0);
    if (rc == -1 || rc != handshake_resp_hdr->size) {
        free(handshake_resp_data); handshake_resp_data = NULL;
        close(server_fd);
        return -1;
    }


    //printf("ret: %s\n", handshake_resp_data); // invalid read size from valgrind
    // TODO: Seems to return 4000 when no client id is provided

    Rpc_Util_SafeFree((void*)&handshake_resp_data);


    return 0;
}

void Rpc_Unix_Close() {
    close(server_fd);
    server_fd = -1;
}

/*
 * Send frame
 * Do not close blah blah
 */
int Rpc_Unix_SendFrame(char* payload) {
#ifdef DISCORDRPC_DEBUG
    printf("[Rpc_Unix_SendFrame] Sending Frame.\n");
    printf("[Rpc_Unix_SendFrame] Payload is %s\n", payload);
#endif
    static int rc = 0;

    Discord_RPCHeader_t handshake_req_hdr;
    handshake_req_hdr.type = DISCORDRPC_RPC_TYPE_FRAME;
    handshake_req_hdr.size = strlen(payload);

    rc = send(server_fd, &handshake_req_hdr, sizeof(Discord_RPCHeader_t), 0);
    if (rc == -1) {
        return -1;
    }

    rc = send(server_fd, payload, strlen(payload), 0);
    if (rc == -1) {
        return -1;
    }

    uint8_t handshake_resp_hdr_data[8];
    rc = recv(server_fd, &handshake_resp_hdr_data, sizeof(handshake_resp_hdr_data), 0);
    if (rc == -1 || rc != sizeof(handshake_resp_hdr_data)) {
        return -1;
    }
    Discord_RPCHeader_t* handshake_resp_hdr = (Discord_RPCHeader_t*)&handshake_resp_hdr_data;

    printf("recv with type %d, sending back %d\n", handshake_resp_hdr->type, handshake_resp_hdr->size);

    char* handshake_resp_data = malloc(handshake_resp_hdr->size * sizeof(char));
    if (handshake_resp_data == NULL) {
        return -1;
    }

    rc = recv(server_fd, handshake_resp_data, handshake_resp_hdr->size, 0);
    if (rc == -1 || rc != handshake_resp_hdr->size) {
        free(handshake_resp_data); handshake_resp_data = NULL;
        return -1;
    }
    //printf("%s\n", handshake_resp_data);

    Rpc_Util_SafeFree((void*)&handshake_resp_data);

    return 0;
}

const char* Rpc_Unix_GetTempPath() {
    // copied blah blah
    const char* temp = getenv("XDG_RUNTIME_DIR");
    temp = temp ? temp : getenv("TMPDIR");
    temp = temp ? temp : getenv("TMP");
    temp = temp ? temp : getenv("TEMP");
    temp = temp ? temp : "/tmp";
    return temp;
}

/*
 * Open the Discord IPC UNIX Socket
 * Returns ---
 */
int Rpc_Unix_Open() {

}



