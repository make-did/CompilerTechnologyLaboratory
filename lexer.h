#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Token类型枚举
typedef enum {
    // 保留字
    TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_DO, TOKEN_MAIN,
    TOKEN_INT, TOKEN_FLOAT, TOKEN_DOUBLE, TOKEN_RETURN,
    TOKEN_CONST, TOKEN_VOID, TOKEN_CONTINUE, TOKEN_BREAK,
    TOKEN_CHAR, TOKEN_UNSIGNED, TOKEN_ENUM, TOKEN_LONG,
    TOKEN_SWITCH, TOKEN_CASE, TOKEN_AUTO, TOKEN_STATIC,
    
    // 特殊符号
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,
    TOKEN_ASSIGN, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE,
    TOKEN_EQ, TOKEN_NE, TOKEN_AND, TOKEN_OR,
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_QUOTE, TOKEN_DQUOTE,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_DOT, TOKEN_COLON,
    
    // 其他
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER, TOKEN_FLOAT_NUM, TOKEN_HEX, TOKEN_OCTAL,
    TOKEN_CHAR_CONST, TOKEN_STRING_CONST,
    
    // 特殊
    TOKEN_EOF, TOKEN_ERROR
} TokenType;

// Token结构体
typedef struct {
    TokenType type;
    char lexeme[256];     // 词素
    int line;             // 所在行号
    int column;           // 所在列号
    union {
        int int_val;      // 整数值
        float float_val;  // 浮点数值
        char char_val;    // 字符值
    } value;
} Token;

// 词法分析器状态
typedef struct {
    FILE* source;         // 源文件指针
    char current_char;    // 当前字符
    int line;             // 当前行
    int column;           // 当前列
    bool has_error;       // 是否有错误
} Lexer;

// 函数声明
Lexer* init_lexer(const char* filename);
void free_lexer(Lexer* lexer);
Token get_token(Lexer* lexer);  // 对应实验要求的GetToken()
const char* token_type_to_str(TokenType type);
const char* token_type_to_code(TokenType type);  // 返回类型编码

// 保留字查找
TokenType lookup_keyword(const char* lexeme);

#endif