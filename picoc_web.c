#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef enum {
    TOK_INT_LITERAL, TOK_ID, TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV,
    TOK_ASSIGN, TOK_SEMI, TOK_LPAREN, TOK_RPAREN, TOK_EOF, TOK_INVALID,
    TOK_KEYWORD_INT
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[64];
} Token;

char *src;
int pos = 0;

Token getNextToken() {
    Token token;
    token.type = TOK_INVALID;
    token.lexeme[0] = '\0';

    while (isspace(src[pos])) pos++;

    if (isdigit(src[pos])) {
        int i = 0;
        while (isdigit(src[pos])) token.lexeme[i++] = src[pos++];
        token.lexeme[i] = '\0';
        token.type = TOK_INT_LITERAL;
    } else if (isalpha(src[pos])) {
        int i = 0;
        while (isalnum(src[pos])) token.lexeme[i++] = src[pos++];
        token.lexeme[i] = '\0';

        if (strcmp(token.lexeme, "int") == 0)
            token.type = TOK_KEYWORD_INT;
        else
            token.type = TOK_ID;
    } else {
        char ch = src[pos++];
        switch (ch) {
            case '+': token.type = TOK_PLUS; break;
            case '-': token.type = TOK_MINUS; break;
            case '*': token.type = TOK_MUL; break;
            case '/': token.type = TOK_DIV; break;
            case '=': token.type = TOK_ASSIGN; break;
            case ';': token.type = TOK_SEMI; break;
            case '(': token.type = TOK_LPAREN; break;
            case ')': token.type = TOK_RPAREN; break;
            case '\0': token.type = TOK_EOF; break;
            default: token.type = TOK_INVALID; break;
        }
        token.lexeme[0] = ch;
        token.lexeme[1] = '\0';
    }
    return token;
}

typedef struct Node {
    enum { NODE_INT, NODE_VAR, NODE_BINOP, NODE_ASSIGN, NODE_SEQ } type;
    union {
        int value;
        char var[64];
        struct { struct Node *left, *right; char op; } binop;
        struct { char var[64]; struct Node *expr; } assign;
    } data;
} Node;

Token currentToken;
void advance() { currentToken = getNextToken(); }

Node* parseExpression();

Node* parseFactor() {
    if (currentToken.type == TOK_INT_LITERAL) {
        Node *node = malloc(sizeof(Node));
        node->type = NODE_INT;
        node->data.value = atoi(currentToken.lexeme);
        advance();
        return node;
    } else if (currentToken.type == TOK_ID) {
        char varName[64];
        strcpy(varName, currentToken.lexeme);
        advance();
        if (currentToken.type == TOK_ASSIGN) {
            advance();
            Node *expr = parseExpression();
            Node *node = malloc(sizeof(Node));
            node->type = NODE_ASSIGN;
            strcpy(node->data.assign.var, varName);
            node->data.assign.expr = expr;
            return node;
        } else {
            Node *node = malloc(sizeof(Node));
            node->type = NODE_VAR;
            strcpy(node->data.var, varName);
            return node;
        }
    } else if (currentToken.type == TOK_LPAREN) {
        advance();
        Node *node = parseExpression();
        if (currentToken.type == TOK_RPAREN) advance();
        return node;
    }
    return NULL;
}

Node* parseTerm() {
    Node *node = parseFactor();
    while (currentToken.type == TOK_MUL || currentToken.type == TOK_DIV) {
        char op = currentToken.lexeme[0];
        advance();
        Node *right = parseFactor();
        Node *newNode = malloc(sizeof(Node));
        newNode->type = NODE_BINOP;
        newNode->data.binop.left = node;
        newNode->data.binop.right = right;
        newNode->data.binop.op = op;
        node = newNode;
    }
    return node;
}

Node* parseExpression() {
    Node *node = parseTerm();
    while (currentToken.type == TOK_PLUS || currentToken.type == TOK_MINUS) {
        char op = currentToken.lexeme[0];
        advance();
        Node *right = parseTerm();
        Node *newNode = malloc(sizeof(Node));
        newNode->type = NODE_BINOP;
        newNode->data.binop.left = node;
        newNode->data.binop.right = right;
        newNode->data.binop.op = op;
        node = newNode;
    }
    return node;
}

Node* parseStatement() {
    if (currentToken.type == TOK_KEYWORD_INT) {
        advance();  // consume 'int'
        if (currentToken.type != TOK_ID) {
            printf("Syntax Error: Expected variable name after 'int'\n");
            return NULL;
        }

        char varName[64];
        strcpy(varName, currentToken.lexeme);
        advance();

        if (currentToken.type != TOK_ASSIGN) {
            printf("Syntax Error: Expected '=' after variable name\n");
            return NULL;
        }

        advance(); 
        Node *expr = parseExpression();

        Node *node = malloc(sizeof(Node));
        node->type = NODE_ASSIGN;
        strcpy(node->data.assign.var, varName);
        node->data.assign.expr = expr;

        if (currentToken.type == TOK_SEMI) advance();
        return node;
    }

    Node* expr = parseExpression();
    if (currentToken.type == TOK_SEMI) advance();
    return expr;
}

Node* parseStatements() {
    Node* first = parseStatement();
    if (!first) return NULL;

    while (currentToken.type != TOK_EOF) {
        Node* second = parseStatement();
        Node* seqNode = malloc(sizeof(Node));
        seqNode->type = NODE_SEQ;
        seqNode->data.binop.left = first;
        seqNode->data.binop.right = second;
        seqNode->data.binop.op = 0;
        first = seqNode;
    }
    return first;
}


int variables[26] = {0};

int evaluate(Node *node) {
    if (node->type == NODE_INT) {
        return node->data.value;
    } else if (node->type == NODE_VAR) {
        return variables[node->data.var[0] - 'a'];
    } else if (node->type == NODE_ASSIGN) {
        int val = evaluate(node->data.assign.expr);
        variables[node->data.assign.var[0] - 'a'] = val;
        return val;
    } else if (node->type == NODE_BINOP) {
        int left = evaluate(node->data.binop.left);
        int right = evaluate(node->data.binop.right);
        switch (node->data.binop.op) {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/': return right ? left / right : 0;
        }
    } else if (node->type == NODE_SEQ) {
        evaluate(node->data.binop.left);
        return evaluate(node->data.binop.right);
    }
    return 0;
}


int main() {
    FILE *fp = fopen("input.txt", "r");
    if (!fp) {
        printf("Error opening input.txt\n");
        return 1;
    }

    char input[1024];
    if (!fgets(input, sizeof(input), fp)) {
        printf("Error reading input\n");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    src = input;
    pos = 0;
    advance();
    Node *ast = parseStatements();

    if (currentToken.type == TOK_EOF) {
        int result = evaluate(ast);
        printf("%d\n", result);  
    } else {
        printf("Syntax Error!\n");
    }

    return 0;
}





