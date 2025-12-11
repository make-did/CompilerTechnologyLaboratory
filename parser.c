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
static int current_depth = 0;

// 记录当前推导状态
static char current_derivation[MAX_STEP_LEN] = "program";

// 保存当前步骤
static void save_step(void) {
    if (step_count < MAX_STEPS) {
        // 添加适当的缩进来显示推导层级
        char formatted_step[MAX_STEP_LEN];
        int indent = current_depth * 2;
        
        // 创建带缩进的步骤
        int i;
        for (i = 0; i < indent && i < MAX_STEP_LEN-1; i++) {
            formatted_step[i] = ' ';
        }
        formatted_step[i] = '\0';
        
        // 添加推导式
        strcat(formatted_step, current_derivation);
        strcpy(steps[step_count], formatted_step);
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

// 匹配并替换终结符
static void match_and_replace(TokenType expected, const char* term_name) {
    if (lookahead.type == expected) {
        // 在推导式中替换终结符
        char* pos = strstr(current_derivation, term_name);
        if (pos) {
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, lookahead.lexeme);
            strcat(temp, pos + strlen(term_name));
            strcpy(current_derivation, temp);
            save_step();
        }
        
        // 前进到下一个token
        lookahead = get_token(lexer);
    } else {
        fprintf(stderr, "Syntax error at line %d, col %d: expected %s but found %s ('%s')\n",
                lookahead.line, lookahead.column,
                token_type_to_str(expected),
                token_type_to_str(lookahead.type),
                lookahead.lexeme);
        parse_error = true;
    }
}

// 前向声明
static void advance_token(void);
static void match(TokenType expected);
static void sync_on(TokenType set[], int set_len);

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

// TODO - 匹配期望的词法单元（简单版本，不输出推导）
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

// TODO - 同步恢复
static void sync_on(TokenType set[], int set_len) {
    bool found = false;
    while (lookahead.type != TOKEN_EOF) {
        for (int i = 0; i < set_len; i++) {
            if (lookahead.type == set[i]) {
                found = true;
                break;
            }
        }
        if (found) break;
        advance_token();
    }
}

// NOTE - 以下为语法函数的实现：

// TODO - program -> block
static void program(void) {
    // 保存初始状态
    save_step();
    
    current_depth++;
    replace_nonterminal("program", "block");
    
    block();
    
    current_depth--;
    
    if (lookahead.type != TOKEN_EOF) {
        fprintf(stderr, "Warning: extra tokens after program end at line %d\n", lookahead.line);
    }
}

// TODO - block -> '{' stmts '}'
static void block(void) {
    current_depth++;
    replace_nonterminal("block", "{ stmts }");
    
    if (lookahead.type == TOKEN_LBRACE) {
        match(TOKEN_LBRACE);
        stmts();
        match(TOKEN_RBRACE);
    } else {
        fprintf(stderr, "Syntax error: expected '{' at line %d\n", lookahead.line);
        parse_error = true;
    }
    
    current_depth--;
}

// TODO - stmts -> stmt stmts | ε
static void stmts(void) {
    // 检查是否应该应用 ε 产生式
    if (lookahead.type == TOKEN_RBRACE) {
        // ε 产生式，直接删除 stmts
        char* pos = strstr(current_derivation, "stmts");
        if (pos) {
            // 删除 stmts（包括前面的空格）
            char temp[MAX_STEP_LEN];
            char* start = pos;
            
            // 向前找空格
            while (start > current_derivation && *(start-1) == ' ') {
                start--;
            }
            
            strncpy(temp, current_derivation, start - current_derivation);
            temp[start - current_derivation] = '\0';
            strcat(temp, pos + 5); // 5是"stmts"的长度
            
            strcpy(current_derivation, temp);
            save_step();
        }
        return;
    }
    
    current_depth++;
    replace_nonterminal("stmts", "stmt stmts");
    
    stmt();
    stmts();
    
    current_depth--;
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
        block();
    } else {
        fprintf(stderr, "Syntax error: unexpected token %s ('%s') at line %d in stmt\n",
                token_type_to_str(lookahead.type), lookahead.lexeme, lookahead.line);
        parse_error = true;
    }
}

// TODO - assignment_stmt -> id = expr ;
static void assignment_stmt(void) {
    current_depth++;
    replace_nonterminal("stmt", "id = expr ;");
    
    // 匹配标识符
    match_and_replace(TOKEN_IDENTIFIER, "id");
    
    // 匹配赋值符号
    match(TOKEN_ASSIGN);
    
    // 处理表达式
    expr();
    
    // 匹配分号
    match(TOKEN_SEMICOLON);
    
    current_depth--;
}

// TODO - while_stmt -> while '(' bool ')' stmt
static void while_stmt(void) {
    current_depth++;
    replace_nonterminal("stmt", "while ( bool ) stmt");
    
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    
    bool_expr();
    
    match(TOKEN_RPAREN);
    
    // 继续解析 while 循环体
    stmt();
    
    current_depth--;
}

// TODO - if_stmt -> if '(' bool ')' stmt [ else stmt ]
static void if_stmt(void) {
    current_depth++;
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
    
    current_depth--;
}

// TODO - do_while_stmt -> do stmt while '(' bool ')' ;
static void do_while_stmt(void) {
    current_depth++;
    replace_nonterminal("stmt", "do stmt while ( bool ) ;");
    
    match(TOKEN_DO);
    stmt();
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    bool_expr();
    match(TOKEN_RPAREN);
    match(TOKEN_SEMICOLON);
    
    current_depth--;
}

// TODO - break_stmt -> break ;
static void break_stmt(void) {
    current_depth++;
    replace_nonterminal("stmt", "break ;");
    
    match(TOKEN_BREAK);
    match(TOKEN_SEMICOLON);
    
    current_depth--;
}

// 表达式处理函数
static void expr(void) {
    current_depth++;
    replace_nonterminal("expr", "term expr'");
    
    term();
    expr_prime();
    
    // 清理 expr'
    char* pos = strstr(current_derivation, "expr'");
    if (pos && strncmp(pos, "expr'", 5) == 0) {
        // 检查 expr' 前面是否有空格
        char* check_pos = pos - 1;
        if (check_pos >= current_derivation && *check_pos == ' ') {
            // 删除空格和 expr'
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, check_pos - current_derivation);
            temp[check_pos - current_derivation] = '\0';
            strcat(temp, pos + 5);
            strcpy(current_derivation, temp);
            save_step();
        } else {
            // 只删除 expr'
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, pos + 5);
            strcpy(current_derivation, temp);
            save_step();
        }
    }
    
    current_depth--;
}

static void expr_prime(void) {
    if (lookahead.type == TOKEN_PLUS) {
        current_depth++;
        replace_nonterminal("expr'", "+ term expr'");
        
        match(TOKEN_PLUS);
        term();
        expr_prime();
        current_depth--;
    } else if (lookahead.type == TOKEN_MINUS) {
        current_depth++;
        replace_nonterminal("expr'", "- term expr'");
        
        match(TOKEN_MINUS);
        term();
        expr_prime();
        current_depth--;
    }
    // ε 产生式：不显示
}

static void term(void) {
    current_depth++;
    replace_nonterminal("term", "factor term'");
    
    factor();
    term_prime();
    
    // 清理 term'
    char* pos = strstr(current_derivation, "term'");
    if (pos && strncmp(pos, "term'", 5) == 0) {
        // 检查 term' 前面是否有空格
        char* check_pos = pos - 1;
        if (check_pos >= current_derivation && *check_pos == ' ') {
            // 删除空格和 term'
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, check_pos - current_derivation);
            temp[check_pos - current_derivation] = '\0';
            strcat(temp, pos + 5);
            strcpy(current_derivation, temp);
            save_step();
        } else {
            // 只删除 term'
            char temp[MAX_STEP_LEN];
            strncpy(temp, current_derivation, pos - current_derivation);
            temp[pos - current_derivation] = '\0';
            strcat(temp, pos + 5);
            strcpy(current_derivation, temp);
            save_step();
        }
    }
    
    current_depth--;
}

static void term_prime(void) {
    if (lookahead.type == TOKEN_MULTIPLY) {
        current_depth++;
        replace_nonterminal("term'", "* factor term'");
        
        match(TOKEN_MULTIPLY);
        factor();
        term_prime();
        current_depth--;
    } else if (lookahead.type == TOKEN_DIVIDE) {
        current_depth++;
        replace_nonterminal("term'", "/ factor term'");
        
        match(TOKEN_DIVIDE);
        factor();
        term_prime();
        current_depth--;
    }
    // ε 产生式：不显示
}

static void factor(void) {
    current_depth++;
    
    if (lookahead.type == TOKEN_LPAREN) {
        replace_nonterminal("factor", "( expr )");
        match(TOKEN_LPAREN);
        expr();
        match(TOKEN_RPAREN);
    } else if (lookahead.type == TOKEN_IDENTIFIER) {
        replace_nonterminal("factor", "id");
        match_and_replace(TOKEN_IDENTIFIER, "id");
    } else if (lookahead.type == TOKEN_INTEGER) {
        replace_nonterminal("factor", "num");
        match_and_replace(TOKEN_INTEGER, "num");
    } else {
        fprintf(stderr, "Syntax error: expected factor at line %d, found %s ('%s')\n",
                lookahead.line, token_type_to_str(lookahead.type), lookahead.lexeme);
        parse_error = true;
    }
    
    current_depth--;
}

static void bool_expr(void) {
    current_depth++;
    replace_nonterminal("bool", "expr bool_rest");
    
    expr();
    bool_rest();
    
    current_depth--;
}

static void bool_rest(void) {
    if (lookahead.type == TOKEN_LT) {
        current_depth++;
        replace_nonterminal("bool_rest", "< expr");
        match(TOKEN_LT);
        expr();
        current_depth--;
    } else if (lookahead.type == TOKEN_LE) {
        current_depth++;
        replace_nonterminal("bool_rest", "<= expr");
        match(TOKEN_LE);
        expr();
        current_depth--;
    } else if (lookahead.type == TOKEN_GT) {
        current_depth++;
        replace_nonterminal("bool_rest", "> expr");
        match(TOKEN_GT);
        expr();
        current_depth--;
    } else if (lookahead.type == TOKEN_GE) {
        current_depth++;
        replace_nonterminal("bool_rest", ">= expr");
        match(TOKEN_GE);
        expr();
        current_depth--;
    } else if (lookahead.type == TOKEN_EQ) {
        current_depth++;
        replace_nonterminal("bool_rest", "== expr");
        match(TOKEN_EQ);
        expr();
        current_depth--;
    } else if (lookahead.type == TOKEN_NE) {
        current_depth++;
        replace_nonterminal("bool_rest", "!= expr");
        match(TOKEN_NE);
        expr();
        current_depth--;
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
    current_depth = 0;
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