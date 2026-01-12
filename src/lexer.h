/*
 * HalcyonScript - Lexer
 */

#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
    const char* source;
    int position;
    int line;
    int column;
    int length;
} HcsLexer;

/* Lexer functions */
HcsLexer* lexer_create(const char* source);
void lexer_free(HcsLexer* lexer);
HcsToken** lexer_tokenize(HcsLexer* lexer, int* token_count);
void lexer_free_tokens(HcsToken** tokens, int count);

#endif /* LEXER_H */
