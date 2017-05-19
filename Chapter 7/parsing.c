#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char* readline(char* prompt){
    fputs(prompt,stdout);
    fgets(buffer,2048,stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy,buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused){}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

long eval_op(long x, char* op, long y){
    if(strcmp(op,"+") == 0){
        return x + y;
    }

    if(strcmp(op,"-") == 0){
        return x - y;
    }

    if(strcmp(op,"*") == 0){
        return x * y;
    }

    if(strcmp(op,"/") == 0){
        return x / y;
    }

    if(strcmp(op,"%") == 0){
        return x % y;
    }

    if(strcmp(op,"^") == 0){
        long result = 1;
        for(int i = 0; i < y; i++){
            result = result * x;
        }
        return result;
    }

    return 0;
}



long eval(mpc_ast_t* t){
    if(strstr(t->tag, "number" )){
        return atoi(t->contents);
    }

     //Operator is always second child 
    char buffer[strlen(t->children[1]->contents) + 1];
    char* op = malloc(strlen(buffer)+1);
    strcpy(op,t->children[1]->contents);
    

    if(strcmp(op,"min") == 0){
        int s = 2; //start after operator
        long lowest = 0;

        while(strstr(t -> children[s]->tag,"number")){
            long value = eval(t->children[s]);
            if(lowest == 0){
                lowest = value;
            } else {
                lowest = value < lowest ? value : lowest;
            }
            s++;
        }   

        return lowest;
    }

    if(strcmp(op,"max") == 0){
        int s = 2; //start after operator
        long highest = 0;

        while(strstr(t -> children[s]->tag,"number")){
            long value = eval(t->children[s]);
            if(highest == 0){
                highest = value;
            } else {
                highest = value > highest ? value : highest;
            }
            s++;
        }   

        return highest;
    }

    //evaluate and store third child in x
    long x = eval(t->children[2]);

    /* If '-' operator receives one argument, negate it */
    if (strcmp(op, "-") == 0 && t->children_num < 5) { return 0 - x; }

    int i = 3;
    while(strstr(t -> children[i]->tag,"expr")){
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}




int main(int argc, char** argv){
   

    //create parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");


    mpca_lang(MPCA_LANG_DEFAULT,
    
    "                                                                       \
    number      : /-?[0-9]+/ ;                                              \
    operator    : '+' | '-' | '*' | '/' | '^' | '%' | \"min\" | \"max\" ;   \
    expr        : <number> | '(' <operator> <expr>+ ')' ;                   \
    lispy       : /^/ <operator> <expr>+ /$/ ;                              \
    "
    ,
    Number,Operator,Expr,Lispy
    );
    
    puts("Lispy Version 0.0.0.0.2");
    puts("Press Ctrl+c to Exit\n");



    while(1){
        char* input = readline("lispy> ");
        add_history(input);
        
        //parse user input
        mpc_result_t r;
        if(mpc_parse("<stdin>",input,Lispy,&r)){
            long result = eval(r.output);
            printf("%li\n",result);
            mpc_ast_delete(r.output);
        } else{
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4,Number, Operator, Expr, Lispy);
    return 0;
}