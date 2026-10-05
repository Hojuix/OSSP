/*
 * Discord RPC Library
 * Goldenkrew3000 / gk3k / Hojuix 2026
 * License: GNU General Public License 3.0
 */

#include <stdio.h> // snprintf()
#include <stdlib.h> // free()
#include <sys/random.h> // Linux getrandom()
#include "utils.h"

/*
 * Generate a UUID. This is used only for the nonce in RPC messages.
 * Returns a heap-allocated UUID string on success, or NULL on failure.
 */
char* Rpc_Util_GenerateUUID() {
    static int rc = 0;
    uint8_t random_bytes[16] = { 0x00 };

    for (size_t i = 0; i < sizeof(random_bytes); i++) {
        random_bytes[i] = arc4random() & 0xFF;
    }

    char* uuid_string = NULL;
    rc = asprintf(&uuid_string, "%.2x%.2x%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x",
             random_bytes[0], random_bytes[1], random_bytes[2], random_bytes[3],
             random_bytes[4], random_bytes[5], random_bytes[6], random_bytes[7],
             random_bytes[8], random_bytes[9], random_bytes[10], random_bytes[11],
             random_bytes[12], random_bytes[13], random_bytes[14], random_bytes[15]);
    if (rc < 0) {
        return NULL;
    }
    return uuid_string;
}

/*
 * Generate a random PID (Required field for SET_ACTIVITY).
 * Respectfully, fuck off Discord. You don't need to know the PID.
 * Yes, I am generating a massive number on purpose to reduce possible collision with a real PID.
 * TODO fix comment after changing to arc4random() for *BSD support
 */
unsigned int Rpc_Util_GenerateRandomPID() {
    unsigned int rand_number = arc4random() & 0xFF;
    return rand_number;
}

void Rpc_Util_SafeFree(void** ptr) {
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}
