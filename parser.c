#include"parser.h"
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

//NOTE -  全局解析器状态
static Lexer* lexer = NULL;
static Token lookahead;
static bool parse_error = false;

//NOTE - 用于打印步骤的相关信息
static int indent_level = 0;
static void indent_inc(void){
    indent_level++;
}
static void indent_dec(void){
    if(indent_level>0) indent_level--;
}
static void print_indent(void){
    for(int i =0;i<indent_level;i++)
    putchar(' ');
}
static void trace_enter(const char* name) {
    print_indent(); 
    printf("Enter: %s\n", name);
    indent_inc();
}
static void trace_exit(const char* name) {
    indent_dec();
    print_indent(); 
    printf("Exit : %s\n", name);
}
static void trace_match(TokenType t, const char* lexeme) {
    print_indent(); 
    printf("Match: %s -> '%s'\n", token_type_to_str(t), lexeme);
}
static void trace_token(const char* note, Token t) {
    print_indent(); 
    printf("%s: %s ('%s') at %d:%d\n", note, token_type_to_str(t.type), t.lexeme, t.line, t.column);
}

//NOTE - 前向声明，非终结符对应一个函数
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


//TODO - 想后扫描
static void advance_token(void){
    lookahead = get_token(lexer);
}
//TODO - 判断是否与期望字符一致
static void match(TokenType expected){
    if(lookahead.type == expected){
        trace_match(lookahead.type,lookahead.lexeme);
        advance_token();
    }else{
        print_indent();
        fprintf(stderr, "Syntax error at line %d, col %d: expected %s but found %s ('%s')\n",
                lookahead.line, lookahead.column,
                token_type_to_str(expected),
                token_type_to_str(lookahead.type),
                lookahead.lexeme);
        parse_error = true;
        TokenType syncset[] = {TOKEN_SEMICOLON,TOKEN_RBRACE,TOKEN_EOF};
        sync_on(syncset,sizeof(syncset)/sizeof(syncset[0]));
        if(lookahead.type != TOKEN_EOF)
            advance_token();
    }
}
//TODO - 同步：跳转到set中的token或EOF。
static void sync_on(TokenType set[],int set_len){
    bool found = false;
    while(lookahead.type != TOKEN_EOF){
        for(int i = 0;i<set_len;i++){
            if(lookahead.type = set[i]){
                found = true;
                break;
            }
        }
        if(found) break;
        advance_token();
    }
}
//TODO - 判断给定的是否为数字
static bool is_number_token(TokenType t){
    return t == TOKEN_INTEGER||t == TOKEN_FLOAT_NUM||t == TOKEN_HEX||t == TOKEN_OCTAL;
}

//NOTE - 以下为语法函数的实现：

//TODO - program->block
static void program(void){
    trace_enter("program");
    block();
    if(lookahead.type != TOKEN_EOF){
        print_indent();
        fprintf(stderr, "Warning: extra tokens after program end at line %d\n", lookahead.line);
    }
}
//TODO - block -> '{' stmts '}'
static void block(void){
    trace_enter("block");
    if(lookahead.type == TOKEN_LBRACE){
        match(TOKEN_LBRACE);
        stmts();
        match(TOKEN_RBRACE);
    }else{
        print_indent();
        fprintf(stderr, "Syntax error: expected '{' at line %d\n", lookahead.line);
        parse_error = true;
        TokenType syncset[] = {TOKEN_RBRACE,TOKEN_EOF};
        sync_on(syncset,sizeof(syncset)/sizeof(syncset[0]));
        if(lookahead.type == TOKEN_RBRACE) advance_token();
    }
    trace_exit("block");
}
//TODO - stmts -> stmt stmts | ε
static void stmts(void){
    trace_enter("stmts");
    for(;;){
        if (lookahead.type == TOKEN_IDENTIFIER ||
            lookahead.type == TOKEN_IF ||
            lookahead.type == TOKEN_WHILE ||
            lookahead.type == TOKEN_DO ||
            lookahead.type == TOKEN_BREAK ||
            lookahead.type == TOKEN_LBRACE){
                stmt();
            }
        else{
            print_indent();
            printf("stmts -> epsilon\n");
            break;
        }    
    }
    trace_exit("stmts");
}
//TODO - stmt -> id = expr ; | if ( bool ) stmt [ else stmt ] | while ( bool ) stmt | do stmt while ( bool ) ; | break ; | block
static void stmt(void){
    trace_enter("stmt");
    if(lookahead.type == TOKEN_IDENTIFIER){
        assignment_stmt();
    }else if(lookahead.type == TOKEN_IF){
        if_stmt();
    }else if(lookahead.type == TOKEN_WHILE){
        while_stmt();
    }else if(lookahead.type == TOKEN_DO){
        do_while_stmt();
    }else if(lookahead.type == TOKEN_BREAK){
        break_stmt();
    }else if(lookahead.type == TOKEN_LBRACE){
        block();
    }else{
        print_indent();
        fprintf(stderr, "Syntax error: unexpected token %s ('%s') at line %d in stmt\n",
                token_type_to_str(lookahead.type), lookahead.lexeme, lookahead.line);
        parse_error = true;
        TokenType syncset[] = { TOKEN_SEMICOLON, TOKEN_RBRACE, TOKEN_EOF };
        sync_on(syncset, sizeof(syncset)/sizeof(syncset[0]));
        if (lookahead.type != TOKEN_EOF) advance_token();
    }
    trace_exit("stmt");
}
//TODO - assignment_stmt -> id = expr ;
static void assignment_stmt(void){
    trace_enter("assignment_stmt");
    trace_token("LHS identifier",lookahead);
    match(TOKEN_IDENTIFIER);
    match(TOKEN_ASSIGN);
    expr();
    match(TOKEN_SEMICOLON);
    trace_exit("assignment_stmt");
}
//TODO - if_stmt -> if '(' bool ')' stmt [ else stmt ]
static void if_stmt(void){
    trace_enter("if_stmt");
    match(TOKEN_IF);
    match(TOKEN_LPAREN);
    bool_expr();
    match(TOKEN_RPAREN);
    stmt();
    if(lookahead.type == TOKEN_ELSE){
        match(TOKEN_ELSE);
        stmt();
    }else{
        print_indent();
        printf("if_stmt -> no else\n");
    }
    trace_exit("if_stmt");
}
//TODO - while_stmt -> while '(' bool ')' stmt
static void while_stmt(void) {
    trace_enter("while_stmt");
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    bool_expr();
    match(TOKEN_RPAREN);
    stmt();
    trace_exit("while_stmt");
}
//TODO - do_while_stmt -> do stmt while '(' bool ')' ;
static void do_while_stmt(void){
    trace_enter("do_while_stmt");
    match(TOKEN_DO);
    stmt();
    match(TOKEN_WHILE);
    match(TOKEN_LPAREN);
    bool_expr();
    match(TOKEN_RPAREN);
    match(TOKEN_SEMICOLON);
    trace_exit("do_while_stmt");
}
//TODO - break_stmt -> break ;
static void break_stmt(void){
    trace_enter("break_stmt");
    match(TOKEN_BREAK);
    match(TOKEN_SEMICOLON);
    trace_exit("break_stmt");
}
//TODO - expr  -> term expr'
static void expr(void) {
    trace_enter("expr");
    term();
    expr_prime();
    trace_exit("expr");
}
//TODO - expr' -> '+' term expr' | '-' term expr' | ε
static void expr_prime(void){
    trace_enter("expr_prime");
    if(lookahead.type == TOKEN_PLUS||lookahead.type == TOKEN_MINUS){
        while(lookahead.type == TOKEN_PLUS || lookahead.type == TOKEN_MINUS){
        if(lookahead.type == TOKEN_PLUS){
            match(TOKEN_PLUS);
            term();
        }else if(lookahead.type == TOKEN_MINUS){
            match(TOKEN_MINUS);
            term();
        }
    }
    }else{
        print_indent();
        printf("expr -> epsilon\n");
    }
    trace_exit("expr");
    
}
//TODO - term -> factor term'
static void term(void){
    trace_enter("term");
    factor();
    term_prime();
    trace_exit("term");
}
//TODO - term' -> '*' factor term' | '/' factor term' | ε
static void term_prime(void) {
    trace_enter("term_prime");
    if (lookahead.type == TOKEN_MULTIPLY || lookahead.type == TOKEN_DIVIDE) {
        while (lookahead.type == TOKEN_MULTIPLY || lookahead.type == TOKEN_DIVIDE) {
            if (lookahead.type == TOKEN_MULTIPLY) {
                match(TOKEN_MULTIPLY);
                factor();
            } else {
                match(TOKEN_DIVIDE);
                factor();
            }
        }
    } else {
        print_indent(); printf("term_prime -> epsilon\n");
    }
    trace_exit("term_prime");
}
//TODO - factor -> '(' expr ')' | id | num
static void factor(void) {
    trace_enter("factor");
    if (lookahead.type == TOKEN_LPAREN) {
        match(TOKEN_LPAREN);
        expr();
        match(TOKEN_RPAREN);
    } else if (lookahead.type == TOKEN_IDENTIFIER) {
        trace_token("factor -> id", lookahead);
        match(TOKEN_IDENTIFIER);
    } else if (is_number_token(lookahead.type)) {
        trace_token("factor -> number", lookahead);
        advance_token();
    } else {
        print_indent();
        fprintf(stderr, "Syntax error: expected factor at line %d, found %s ('%s')\n",
                lookahead.line, token_type_to_str(lookahead.type), lookahead.lexeme);
        parse_error = true;
        TokenType syncset[] = { TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,
                                TOKEN_SEMICOLON, TOKEN_RPAREN, TOKEN_RBRACE, TOKEN_EOF };
        sync_on(syncset, sizeof(syncset)/sizeof(syncset[0]));
        if (lookahead.type != TOKEN_EOF) advance_token();
    }
    trace_exit("factor");
}

//TODO - bool -> expr bool_rest
static void bool_expr(void) {
    trace_enter("bool_expr");
    expr();
    bool_rest();
    trace_exit("bool_expr");
}
//TODO - bool_rest -> < expr | <= expr | > expr | >= expr | == expr | != expr | ε
static void bool_rest(void) {
    trace_enter("bool_rest");
    if (lookahead.type == TOKEN_LT) {
        match(TOKEN_LT); expr();
    } else if (lookahead.type == TOKEN_LE) {
        match(TOKEN_LE); expr();
    } else if (lookahead.type == TOKEN_GT) {
        match(TOKEN_GT); expr();
    } else if (lookahead.type == TOKEN_GE) {
        match(TOKEN_GE); expr();
    } else if (lookahead.type == TOKEN_EQ) {
        match(TOKEN_EQ); expr();
    } else if (lookahead.type == TOKEN_NE) {
        match(TOKEN_NE); expr();
    } else {
        print_indent(); printf("bool_rest -> epsilon\n");
    }
    trace_exit("bool_rest");
}

//NOTE - 对外解析函数
int parse_file(const char* filename){
    parse_error = false;
    indent_level = 0;
    if(lexer){
        free_lexer(lexer);
        lexer = NULL;
    }
    lexer = init_lexer(filename);
    if(!lexer){
        fprintf(stderr, "Failed to open source file: %s\n", filename);
        return 1;
    }
    //读入第一个token
    advance_token();
    // 开始解析
    program();

    if(lookahead.type != TOKEN_EOF){
        while (lookahead.type != TOKEN_EOF) advance_token();
    }

    free_lexer(lexer);
    lexer = NULL;
    if(parse_error){
        fprintf(stderr, "Parsing finished: syntax errors detected.\n");
        return 2;
    }else{
        printf("Parsing finished: no syntax errors detected.\n");
        return 0;
    }
}