/*
 * HalcyonScript - Parser
 */

#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"

typedef struct {
    HcsToken** tokens;
    int token_count;
    int pos;
} HcsParser;

/* Parser functions */
HcsParser* parser_create(HcsToken** tokens, int token_count);
void parser_free(HcsParser* parser);
HcsAstNode* parser_parse(HcsParser* parser);

#endif /* PARSER_H */
