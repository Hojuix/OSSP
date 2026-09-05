/*
 * Discord RPC Library
 * Goldenkrew3000 / Hojuix 2026
 * License: GNU General Public License 3.0
 */

#ifndef _DISCORDRPC_UTILS_H
#define _DISCORDRPC_UTILS_H
#include <stdint.h>

char* Rpc_Util_GenerateUUID();
uint32_t Rpc_Util_GenerateRandomPID();
void Rpc_Util_SafeFree(void** ptr);

#endif
