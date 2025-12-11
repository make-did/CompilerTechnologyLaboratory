// 示例 main：演示如何使用 parser 与你已有的 lexer
// 编译：gcc lexer.c parser.c parser_main.c -o parser
// 运行：./parser test.c

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    int rc = parse_file(filename);
    if (rc == 0) {
        printf("Success: source '%s' parsed OK.\n", filename);
    } else if (rc == 1) {
        printf("Fatal: could not open file.\n");
    } else {
        printf("Parsed with errors (exit code %d).\n", rc);
    }
    return rc;
}