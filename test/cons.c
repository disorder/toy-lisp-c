/*
#define ODUMP(name) debug(" " #name "\n", (print_obj(name),0))
#define CONS(c) ((cons_type != O_TYPE(c)?               \
                  (ODUMP(c), raise("not a cons %016x", c), 0) :      \
                  c))
*/
/* #define CDR(c, value) ((cons_type != O_TYPE(c)?                 \ */
/*         { ODUMP(c); raise("not a cons")) :                      \ */
/*                         (c->cdr = value))) */

// prevent warnings
#define ODUMP(name)
#define CONS(c) c

#include "../str.h"

void test_cons()
{
    cons_t *X = cons(NULL, NULL);
    //cons_t *c = cons(NULL, NULL);
    cons_t *c = X;
    print_cons(c, NULL);
    puts("");
    iterate_list(c, &print_cons, NULL);
    puts("");

    print_list(NIL);
    puts("");
    print_list(c);
    puts("");

    puts("looping");
    for (int i=0; i<3; i++) {
        printf("%d\n", i);
        // this is just a hack, object_t should never be allocated directly
        NEW(object, o);
        OBJ(o, type) = object_type;
        OBJ(o, id) = 1234;
        cons_append(c, o);
        cons_append(c, (object_t *) cons((object_t *)NIL,(object_t *)NIL));
        cons_append(c, (object_t *) NIL);
    }
    print_list(c);
    //iterate_list(c, &print_cons, NULL);
    puts("");

    // will end with "car . object_type)"
    puts("replace last with object_type");
    ODUMP(c);
    cons_t *l = last(c);
    ODUMP(c);
    ODUMP(l);
    NEW(object, o);
    OBJ(o, type) = object_type;
    OBJ(o, id) = 0xff;
    //l->cdr = o;
    CONS(l)->cdr = o;
    //CDR(l) = o;
//    print_obj(o);
//    debug("\nlast %016x %016x\n", l->car, l->cdr);
    ODUMP(c);
    print_list(c);
    puts("");

    // replace second element with NIL
    //c->cdr->car = NIL;
    CONS(CONS(c)->cdr)->car = NIL;
    debug("HERE\n");
    print_list(c);
    puts("");

    // replace second element with (NIL)
//    debug(" c\n", (print_obj(c), 0));
//    debug(" car\n", (print_obj(c->car), 0));
    ODUMP(c);
    ODUMP(c->car);
    ODUMP(c->cdr);
    ODUMP(c->cdr->car);
    // ok
    CONS(CONS(c)->cdr)->car = NIL;
    ODUMP(c->cdr->car);
    // ok
    CONS(CONS(c)->cdr)->car = o;
    ODUMP(c->cdr->car);
    // ok
    puts("tagged");
    CONS(CONS(c)->cdr)->car = (cons_t *) ~0;
    ODUMP(c->cdr->car);
    // not ok
    puts("wrong output - and all before last is same as first");
    CONS(CONS(c)->cdr)->car = cons(NIL,NIL);

    cons(NIL,NIL)->cdr = cons(NIL,NIL);

    cons_t *x = cons(NIL,NIL);
    ODUMP(x);
    c->cdr->car = x;
    print_list(c);
    cons_t omg = {
        /*
        .obj = {
            .type = cons_type,
            .id = 0xdead,
        },
        */
        .car = NIL,
        .cdr = NIL,
    };
//    c->cdr->car = &omg;
    ODUMP(c->cdr->car);
    c->cdr->car->cdr = &omg;
    print_list(c);
    puts("");
    puts("is this wrong?");
    ODUMP(cons(NIL,NIL));
    puts("");
    print_list(c);
    puts("");

    debug("dotted\n");
#define VAL (0x42<<2 | 1)
    //c->cdr->cdr = (cons_t *) VAL;//o;
    print_list(c);
    puts("");
    c->cdr->car->car = (cons_t *) VAL;//o;
    print_list(c);
    puts("");
    c->cdr->car->cdr = (cons_t *) VAL;//o;
    print_list(c);
    puts("");
    c->cdr->car->car = &omg;
    print_list(c);
    puts("");

    //iterate_list
    puts("");
}
