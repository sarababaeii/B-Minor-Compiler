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

//MARK: Constants
const int MAX_N = 500;
const int KYEWORDS_NUM = 15;
const char KEYWORDS[KYEWORDS_NUM][MAX_N] = {
    "array", "boolean", "char", "else", "false", "for", "function", "if",
    "integer", "print", "return", "string", "true", "void", "while"
};
char input_file[] = "/Users/mackbook/Desktop/Sara/Study/Programming/CS 97/1400-1/Compiler/Homework/HW-02/input1.txt";
char output_file[] = "/Users/mackbook/Desktop/Sara/Study/Programming/CS 97/1400-1/Compiler/Homework/HW-02/ouput1.txt";

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

typedef struct{
    Token *array;
    size_t used;
    size_t size;
} TokenArray;

Address *current_address;
int last_column_number = 0;

//MARK: Function Prototypes
Address* create_address(int line, int column);
TokenArray* scan_tokens(char*, char*);
Token* scan_next_token(FILE*);
Token* scan_integer_token(char, FILE*);
Token* scan_string_or_char_token(char, FILE*);
Token* scan_keyword_or_identifier_token(char, FILE*);
Token* scan_prefixable_operator_token(char, char, FILE*);
Token* create_token(TokenType, char*, Address);
Token* create_operator_token(char*, Address );
Token* create_simple_operator_token(char*);
Token* create_keyword_or_identifier_token(char*, Address);
Token* create_string_or_char_token(char, char*, Address);
Token get_token_from_array(TokenArray*, int);
FILE* open_file(char*, char*);
void insert_to_token_array(TokenArray*, Token);
void free_token_array(TokenArray*);
void print_token(Token*, FILE*);
void prepare_next_token(FILE*);
void skip_whitespaces(FILE*);
void skip_comments(FILE*);
void skip_c_style_comment(int, FILE*);
void skip_cpp_style_comment(int, FILE*);
void put_back_last_ascii(int, FILE*);
int get_next_ascii(FILE*);
int is_keyword(char*);
int addresses_are_equal(Address*, Address*);
char* get_token_description(TokenType);

void print_token_stndrd(Token t) {
    printf("Token: (\n\ttype: %s, \n\tvalue: %s, \n\tline: %d, col: %d)\n",
           get_token_description(t.type), t.value, t.address.line, t.address.column);
}

//MARK: Main
int main(int argc, const char * argv[]) {
    printf("Hello, World!\n");
    TokenArray *tokens;
    tokens = scan_tokens(input_file, output_file);
    for (int i = 0; i < tokens->used; i++) {
        print_token_stndrd(get_token_from_array(tokens, i));
    }
    return 0;
}

//MARK: File Management
FILE* open_file(char file_name[], char operation[]) {
    FILE *fp = fopen(file_name, operation);
    if(!fp) {
        perror("File opening failed.");
        exit(1);
    }
    return fp;
}

int get_next_ascii(FILE *fp) {
    int c = fgetc(fp);
    if (c == '\n') {
        current_address->line++;
        last_column_number = current_address->column;
        current_address->column = 0;
    } else {
        current_address->column++;
    }
    return c;
}

void put_back_last_ascii(int c, FILE *fp) {
    ungetc(c, fp);
    if (c == '\n') {
        current_address->line--;
        current_address->column = last_column_number;
    } else {
        current_address->column--;
    }
}

//MARK: Type Functions
Address* create_address(int line, int col) {
    Address *address = malloc(sizeof(Address));
    if (address == NULL) {
        return NULL;
    }
    address->line = line;
    address->column = col;
    return address;
}

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

TokenArray* create_token_array(size_t initial_size) {
    TokenArray *a = malloc(sizeof(TokenArray));
    a->array = malloc(initial_size * sizeof(Token));
    a->used = 0;
    a->size = initial_size;
    return a;
}

int addresses_are_equal(Address *add1, Address *add2) {
    return (add1->line == add2->line && add1->column == add2->column);
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

void print_token(Token *t, FILE *fp) {
    fprintf(fp, "Token: (\n\ttype: %s, \n\tvalue: %s, \n\tline: %d, col: %d)\n",
           get_token_description(t->type), t->value, t->address.line, t->address.column);
}

void insert_to_token_array(TokenArray *a, Token t) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(Token));
    }
    a->array[a->used++] = t;
}

void free_token_array(TokenArray *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

Token get_token_from_array(TokenArray *a, int index) {
    return a->array[index];
}

//MARK: Skipping Comments and Whitespaces
void prepare_next_token(FILE *fp) {
    Address last_address = *current_address, new_address = *current_address;
    do {
        skip_whitespaces(fp);
        skip_comments(fp);
        last_address = new_address;
        new_address = *current_address;
     } while (!addresses_are_equal(&last_address, &new_address));
}

void skip_whitespaces(FILE *fp) {
    int c = get_next_ascii(fp);
    while ((isspace(c) || c == '\n') && c != EOF) {
        c = get_next_ascii(fp);
    }
    put_back_last_ascii(c, fp);
}

void skip_comments(FILE *fp) {
    int c = get_next_ascii(fp);
    if (c == '/') {
        int d = get_next_ascii(fp);
        if (d == '/') {
            skip_cpp_style_comment(d, fp);
            return;
        }
        if (d == '*') {
            skip_c_style_comment(d, fp);
            return;
        }
        put_back_last_ascii(d, fp);
    }
    put_back_last_ascii(c, fp);
}

void skip_c_style_comment(int d, FILE *fp) { /* */
    int e = get_next_ascii(fp);
    while ((d != '*' || e != '/') && e != EOF) {
        d = e;
        e = get_next_ascii(fp);
    }
}

void skip_cpp_style_comment(int d, FILE *fp) { //
    while (d != '\n' && d != EOF) {
        d = get_next_ascii(fp);
    }
}

// MARK: Operator Tokens
Token* create_operator_token(char val[], Address add) {
    return create_token(OPERATOR, val, add);
}

Token* create_simple_operator_token(char val[]) {
    return create_operator_token(val, *current_address);
}

Token* scan_prefixable_operator_token(char extracted_char, char expected_char, FILE *fp) {
    Address address = *current_address;
    char value[3] = {extracted_char, 0, 0};
    int d = get_next_ascii(fp);
    if (d == expected_char) {
        value[1] = d;
        return create_operator_token(value, address);
    }
    put_back_last_ascii(d, fp);
    return create_simple_operator_token(value);
}

//MARK: Keyword and Identifier Tokens
int is_keyword(char str[]) {
    for (int i = 0; i < KYEWORDS_NUM; i++) {
        if (strcmp(KEYWORDS[i], str) == 0) {
            return 1;
        }
    }
    return 0;
}

Token* create_keyword_or_identifier_token(char val[], Address add) {
    if (is_keyword(val)) {
        return create_token(KEYWORD, val, add);
    }
    return create_token(IDENTIFIER, val, add);
}

Token* scan_keyword_or_identifier_token(char c, FILE *fp) {
    Address add = *current_address;
    char value[MAX_N];
    int i;
    for (i = 0; isalnum(c) || c == '_'; i++) {
        value[i] = c;
        c = get_next_ascii(fp);
    }
    put_back_last_ascii(c, fp);
    value[i] = 0;
    return create_keyword_or_identifier_token(value, add);
}

//MARK: Integer, String and Character Tokens
Token* scan_integer_token(char c, FILE *fp) {
    Address add = *current_address;
    char value[MAX_N];
    int i;
    for (i = 0; isdigit(c); i++) {
        value[i] = c;
        c = get_next_ascii(fp);
    }
    put_back_last_ascii(c, fp);
    value[i] = 0;
    return create_token(INTEGER, value, add);
}

Token* create_string_or_char_token(char sign, char val[], Address add) {
    if (sign == '"') {
        return create_token(STRING, val, add);
    }
    return create_token(CHARACTER, val, add);
}

Token* scan_string_or_char_token(char sign, FILE *fp) { //TODO: \0
    int c = get_next_ascii(fp);
    Address add = *current_address;
    char value[MAX_N];
    int i;
    for (i = 0; c != sign || (i > 0 && value[i - 1] == '\\'); i++) {
        value[i] = c;
        c = get_next_ascii(fp);
    }
    value[i] = 0;
    return create_string_or_char_token(sign, value, add);
}

//MARK: Scanning
TokenArray* scan_tokens(char input_file_name[], char output_file_name[]) {
    FILE *fp_in = open_file(input_file_name, "r");
    FILE *fp_out = open_file(output_file_name, "w");
    TokenArray *a = create_token_array(10);
    current_address = create_address(1, 1);
    Token *t = scan_next_token(fp_in);
    while (t != NULL) {
        insert_to_token_array(a, *t);
        print_token(t, fp_out);
        t = scan_next_token(fp_in);
    }
    fclose(fp_in);
    fclose(fp_out);
    return a;
}

Token* scan_next_token(FILE *fp) {
    prepare_next_token(fp);
    int c = get_next_ascii(fp);
    if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
        c == ';' || c == ':' || c == ',' || c == '*' || c == '/' || c == '%' || c == '^') {
        char value[] = {c, 0};
        return create_simple_operator_token(value);
    }
    if (c == '+' || c == '-' || c == '&' || c == '|') {
        return scan_prefixable_operator_token(c, c, fp);
    }
    if (c == '<' || c == '>' || c == '!' || c == '=') {
        return scan_prefixable_operator_token(c, '=', fp);
    }
    if (c == '\'' || c == '"') {
        return scan_string_or_char_token(c, fp);
    }
    if (isdigit(c)) {
        return scan_integer_token(c, fp);
    }
    if (c == '_' || isalpha(c)) {
        return scan_keyword_or_identifier_token(c, fp);
    }
    return NULL;
}
