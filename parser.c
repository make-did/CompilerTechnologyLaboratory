#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

// NOTE - 全局解析器状态
static Lexer* lexer = NULL;
static Token lookahead;
static bool parse_error = false;

// NOTE - 用于记录推导过程的二维数组
#define MAX_STEPS 200
#define MAX_STEP_LEN 256
static char steps[MAX_STEPS][MAX_STEP_LEN];
static int step_count = 0;

// 记录当前推导状态
static char current_derivation[MAX_STEP_LEN] = "program";

// 保存当前步骤
static void save_step(void) {
    if (step_count < MAX_STEPS) {
        strcpy(steps[step_count], current_derivation);
        step_count++;
    }
}

// 替换推导式中的非终结符
static void replace_nonterminal(const char* nonterm, const char* replacement) {
    char temp[MAX_STEP_LEN] = "";
    char* pos = strstr(current_derivation, nonterm);
    
    if (pos) {
        // 复制前半部分
        int len = pos - current_derivation;
        strncpy(temp, current_derivation, len);
        temp[len] = '\0';
        
        // 添加替换字符串
        strcat(temp, replacement);
        
        // 添加后半部分
        strcat(temp, pos + strlen(nonterm));
        
        // 更新当前推导
        strcpy(current_derivation, temp);
        
        // 保存步骤
        save_step();
    }
}

// 检查并删除尾部的非终结符
static void cleanup_nonterminal(const char* nonterm) {
    char* pos = strstr(current_derivation, nonterm);
    while (pos) {
        char temp[MAX_STEP_LEN];
        strncpy(temp, current_derivation, pos - current_derivation);
        temp[pos - current_derivation] = '\0';
        strcat(temp, pos + strlen(nonterm));
        strcpy(current_derivation, temp);
        
        // 更新位置
        pos = strstr(current_derivation, nonterm);
    }
}

// 前向声明
static void advance_token(void);
static void match(TokenType expected);

static void program(void);
static void block(void);
static void stmts(void);
static void stmt(void);

static void assignment_stmt(void);
static void if_stmt(void);
static void while_stmt(void);
static void do_while_stmt(void);
static void break_stmt(void);

static void expr(void);
static void expr_prime(void);
static void term(void);
static void term_prime(void);
static void factor(void);

static void bool_expr(void);
static void bool_rest(void);

// TODO - 词法单元前进
static void advance_token(void) {
    lookahead = get_token(lexer);
}

// TODO - 匹配期望的词法单元
static void match(TokenType expected) {
    if (lookahead.type == expected) {
        advance_token();
    } else {
        fprintf(stderr, "Syntax error at line %d, col %d: expected %s but found %s ('%s')\n",
                lookahead.line, lookahead.column,
                token_type_to_str(expected),
                token_type_to_str(lookahead.type),
                lookahead.lexeme);
        parse_error = true;
    }
}

// NOTE - 以下为语法函数的实现：

// TODO - program -> block
static void program(void) {
    // 保存初始状态
    save_step();
    
    replace_nonterminal("program", "block");
    
    block();
    
    // 清理可能的剩余非终结符
    cleanup_nonterminal("stmts");
    cleanup_nonterminal("stmt");
    
    if (lookahead.type != TOKEN_EOF) {
        fprintf(stderr, "Warning: extra tokens after program end at line %d\n", lookahead.line);
    }
}

// TODO - block -> '{' stmts '}'
static void block(void) {
    // 保存当前的推导状态
    char saved_derivation[MAX_STEP_LEN];
    strcpy(saved_derivation, current_derivation);
    
    // 先替换为block
    replace_nonterminal("block", "{ stmts }");
    
    if (lookahead.type == TOKEN_LBRACE) {
        match(TOKEN_LBRACE);
        stmts();
        match(TOKEN_RBRACE);
    } else {
        fprintf(stderr, "Syntax error: expected '{' at line %d\n", lookahead.line);
        parse_error = true;
    }
}

// TODO - stmts -> stmt stmts | ε
static void stmts(void) {
    // 检查是否应该应用 ε 产生式
    if (lookahead.type == TOKEN_RBRACE) {
        // ε 产生式：从推导式中删除 stmts
        char* pos = strstr(current_derivation, "stmts");
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, pos + 5); // 5是"stmts"的长度
            strcpy(current_derivation, temp);
            save_step();
        }
        return;
    }
    
    replace_nonterminal("stmts", "stmt stmts");
    
    stmt();
    stmts();
}

// TODO - stmt -> id = expr ; | if ( bool ) stmt [ else stmt ] | while ( bool ) stmt | do stmt while ( bool ) ; | break ; | block
static void stmt(void) {
    if (lookahead.type == TOKEN_IDENTIFIER) {
        assignment_stmt();
    } else if (lookahead.type == TOKEN_IF) {
        if_stmt();
    } else if (lookahead.type == TOKEN_WHILE) {
        while_stmt();
    } else if (lookahead.type == TOKEN_DO) {
        do_while_stmt();
    } else if (lookahead.type == TOKEN_BREAK) {
        break_stmt();
    } else if (lookahead.type == TOKEN_LBRACE) {
        // 这里直接调用block，因为block函数会处理替换
        block();
    } else {
        fprintf(stderr, "Syntax error: unexpected token %s ('%s') at line %d in stmt\n",
                token_type_to_str(lookahead.type), lookahead.lexeme, lookahead.line);
        parse_error = true;
    }
}

// TODO - assignment_stmt -> id = expr ;
static void assignment_stmt(void) {
    replace_nonterminal("stmt", "id = expr ;");
    
    // 匹配标识符
    match(TOKEN_IDENTIFIER);
    
    // 匹配赋值符号
    match(TOKEN_ASSIGN);
    
    // 处理表达式
    expr();
    
    // 匹配分号
    match(TOKEN_SEMICOLON);
}

// TODO - while_stmt -> while '(' bool ')' stmt
static void while_stmt(void) {
    // 保存当前推导式，以便之后替换stmt
    char saved_derivation[MAX_STEP_LEN];
    strcpy(saved_derivation, current_derivation);
    
    replace_nonterminal("stmt", "while ( bool ) stmt");
    
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    
    bool_expr();
    
    match(TOKEN_RPAREN);
    
    // 继续解析 while 循环体
    // 这里需要判断循环体是block还是单个stmt
    if (lookahead.type == TOKEN_LBRACE) {
        // 循环体是block
        // 先替换stmt为block
        char* pos = strstr(current_derivation, "stmt");
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, "block");
            strcat(temp, pos + 4);
            strcpy(current_derivation, temp);
            save_step();
        }
        
        // 然后解析block
        block();
    } else {
        // 循环体是单个stmt
        stmt();
    }
}

// TODO - if_stmt -> if '(' bool ')' stmt [ else stmt ]
static void if_stmt(void) {
    replace_nonterminal("stmt", "if ( bool ) stmt");
    
    match(TOKEN_IF);
    match(TOKEN_LPAREN);
    
    bool_expr();
    
    match(TOKEN_RPAREN);
    
    if (lookahead.type == TOKEN_ELSE) {
        // 更新推导式
        char* pos = strstr(current_derivation, "stmt");
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, "stmt else stmt");
            strcat(temp, pos + 4);
            strcpy(current_derivation, temp);
            save_step();
        }
        
        stmt();
        match(TOKEN_ELSE);
        stmt();
    } else {
        stmt();
    }
}

// TODO - do_while_stmt -> do stmt while '(' bool ')' ;
static void do_while_stmt(void) {
    replace_nonterminal("stmt", "do stmt while ( bool ) ;");
    
    match(TOKEN_DO);
    stmt();
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    bool_expr();
    match(TOKEN_RPAREN);
    match(TOKEN_SEMICOLON);
}

// TODO - break_stmt -> break ;
static void break_stmt(void) {
    replace_nonterminal("stmt", "break ;");
    
    match(TOKEN_BREAK);
    match(TOKEN_SEMICOLON);
}

// 表达式处理函数
static void expr(void) {
    replace_nonterminal("expr", "term expr'");
    
    term();
    expr_prime();
}

static void expr_prime(void) {
    if (lookahead.type == TOKEN_PLUS) {
        replace_nonterminal("expr'", "+ term expr'");
        
        match(TOKEN_PLUS);
        term();
        expr_prime();
    } else if (lookahead.type == TOKEN_MINUS) {
        replace_nonterminal("expr'", "- term expr'");
        
        match(TOKEN_MINUS);
        term();
        expr_prime();
    } else {
        // ε 产生式：删除 expr'
        char* pos = strstr(current_derivation, "expr'");
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, pos + 5); // 5是"expr'"的长度
            strcpy(current_derivation, temp);
            save_step();
        }
    }
}

static void term(void) {
    replace_nonterminal("term", "factor term'");
    
    factor();
    term_prime();
}

static void term_prime(void) {
    if (lookahead.type == TOKEN_MULTIPLY) {
        replace_nonterminal("term'", "* factor term'");
        
        match(TOKEN_MULTIPLY);
        factor();
        term_prime();
    } else if (lookahead.type == TOKEN_DIVIDE) {
        replace_nonterminal("term'", "/ factor term'");
        
        match(TOKEN_DIVIDE);
        factor();
        term_prime();
    } else {
        // ε 产生式：删除 term'
        char* pos = strstr(current_derivation, "term'");
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, pos + 5); // 5是"term'"的长度
            strcpy(current_derivation, temp);
            save_step();
        }
    }
}

static void factor(void) {
    if (lookahead.type == TOKEN_LPAREN) {
        replace_nonterminal("factor", "( expr )");
        match(TOKEN_LPAREN);
        expr();
        match(TOKEN_RPAREN);
    } else if (lookahead.type == TOKEN_IDENTIFIER) {
        replace_nonterminal("factor", "id");
        match(TOKEN_IDENTIFIER);
    } else if (lookahead.type == TOKEN_INTEGER) {
        replace_nonterminal("factor", "num");
        match(TOKEN_INTEGER);
    } else {
        fprintf(stderr, "Syntax error: expected factor at line %d, found %s ('%s')\n",
                lookahead.line, token_type_to_str(lookahead.type), lookahead.lexeme);
        parse_error = true;
    }
}

static void bool_expr(void) {
    replace_nonterminal("bool", "expr bool_rest");
    
    expr();
    bool_rest();
}

static void bool_rest(void) {
    if (lookahead.type == TOKEN_LT) {
        replace_nonterminal("bool_rest", "< expr");
        match(TOKEN_LT);
        expr();
    } else if (lookahead.type == TOKEN_LE) {
        replace_nonterminal("bool_rest", "<= expr");
        match(TOKEN_LE);
        expr();
    } else if (lookahead.type == TOKEN_GT) {
        replace_nonterminal("bool_rest", "> expr");
        match(TOKEN_GT);
        expr();
    } else if (lookahead.type == TOKEN_GE) {
        replace_nonterminal("bool_rest", ">= expr");
        match(TOKEN_GE);
        expr();
    } else if (lookahead.type == TOKEN_EQ) {
        replace_nonterminal("bool_rest", "== expr");
        match(TOKEN_EQ);
        expr();
    } else if (lookahead.type == TOKEN_NE) {
        replace_nonterminal("bool_rest", "!= expr");
        match(TOKEN_NE);
        expr();
    } else {
        // ε 产生式：删除 bool_rest
        char* pos = strstr(current_derivation, "bool_rest");
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, pos + 9); // 9是"bool_rest"的长度
            strcpy(current_derivation, temp);
            save_step();
        }
    }
}

// 打印所有步骤
static void print_all_steps(void) {
    printf("Derivation steps:\n");
    for (int i = 0; i < step_count; i++) {
        printf("%3d: %s\n", i + 1, steps[i]);
    }
    printf("\n");
}

// NOTE - 对外解析函数
int parse_file(const char* filename) {
    parse_error = false;
    step_count = 0;
    strcpy(current_derivation, "program");
    
    if (lexer) {
        free_lexer(lexer);
        lexer = NULL;
    }
    
    lexer = init_lexer(filename);
    if (!lexer) {
        fprintf(stderr, "Failed to open source file: %s\n", filename);
        return 1;
    }
    
    // 读入第一个token
    advance_token();
    // 开始解析
    program();
    
    if (lookahead.type != TOKEN_EOF) {
        while (lookahead.type != TOKEN_EOF) advance_token();
    }
    
    free_lexer(lexer);
    lexer = NULL;
    
    // 打印所有推导步骤
    print_all_steps();
    
    if (parse_error) {
        fprintf(stderr, "Parsing finished: syntax errors detected.\n");
        return 2;
    } else {
        printf("Parsing finished: no syntax errors detected.\n");
        return 0;
    }
}