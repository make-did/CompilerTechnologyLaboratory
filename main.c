//词法分析器运行主函数

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

// 函数声明
void print_source_with_line_numbers(const char* filename);
void print_binary_form_per_line(const char* filename);
void print_error_summary(Lexer* lexer);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        fprintf(stderr, "Example: %s test.c\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    
    printf("========== Lexical Analyzer ==========\n");
    printf("File: %s\n\n", filename);
    
    // 功能1：显示带行号的源程序
    printf("=== Source Code with Line Numbers ===\n");
    print_source_with_line_numbers(filename);
    printf("\n");
    
    // 功能2：打印每行包含的记号的二元形式
    printf("=== Binary Forms (Token Type, Value) per Line ===\n");
    print_binary_form_per_line(filename);
    printf("\n");
    
    // 功能3：错误统计
    Lexer* lexer = init_lexer(filename);
    if (lexer) {
        print_error_summary(lexer);
        free_lexer(lexer);
    }
    
    return 0;
}

// 显示带行号的源程序
void print_source_with_line_numbers(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return;
    }
    
    char buffer[1024];
    int line_num = 1;
    
    while (fgets(buffer, sizeof(buffer), file)) {
        // 移除末尾的换行符
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        if (len > 1 && buffer[len-2] == '\r') {  // Windows换行符
            buffer[len-2] = '\0';
        }
        
        // 打印行号和内容
        printf("%4d: %s\n", line_num, buffer);
        line_num++;
    }
    
    fclose(file);
}

// 打印每行包含的记号的二元形式
void print_binary_form_per_line(const char* filename) {
    Lexer* lexer = init_lexer(filename);
    if (!lexer) {
        fprintf(stderr, "Error: Cannot initialize lexer\n");
        return;
    }
    
    int current_line = 0;
    int token_count = 0;
    int error_count = 0;
    
    printf("Line | Binary Forms\n");
    printf("-----|-----------------------------------------------------\n");
    
    // 先收集所有token，按行分组
    Token tokens[1000];
    int token_index = 0;
    
    // 收集token
    while (1) {
        Token token = get_token(lexer);
        
        if (token.type == TOKEN_EOF) {
            break;
        }
        
        if (token.type == TOKEN_ERROR) {
            error_count++;
        }
        
        if (token_index < 1000) {
            tokens[token_index++] = token;
        }
        
        token_count++;
        if (token_count > 500) break; // 防止无限循环
    }
    
    // 按行打印二元式 - 使用属性名称
    for (int i = 0; i < token_index; i++) {
        Token token = tokens[i];
        
        // 开始新行
        if (token.line != current_line) {
            if (current_line > 0) {
                printf("\n");
            }
            printf("%4d | ", token.line);
            current_line = token.line;
        }
        
        printf("(%s, %s) ", token_type_to_str(token.type), token.lexeme);
    }
    
    printf("\n\n");
    printf("Total tokens: %d\n", token_count);
    printf("Total errors: %d\n", error_count);
    
    free_lexer(lexer);
}

// 打印错误摘要
void print_error_summary(Lexer* lexer) {
    if (!lexer) return;
    
    // 重新分析以收集错误信息
    int error_count = 0;
    int line_errors[100] = {0};
    
    // 重置词法分析器
    fseek(lexer->source, 0, SEEK_SET);
    lexer->line = 1;
    lexer->column = 0;
    lexer->has_error = false;
    lexer->current_char = fgetc(lexer->source);
    
    // 收集错误
    while (1) {
        Token token = get_token(lexer);
        
        if (token.type == TOKEN_EOF) {
            break;
        }
        
        if (token.type == TOKEN_ERROR) {
            error_count++;
            if (token.line < 100) {
                line_errors[token.line]++;
            }
        }
    }
    
    printf("=== Error Summary ===\n");
    if (error_count == 0) {
        printf("No lexical errors found.\n");
    } else {
        printf("Total errors: %d\n", error_count);
        printf("Errors by line:\n");
        for (int i = 0; i < 100; i++) {
            if (line_errors[i] > 0) {
                printf("  Line %d: %d error(s)\n", i, line_errors[i]);
            }
        }
    }
}