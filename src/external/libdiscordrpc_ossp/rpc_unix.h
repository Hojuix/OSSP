/*
 * Discord RPC Library
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 */

#ifndef _DISCORDRPC_RPC_UNIX_H
#define _DISCORDRPC_RPC_UNIX_H

int Rpc_Unix_Initialize(char* client_id);
const char* Rpc_Unix_GetTempPath();
int Rpc_Unix_SendFrame(char* payload);

#endif
