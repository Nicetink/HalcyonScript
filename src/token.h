/*
 * HalcyonScript - Token definitions
 */

#ifndef TOKEN_H
#define TOKEN_H

#include "../include/halcyon.h"

/* Token structure */
typedef struct {
    HcsTokenType type;
    char* value;
    int line;
    int column;
} HcsToken;

/* Token functions */
HcsToken* token_create(HcsTokenType type, const char* value, int line, int column);
void token_free(HcsToken* token);
const char* token_type_name(HcsTokenType type);

#endif /* TOKEN_H */
