#ifndef PARSER_H
#define PARSER_H

#include"lexer.h"

//返回0表示语法通过 
int parse_file(const char* filename);

#endif