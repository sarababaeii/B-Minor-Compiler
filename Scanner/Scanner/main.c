//
//  main.c
//  Scanner for B-Minor compiler
//
//  Created by Sara Babaei on 12/9/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//MARK: Constants and Global Variables
const int MAX_N = 500;
const int KYEWORDS_NUM = 15;
const char KEYWORDS[KYEWORDS_NUM][MAX_N] = {
    "array", "boolean", "char", "else", "false", "for", "function", "if",
    "integer", "print", "return", "string", "true", "void", "while"
};

//MARK: Type Definitions
typedef enum {
    KEYWORD,
    IDENTIFIER,
    OPERATOR,
    INTEGER,
    CHARACTER,
    STRING
} TokenType;

typedef struct {
    int line;
    int column;
} Address;

typedef struct {
    TokenType type;
    char value[MAX_N];
    Address address;
} Token;

Address *current_address;
int last_column_number = 0;


Address* create_address(int line, int column);
Token* create_token(TokenType type, char value[], Address address);
int addresses_are_equal(Address *address1, Address *address2);

int get_next_ascii(FILE *file_pointer);
void put_back_last_ascii(int c, FILE *file_pointer);
FILE* open_file(char file_name[], char operation[]);

void skip_cpp_style_comments(int d, FILE *file_pointer);
void skip_c_style_comments(int d, FILE *file_pointer);
void skip_comments(FILE *file_pointer);
void skip_whitespaces(FILE *file_pointer);
void prepare_next_token(FILE *file_pointer);

Token* create_operator_token(char value[], Address address);
Token* create_simple_operator_token(char value[]);
Token* scan_prefixable_operator_token(char extracted_char, char expected_char, FILE *file_pointer);

int is_keyword(char str[]);
Token* create_keyword_or_identifier_token(char value[], Address address);
Token* scan_keyword_or_identifier_token(char c, FILE *file_pointer);

Token* scan_integer_token(char c, FILE *file_pointer);
Token* scan_character_token(FILE *file_pointer);
Token* scan_string_token(FILE *file_pointer);

Token* scan_next_token(FILE *fp);
void print_token(Token *t);
char* get_token_description(TokenType t);
void scan_tokens(char file_name[]);

int main(int argc, const char * argv[]) {
    printf("Hello, World!\n");
    current_address = create_address(1, 1);
    scan_tokens("/Users/mackbook/Desktop/Sara/Study/Programming/CS 97/1400-1/Compiler/Homework/HW-02/input1.txt");
    
    return 0;
}

//MARK: File Management
FILE* open_file(char file_name[], char operation[]) {
    FILE *fp = fopen(file_name, operation);
    if(!fp) {
        perror("File opening failed");
        exit(1);
    }
    return fp;
}

int get_next_ascii(FILE *file_pointer) {
    int c = fgetc(file_pointer);
    if (c == '\n') {
        current_address->line++;
        last_column_number = current_address->column;
        current_address->column = 0;
    } else {
        current_address->column++;
    }
    return c;
}

void put_back_last_ascii(int c, FILE *file_pointer) {
    ungetc(c, file_pointer);
    if (c == '\n') {
        current_address->line--;
        current_address->column = last_column_number;
    } else {
        current_address->column--;
    }
}

//MARK: Type Functions
Token* create_token(TokenType type, char value[], Address address) {
    Token *t = malloc(sizeof(Token));
    if (t == NULL) {
        return NULL;
    }
    t->type = type;
    t->address = address;
    strcpy(t->value, value);
    return t;
}

Address* create_address(int line, int column) {
    Address *address = malloc(sizeof(Address));
    if (address == NULL) {
        return NULL;
    }
    address->line = line;
    address->column = column;
    return address;
}

int addresses_are_equal(Address *address1, Address *address2) {
    return (address1->line == address2->line && address1->column == address2->column);
}

void print_token(Token *t) {
    printf("Token: (type: %s, value: %s, line: %d, col: %d)\n",
           get_token_description(t->type), t->value, t->address.line, t->address.column);
}

char* get_token_description(TokenType t) {
    switch (t) {
        case KEYWORD:
            return "keyword";
            break;
        case IDENTIFIER:
            return "identifier";
            break;
        case OPERATOR:
            return "operator";
            break;
        case INTEGER:
            return "integer";
            break;
        case CHARACTER:
            return "character";
            break;
        case STRING:
            return "string";
            break;
    }
}

// Skipping Comments and Whitespaces
void prepare_next_token(FILE *file_pointer) {
    Address last_address = *current_address, new_address = *current_address;
    do {
        skip_whitespaces(file_pointer);
        skip_comments(file_pointer);
        last_address = new_address;
        new_address = *current_address;
     } while (!addresses_are_equal(&last_address, &new_address));
}

void skip_whitespaces(FILE *file_pointer) {
    int c = get_next_ascii(file_pointer);
    while ((isspace(c) || c == '\n') && c != EOF) {
        c = get_next_ascii(file_pointer);
    }
    put_back_last_ascii(c, file_pointer);
}

void skip_comments(FILE *file_pointer) {
    int c = get_next_ascii(file_pointer);
    if (c == '/') {
        int d = get_next_ascii(file_pointer);
        if (d == '/') {
            skip_cpp_style_comments(d, file_pointer);
            return;
        } else if (d == '*') {
            skip_c_style_comments(d, file_pointer);
            return;
        } else {
            put_back_last_ascii(d, file_pointer);
        }
    }
    put_back_last_ascii(c, file_pointer);
}

void skip_c_style_comments(int d, FILE *file_pointer) { /* */
    int e = get_next_ascii(file_pointer);
    while ((d != '*' || e != '/') && e != EOF) {
        d = e;
        e = get_next_ascii(file_pointer);
    }
}

void skip_cpp_style_comments(int d, FILE *file_pointer) { //
    while (d != '\n' && d != EOF) {
        d = get_next_ascii(file_pointer);
    }
}

// Creating Operator Tokens
Token* create_operator_token(char value[], Address address) {
    return create_token(OPERATOR, value, address);
}

Token* create_simple_operator_token(char value[]) {
    return create_operator_token(value, *current_address);
}

Token* scan_prefixable_operator_token(char extracted_char, char expected_char, FILE *file_pointer) {
    Address address = *current_address;
    char value[3] = {extracted_char, 0, 0};
    int d = get_next_ascii(file_pointer);
    if (d == expected_char) {
        value[1] = d;
        return create_operator_token(value, address);
    }
    put_back_last_ascii(d, file_pointer);
    return create_simple_operator_token(value);
}

// Creating Keyword and Identifier Tokens
int is_keyword(char str[]) {
    for (int i = 0; i < KYEWORDS_NUM; i++) {
        if (strcmp(KEYWORDS[i], str) == 0) {
            return 1;
        }
    }
    return 0;
}

Token* create_keyword_or_identifier_token(char value[], Address address) {
    if (is_keyword(value)) {
        return create_token(KEYWORD, value, address);
    }
    return create_token(IDENTIFIER, value, address);
}

Token* scan_keyword_or_identifier_token(char c, FILE *file_pointer) {
    Address address = *current_address;
    char value[MAX_N];
    int i;
    for (i = 0; isalnum(c) || c == '_'; i++) {
        value[i] = c;
        c = get_next_ascii(file_pointer);
    }
    put_back_last_ascii(c, file_pointer);
    value[i] = 0;
    return create_keyword_or_identifier_token(value, address);
}

// Creating Integer, String and Character Tokens
Token* scan_integer_token(char c, FILE *file_pointer) {
    Address address = *current_address;
    char value[MAX_N];
    int i;
    for (i = 0; isdigit(c); i++) {
        value[i] = c;
        c = get_next_ascii(file_pointer);
    }
    put_back_last_ascii(c, file_pointer);
    value[i] = 0;
    return create_token(INTEGER, value, address);
}

Token* scan_character_token(FILE *file_pointer) { //TODO: Check, \'s
    Address address = *current_address;
    int c = get_next_ascii(file_pointer);
    if (c == '\'') {
        return create_token(CHARACTER, "", address);
    } else if (c == '\\') {
        
    } else {
        char value[2] = {c, 0};
        return create_token(CHARACTER, value, address);
    }
    return NULL;
}

Token* scan_string_token(FILE *file_pointer) { //TODO: Check
    int c = get_next_ascii(file_pointer);
    Address address = *current_address;
    char value[MAX_N];
    int i;
    for (i = 0; c != '"'; i++) {
        value[i] = c;
        c = get_next_ascii(file_pointer);
    }
    value[i] = 0;
    return create_token(STRING, value, address);
}

// Scanning
void scan_tokens(char file_name[]) {
    FILE *fp = open_file(file_name, "r");
    Token *t = scan_next_token(fp);
    while (t != NULL) {
        print_token(t);
        t = scan_next_token(fp);
    }
    fclose(fp);
}


Token* scan_next_token(FILE *fp) {
    prepare_next_token(fp);
    int c = get_next_ascii(fp);
    printf("%c\n", c);
    
    if (c == ';' || c == ':' || c == ',' || c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}' || c == '*' || c == '/' || c == '%' || c == '^') {
        char value[] = {c, 0};
        return create_simple_operator_token(value);
    }
    if (c == '+' || c == '-' || c == '&' || c == '|') {
        return scan_prefixable_operator_token(c, c, fp);
    }
    if (c == '<' || c == '>' || c == '!' || c == '=') {
        return scan_prefixable_operator_token(c, '=', fp);
    }
    if (c == '\'') {
        return scan_character_token(fp);
    }
    if (c == '"') {
        return scan_string_token(fp);
    }
    if (isdigit(c)) {
        return scan_integer_token(c, fp);
    }
    if (c == '_' || isalpha(c)) {
        return scan_keyword_or_identifier_token(c, fp);
    }
    return NULL;
}

