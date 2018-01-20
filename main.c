#include "common.h"
#include "str.h"
#include "read.h"
#include "eval.h"

void test_cons();
void test_str();
void test_token();
void test_stoken();
void test_eval();

void print_stats()
{
    printf("consed: %zu\n", consed);
}

// callback
int cb_main_eval(cons_t *c, cons_t **results)
{
    // discarding results
    printf("EVAL> %s\n", dump_obj(c->car));
    object_t *value = eval_token(c->car);
    if (TAG(value))
        printf("TAG> %d\n", (int) TAG(value));
    else
        printf("TYPE> %d\n", O_TYPE(value));
    printf("RESULT> %s\n", dump_obj(value));
    if (results != NULL)
        *results = cons_append(*results, value);

    return 0;
}

void main_eval(cons_t *code, cons_t **results)
{
    iterate_list(code, &cb_main_eval, results);
}

void test()
{
    atexit(&print_stats);
    test_cons();
    test_str();
    test_stoken();
    test_token();

    puts("reading code");
    // beware NULL
    str_t *code = read_file("test.lisp");
    printf("used %zu\n", code->used);

    cons_t *tokens = read_code(code);
    print_list(tokens);

    init_eval();
    test_eval();

    str_t *internal = new_str_c("(defun internal () (puts 'internal'))", 0);
    eval_code(read_code(internal), NULL);
    //eval_code(tokens, NULL);
    main_eval(tokens, NULL);

    //raise("test %d\n", NULL);
}

#include <libgen.h>
#include <string.h>
jmp_buf repl;
int DIE = 1;
int main(int argc, char *argv[])
{
    if (0 == strncmp("test", basename(argv[0]), 4)) {
        test();
        return 0;
    }

    if (argc > 1) {
        str_t *code = read_file(argv[1]);
        cons_t *tokens = read_code(code);
        printf("read %zu bytes\n", code->used);
        init_eval();
        //eval_code(tokens, NULL);
        main_eval(tokens, NULL);
        return 0;
    }

    // simple REPL, no extra libraries
    init_eval();
    str_t *code = new_str(1024, 0);
    int c;
    DIE = 0;
    printf("> ");
    while ((c = getchar()) != EOF) {
        str_append(code, c);
        if (c == '\n') {
            if (!setjmp(repl)) {
                cons_t *tokens = read_code(code);
                //print_list(tokens);
                //eval_code(tokens, NULL);
                main_eval(tokens, NULL);
            } else {
                printf("\nevaluation failed\n");
                // roll back stack but keep global variables
                while (1) {
                    token_t *name = caar(stack);
                    // global is the name, it should be something unique
                    if (is_a(token, name) &&
                        0 == strcmp("global", TOKEN_DATA(name)))
                        break;
                    POP;
                }
                // stack = NIL;
                // init_eval();
            }
            // null everything, \0 is just a whitespace to parser
            memset(code->data, 0, code->used);
            code->used = 0;
            printf("\n> ");
        }
    }

    return 0;
}
