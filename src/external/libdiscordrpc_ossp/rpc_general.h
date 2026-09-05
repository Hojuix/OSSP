/*
 * Discord RPC Library
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 */

#ifndef _DISCORDRPC_RPC_GENERAL_H
#define _DISCORDRPC_RPC_GENERAL_H
#include <stdint.h> // uint32_t

#define DISCORDRPC_RPC_TYPE_HANDSHAKE       0
#define DISCORDRPC_RPC_TYPE_FRAME           1
#define DISCORDRPC_RPC_TYPE_CLOSE           2
#define DISCORDRPC_RPC_TYPE_PING            3
#define DISCORDRPC_RPC_TYPE_PONG            4

#define DISCORDRPC_ACTIVITY_TYPE_PLAYING    0
#define DISCORDRPC_ACTIVITY_TYPE_LISTENING  2
#define DISCORDRPC_ACTIVITY_TYPE_WATCHING   3
#define DISCORDRPC_ACTIVITY_TYPE_COMPETING  5

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) Discord_RPCHeader_t;

typedef struct {
    char* state;
    char* state_url;
    char* details;
    char* details_url;
    int activity_type;
    long timestamp_start; // TODO: Do I use long for a UNIX timestamp?
    long timestamp_end;
    char* large_image;
    char* large_text;
    char* large_url;
    char* small_image;
    char* small_text;
    char* small_url;
} Discord_RPC_SendActivity_t;

Discord_RPC_SendActivity_t* Rpc_General_SetActivity_Constructor();
void Rpc_General_SetActivity_Deconstructor(Discord_RPC_SendActivity_t** obj_ptr);

int Rpc_General_Initialize(char* client_id);
int Rpc_General_SetActivity(Discord_RPC_SendActivity_t* activity);

#endif
