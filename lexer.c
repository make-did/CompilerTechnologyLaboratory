#include "lexer.h"

// 保留字表
typedef struct {
    const char* word;
    TokenType type;
} Keyword;

Keyword keywords[] = {
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"do", TOKEN_DO},
    {"main", TOKEN_MAIN},
    {"int", TOKEN_INT},
    {"float", TOKEN_FLOAT},
    {"double", TOKEN_DOUBLE},
    {"return", TOKEN_RETURN},
    {"const", TOKEN_CONST},
    {"void", TOKEN_VOID},
    {"continue", TOKEN_CONTINUE},
    {"break", TOKEN_BREAK},
    {"char", TOKEN_CHAR},
    {"unsigned", TOKEN_UNSIGNED},
    {"enum", TOKEN_ENUM},
    {"long", TOKEN_LONG},
    {"switch", TOKEN_SWITCH},
    {"case", TOKEN_CASE},
    {"auto", TOKEN_AUTO},
    {"static", TOKEN_STATIC},
    {NULL, TOKEN_IDENTIFIER}  // 结束标记
};

// 初始化词法分析器
Lexer* init_lexer(const char* filename) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    
    lexer->source = fopen(filename, "r");
    if (!lexer->source) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        free(lexer);
        return NULL;
    }
    
    lexer->line = 1;
    lexer->column = 0;
    lexer->has_error = false;
    lexer->current_char = fgetc(lexer->source);
    
    // 跳过UTF-8 BOM (如果存在)
    if (lexer->current_char == 0xEF) {
        int b1 = fgetc(lexer->source);
        int b2 = fgetc(lexer->source);
        if (b1 == 0xBB && b2 == 0xBF) {
            lexer->current_char = fgetc(lexer->source);
        } else {
            ungetc(b2, lexer->source);
            ungetc(b1, lexer->source);
        }
    }
    
    return lexer;
}

// 释放资源
void free_lexer(Lexer* lexer) {
    if (lexer) {
        if (lexer->source) {
            fclose(lexer->source);
        }
        free(lexer);
    }
}

// 获取下一个字符
void advance(Lexer* lexer) {
    if (lexer->current_char != EOF) {
        lexer->current_char = fgetc(lexer->source);
        lexer->column++;
        
        if (lexer->current_char == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
    }
}

// 查看下一个字符而不移动指针
char peek(Lexer* lexer) {
    int ch = fgetc(lexer->source);
    ungetc(ch, lexer->source);
    return (char)ch;
}

// 跳过空白符（空格、制表符）
void skip_whitespace(Lexer* lexer) {
    while (lexer->current_char == ' ' || lexer->current_char == '\t') {
        advance(lexer);
    }
}

// 跳过单行注释 //
void skip_single_line_comment(Lexer* lexer) {
    while (lexer->current_char != '\n' && lexer->current_char != EOF) {
        advance(lexer);
    }
    if (lexer->current_char == '\n') {
        advance(lexer);
    }
}

// 跳过多行注释 /* */
void skip_multi_line_comment(Lexer* lexer) {
    // 跳过/*!
    advance(lexer);  // 跳过 /
    advance(lexer);  // 跳过 *
    
    while (!(lexer->current_char == '*' && peek(lexer) == '/')) {
        if (lexer->current_char == EOF) {
            fprintf(stderr, "Error at line %d: Unclosed multi-line comment\n", lexer->line);
            lexer->has_error = true;
            return;
        }
        advance(lexer);
    }
    
    // 跳过 */
    advance(lexer);  // 跳过 *
    advance(lexer);  // 跳过 /
}

// 识别标识符或保留字
Token identifier(Lexer* lexer) {
    Token token;
    token.type = TOKEN_IDENTIFIER;
    token.line = lexer->line;
    token.column = lexer->column;
    
    int i = 0;
    while (isalnum(lexer->current_char) || lexer->current_char == '_') {
        if (i < 255) {
            token.lexeme[i++] = lexer->current_char;
        }
        advance(lexer);
    }
    token.lexeme[i] = '\0';
    
    // 检查是否为保留字
    TokenType keyword_type = lookup_keyword(token.lexeme);
    if (keyword_type != TOKEN_IDENTIFIER) {
        token.type = keyword_type;
    }
    
    return token;
}

// 识别数字（支持十进制、十六进制、八进制、浮点数）
Token number(Lexer* lexer) {
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    char buffer[256];
    int i = 0;
    bool is_float = false;
    bool is_hex = false;
    bool is_octal = false;
    
    // 处理十六进制 0x 或 0X
    if (lexer->current_char == '0') {
        buffer[i++] = lexer->current_char;
        advance(lexer);
        
        if (lexer->current_char == 'x' || lexer->current_char == 'X') {
            buffer[i++] = lexer->current_char;
            advance(lexer);
            is_hex = true;
            token.type = TOKEN_HEX;
        } else if (isdigit(lexer->current_char)) {
            is_octal = true;
            token.type = TOKEN_OCTAL;
        } else {
            // 只是0
            token.type = TOKEN_INTEGER;
        }
    } else {
        token.type = TOKEN_INTEGER;
    }
    
    // 收集数字
    if (is_hex) {
        // 十六进制数字
        while (isxdigit(lexer->current_char)) {
            if (i < 255) buffer[i++] = lexer->current_char;
            advance(lexer);
        }
    } else {
        // 十进制或八进制
        while (isdigit(lexer->current_char) || 
               (lexer->current_char == '.' && !is_float)) {
            
            if (lexer->current_char == '.') {
                is_float = true;
                token.type = TOKEN_FLOAT_NUM;
            }
            
            if (i < 255) buffer[i++] = lexer->current_char;
            advance(lexer);
        }
        
        // 科学计数法
        if (lexer->current_char == 'e' || lexer->current_char == 'E') {
            is_float = true;
            token.type = TOKEN_FLOAT_NUM;
            if (i < 255) buffer[i++] = lexer->current_char;
            advance(lexer);
            
            // 指数符号
            if (lexer->current_char == '+' || lexer->current_char == '-') {
                if (i < 255) buffer[i++] = lexer->current_char;
                advance(lexer);
            }
            
            // 指数数字
            while (isdigit(lexer->current_char)) {
                if (i < 255) buffer[i++] = lexer->current_char;
                advance(lexer);
            }
        }
    }
    
    // 检查后缀（如L, U, F等）
    while (isalpha(lexer->current_char)) {
        if (i < 255) buffer[i++] = lexer->current_char;
        advance(lexer);
    }
    
    buffer[i] = '\0';
    strcpy(token.lexeme, buffer);
    
    // 转换数值
    if (is_float) {
        token.value.float_val = atof(buffer);
    } else {
        if (is_hex) {
            token.value.int_val = (int)strtol(buffer, NULL, 16);
        } else if (is_octal) {
            token.value.int_val = (int)strtol(buffer, NULL, 8);
        } else {
            token.value.int_val = atoi(buffer);
        }
    }
    
    return token;
}

// 识别字符常量,包含单引号
Token character(Lexer* lexer) {
    Token token;
    token.type = TOKEN_CHAR_CONST;
    token.line = lexer->line;
    token.column = lexer->column;
    
    char buffer[256];
    int i = 0;
    
    // 保存开头的单引号
    buffer[i++] = '\'';
    
    advance(lexer);  // 跳过开头的单引号
    
    // 处理转义字符或普通字符
    if (lexer->current_char == '\\') {
        // 转义字符
        buffer[i++] = '\\';
        advance(lexer);
        if (lexer->current_char == 'n' || lexer->current_char == 't' || 
            lexer->current_char == '\\' || lexer->current_char == '\'' ||
            lexer->current_char == '"' || lexer->current_char == '0') {
            buffer[i++] = lexer->current_char;
            advance(lexer);
        } else {
            fprintf(stderr, "Error at line %d: Invalid escape sequence\n", lexer->line);
            lexer->has_error = true;
            token.type = TOKEN_ERROR;
            return token;
        }
    } else if (lexer->current_char == '\'' || lexer->current_char == '\n' || lexer->current_char == EOF) {
        fprintf(stderr, "Error at line %d: Invalid character constant\n", lexer->line);
        lexer->has_error = true;
        token.type = TOKEN_ERROR;
        return token;
    } else {
        // 普通字符
        token.value.char_val = lexer->current_char;
        buffer[i++] = lexer->current_char;
        advance(lexer);
    }
    
    // 保存结尾的单引号
    buffer[i++] = '\'';
    
    // 检查并跳过结束的单引号
    if (lexer->current_char != '\'') {
        fprintf(stderr, "Error at line %d: Unclosed character constant\n", lexer->line);
        lexer->has_error = true;
        token.type = TOKEN_ERROR;
    } else {
        advance(lexer);  // 跳过结束的单引号
    }
    
    buffer[i] = '\0';
    strcpy(token.lexeme, buffer);
    
    return token;
}

// 识别字符串常量 ,包含双引号
Token string(Lexer* lexer) {
    Token token;
    token.type = TOKEN_STRING_CONST;
    token.line = lexer->line;
    token.column = lexer->column;
    
    char buffer[256];
    int i = 0;
    
    // 保存开头的双引号
    buffer[i++] = '"';
    
    advance(lexer);  // 跳过开头的双引号
    
    // 获取字符串内容
    while (lexer->current_char != '"' && lexer->current_char != EOF && 
           lexer->current_char != '\n') {
        
        // 处理转义字符
        if (lexer->current_char == '\\') {
            if (i < 254) {
                buffer[i++] = '\\';
                advance(lexer);
                if (lexer->current_char == 'n' || lexer->current_char == 't' || 
                    lexer->current_char == '\\' || lexer->current_char == '"' ||
                    lexer->current_char == '\'' || lexer->current_char == '0') {
                    buffer[i++] = lexer->current_char;
                    advance(lexer);
                } else {
                    fprintf(stderr, "Error at line %d: Invalid escape sequence in string\n", lexer->line);
                    lexer->has_error = true;
                    token.type = TOKEN_ERROR;
                    return token;
                }
            } else {
                // 缓冲区溢出
                break;
            }
        } else {
            if (i < 255) {
                buffer[i++] = lexer->current_char;
            }
            advance(lexer);
        }
    }
    
    // 保存结尾的双引号
    if (i < 255) {
        buffer[i++] = '"';
    }
    
    // 检查结束的双引号
    if (lexer->current_char != '"') {
        fprintf(stderr, "Error at line %d: Unclosed string constant\n", lexer->line);
        lexer->has_error = true;
        token.type = TOKEN_ERROR;
    } else {
        advance(lexer);  // 跳过结束的双引号
    }
    
    buffer[i] = '\0';
    strcpy(token.lexeme, buffer);
    
    return token;
}

// 词法分析函数
Token get_token(Lexer* lexer) {
    Token token;
    
    // 跳过空白符
    skip_whitespace(lexer);
    
    // 处理注释
    while (lexer->current_char == '/') {
        char next = peek(lexer);
        if (next == '/') {
            // 单行注释
            skip_single_line_comment(lexer);
            skip_whitespace(lexer);
        } else if (next == '*') {
            // 多行注释
            skip_multi_line_comment(lexer);
            skip_whitespace(lexer);
        } else {
            break;
        }
    }
    
    // 设置基本属性
    token.line = lexer->line;
    token.column = lexer->column;
    
    // 处理文件结束
    if (lexer->current_char == EOF) {
        token.type = TOKEN_EOF;
        strcpy(token.lexeme, "EOF");
        return token;
    }
    
    // 处理换行符
    if (lexer->current_char == '\n') {
        advance(lexer);
        return get_token(lexer);  // 递归获取下一个token
    }
    
    // 标识符：字母或下划线开头
    if (isalpha(lexer->current_char) || lexer->current_char == '_') {
        return identifier(lexer);
    }
    
    // 数字
    if (isdigit(lexer->current_char)) {
        return number(lexer);
    }
    
    // 字符常量
    if (lexer->current_char == '\'') {
        return character(lexer);
    }
    
    // 字符串常量
    if (lexer->current_char == '"') {
        return string(lexer);
    }
    
    // 处理特殊符号
    char current = lexer->current_char;
    advance(lexer);
    
    // 检查双字符运算符
    switch (current) {
        case '+':
            token.type = TOKEN_PLUS;
            strcpy(token.lexeme, "+");
            break;
        case '-':
            token.type = TOKEN_MINUS;
            strcpy(token.lexeme, "-");
            break;
        case '*':
            token.type = TOKEN_MULTIPLY;
            strcpy(token.lexeme, "*");
            break;
        case '/':
            token.type = TOKEN_DIVIDE;
            strcpy(token.lexeme, "/");
            break;
        case '=':
            if (lexer->current_char == '=') {
                token.type = TOKEN_EQ;
                strcpy(token.lexeme, "==");
                advance(lexer);
            } else {
                token.type = TOKEN_ASSIGN;
                strcpy(token.lexeme, "=");
            }
            break;
        case '<':
            if (lexer->current_char == '=') {
                token.type = TOKEN_LE;
                strcpy(token.lexeme, "<=");
                advance(lexer);
            } else {
                token.type = TOKEN_LT;
                strcpy(token.lexeme, "<");
            }
            break;
        case '>':
            if (lexer->current_char == '=') {
                token.type = TOKEN_GE;
                strcpy(token.lexeme, ">=");
                advance(lexer);
            } else {
                token.type = TOKEN_GT;
                strcpy(token.lexeme, ">");
            }
            break;
        case '!':
            if (lexer->current_char == '=') {
                token.type = TOKEN_NE;
                strcpy(token.lexeme, "!=");
                advance(lexer);
            } else {
                fprintf(stderr, "Error at line %d: Invalid operator '!'\n", token.line);
                lexer->has_error = true;
                token.type = TOKEN_ERROR;
                strcpy(token.lexeme, "!");
            }
            break;
        case '&':
            if (lexer->current_char == '&') {
                token.type = TOKEN_AND;
                strcpy(token.lexeme, "&&");
                advance(lexer);
            } else {
                fprintf(stderr, "Error at line %d: Invalid operator '&'\n", token.line);
                lexer->has_error = true;
                token.type = TOKEN_ERROR;
                strcpy(token.lexeme, "&");
            }
            break;
        case '|':
            if (lexer->current_char == '|') {
                token.type = TOKEN_OR;
                strcpy(token.lexeme, "||");
                advance(lexer);
            } else {
                fprintf(stderr, "Error at line %d: Invalid operator '|'\n", token.line);
                lexer->has_error = true;
                token.type = TOKEN_ERROR;
                strcpy(token.lexeme, "|");
            }
            break;
        case '{':
            token.type = TOKEN_LBRACE;
            strcpy(token.lexeme, "{");
            break;
        case '}':
            token.type = TOKEN_RBRACE;
            strcpy(token.lexeme, "}");
            break;
        case '(':
            token.type = TOKEN_LPAREN;
            strcpy(token.lexeme, "(");
            break;
        case ')':
            token.type = TOKEN_RPAREN;
            strcpy(token.lexeme, ")");
            break;
        case '[':
            token.type = TOKEN_LBRACKET;
            strcpy(token.lexeme, "[");
            break;
        case ']':
            token.type = TOKEN_RBRACKET;
            strcpy(token.lexeme, "]");
            break;
        case ';':
            token.type = TOKEN_SEMICOLON;
            strcpy(token.lexeme, ";");
            break;
        case ',':
            token.type = TOKEN_COMMA;
            strcpy(token.lexeme, ",");
            break;
        case '.':
            token.type = TOKEN_DOT;
            strcpy(token.lexeme, ".");
            break;
        case ':':
            token.type = TOKEN_COLON;
            strcpy(token.lexeme, ":");
            break;
        default:
            fprintf(stderr, "Error at line %d, column %d: Invalid character '%c'\n", 
                    token.line, token.column, current);
            lexer->has_error = true;
            token.type = TOKEN_ERROR;
            strcpy(token.lexeme, &current);
            token.lexeme[1] = '\0';
    }
    
    return token;
}

// 查找保留字
TokenType lookup_keyword(const char* lexeme) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(keywords[i].word, lexeme) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

// Token类型转字符串
const char* token_type_to_str(TokenType type) {
    switch (type) {
        // 保留字
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_DO: return "DO";
        case TOKEN_MAIN: return "MAIN";
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_DOUBLE: return "DOUBLE";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_CONST: return "CONST";
        case TOKEN_VOID: return "VOID";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_UNSIGNED: return "UNSIGNED";
        case TOKEN_ENUM: return "ENUM";
        case TOKEN_LONG: return "LONG";
        case TOKEN_SWITCH: return "SWITCH";
        case TOKEN_CASE: return "CASE";
        case TOKEN_AUTO: return "AUTO";
        case TOKEN_STATIC: return "STATIC";
        
        // 特殊符号
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_MULTIPLY: return "MULTIPLY";
        case TOKEN_DIVIDE: return "DIVIDE";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LE: return "LE";
        case TOKEN_GE: return "GE";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NE: return "NE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_QUOTE: return "QUOTE";
        case TOKEN_DQUOTE: return "DQUOTE";
        case TOKEN_DOT: return "DOT";
        case TOKEN_COLON: return "COLON";
        
        // 其他
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT_NUM: return "FLOAT_NUM";
        case TOKEN_HEX: return "HEX";
        case TOKEN_OCTAL: return "OCTAL";
        case TOKEN_CHAR_CONST: return "CHAR_CONST";
        case TOKEN_STRING_CONST: return "STRING_CONST";
        
        // 特殊
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        
        default: return "UNKNOWN";
    }
}