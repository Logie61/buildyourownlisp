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

enum {LVAL_NUM, LVAL_ERR};
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};


typedef struct {
    int type;
    long num;
    int err;
} lval;

lval lval_num(long x){
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

lval lval_err(int x){
    lval v;
    v.type = LVAL_ERR;
    v.err = x;

    return v;
}

void lval_print(lval v){
    switch(v.type){
        case LVAL_NUM: 
            printf("%li",v.num);
            break;
        case LVAL_ERR:
            if(v.err == LERR_DIV_ZERO){
                printf("Error: Division by Zero!");
            }
            if(v.err == LERR_BAD_OP){
                printf("ErrorD Invalid Operator!");
            }
            if(v.err == LERR_BAD_NUM){
                printf("ErrorD Invalid Number!");
            }
        break;
    }
}

void lval_println(lval v){
    lval_print(v);
    putchar('\n');
}

lval eval_op(lval x, char* op, lval y){

	if(x.type == LVAL_ERR){
		return x;
	}

	if(y.type == LVAL_ERR){
		return y;
	}

    if(strcmp(op,"+") == 0){
		return lval_num(x.num + y.num);
    }

    if(strcmp(op,"-") == 0){
        return lval_num(x.num - y.num);
    }

    if(strcmp(op,"*") == 0){
        return lval_num(x.num * y.num);
    }

    if(strcmp(op,"/") == 0){

		if(y.num == 0){
			return lval_err(LERR_DIV_ZERO);
		}

        return lval_num(x.num / y.num);
    }

    if(strcmp(op,"%") == 0){
        return lval_num(x.num % y.num);
    }

    if(strcmp(op,"^") == 0){
        long result = 1;
        for(int i = 0; i < y.num; i++){
            result = result * x.num;
        }
        return lval_num(result);
    }

    return lval_err(LERR_BAD_OP);
}



lval eval(mpc_ast_t* t){
    if(strstr(t->tag, "number" )){

		errno = 0;
		long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

     //Operator is always second child 
    char buffer[strlen(t->children[1]->contents) + 1];
    char* op = malloc(strlen(buffer)+1);
    strcpy(op,t->children[1]->contents);
    

    if(strcmp(op,"min") == 0){
        int s = 2; //start after operator
        long lowest = 0;

        while(strstr(t -> children[s]->tag,"number")){
            lval value = eval(t->children[s]);
            if(lowest == 0){
                lowest = value.num;
            } else {
                lowest = value.num < lowest ? value.num : lowest;
            }
            s++;
        }   

        return lval_num(lowest);
    }

    if(strcmp(op,"max") == 0){
        int s = 2; //start after operator
        long highest = 0;

        while(strstr(t -> children[s]->tag,"number")){
            lval value = eval(t->children[s]);
            if(highest == 0){
                highest = value.num;
            } else {
                highest = value.num > highest ? value.num : highest;
            }
            s++;
        }   

        return lval_num(highest);
    }

    //evaluate and store third child in x
    lval x = eval(t->children[2]);

    /* If '-' operator receives one argument, negate it */
    if (strcmp(op, "-") == 0 && t->children_num < 5) { return lval_num(0 - x.num) ; }

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
            lval result = eval(r.output);
            lval_println(result);
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