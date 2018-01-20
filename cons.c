#include <stdlib.h>
#include <stdio.h>

#include "cons.h"
#include "str.h"
#include "token.h"

#include "test/cons.c"

size_t consed = 0;

cons_t *cons(object_t *car, object_t *cdr)
{
    consed += 1;
    NEW(cons, c);

    c->car = car;
    c->cdr = cdr;

    CHECK_PTR(c);
    return c;
}

object_t *car(cons_t *c)
{
    if (!listp(c))
        raise("car: not a list");
    return nilp(c)? NIL : c->car;
}

cons_t *cdr(cons_t *c)
{
    if (!listp(c))
        raise("car: not a list");
    return nilp(c)? NIL : c->cdr;
}

object_t *nth(u32 n, cons_t *c)
{
    while (n-- > 0)
        c = cdr(c);
    return car(c);
}

int length(cons_t *c)
{
    int i = 0;
    while (!nilp(c)) {
        c = cdr(c);
        i++;
    }
    return i;
}

// this whole function could be simpler with listp() and cdr()
cons_t *last(cons_t *this)
{
    if (nilp(this))
        return NIL;

    if (!listp(this)) {
        raise("last() called on value that is not a list: %s", dump_obj(this));
    }

    cons_t *l = this;
    while (l->cdr != NULL)
        l = (cons_t *) l->cdr;

    return l;
}

//#define APPEND(c, o) cons_append((cons_t *) c, (object_t *) o)
// destructive
// returns self/new list if was nil which is different from lisp
cons_t *cons_append(cons_t *c, object_t *obj)
{
    if (nilp(c))
        // new list
        return cons(obj, (object_t *) NIL);

    cons_t *l = last(c);
    if (nilp(l->cdr))
        l->cdr = (object_t *) cons(obj, (object_t *) NIL);
    else
        raise("can't append to last element with cdr = %016zx", (size_t)l->cdr);

    return c;
}

// proper list
// TODO circular structure?
// yields every cons in list, breaks and returns current cons if callback
// returns false (do not continue signal)
cons_t *iterate_list(cons_t *this, int (*yield)(cons_t *, object_t *),
                     void *data)
{
    if (nilp(this))
        return NIL;

    if (yield(this, data))
        return this;

    while (!nilp(this->cdr)) {
        if (!listp(this->cdr)) {
            // TODO we should at least warn that was not a proper list?
            //puts("list with last cdr is not NIL");
            break;
        } else {
            this = (cons_t *) this->cdr;
            if (yield(this, data))
                return this;
        }
    }

    return NIL;
}

#define DUMP(o) (nilp(o)? debug("NIL") : (IMM(o)? debug("%016x", o) : debug("%s", dump_obj(o))))
//#define DUMPC(o) (printf("("), DUMP(o->car), debug(" . "),   \
//                  DUMP(o->cdr), debug(") "))
//#define DUMPC(o) (printf("("), DUMP(o->car), debug(" . %x)", o->cdr))
#define DUMPC(o) debug("(%zx . %zx)", (size_t)o->car, (size_t)o->cdr)
// callback
int print_cons(cons_t *cons, void *data)
{
    debug("CAR=");
    // TODO this is correct but not same as in ruby, may cause issues
    if (nilp(cons))
        debug("NIL");
    else
        DUMPC(cons);
        //printf("(%016x . %016x)", cons->car, cons->cdr);
    debug(" ");

    return 0;
}

// will print NIL as ()
str_t *sprint_list(str_t *s, cons_t *c)
{
    s = str_sprintf(s, "(");
    if (nilp(c))
        ;
    else
        iterate_list(c, &dump_car, &s);
    str_sprintf(s, ")");
    return s;
}

// will print NIL as ()
void print_list(cons_t *c)
{
    str_t *s = str_printf("(");
    if (nilp(c))
        ;
    else
        iterate_list(c, &dump_car, &s);
    str_sprintf(s, ")\n");
    puts(s->data);
}

str_t *sprint_obj(str_t *s, object_t *o)
{
    if (s == NULL)
        s = str_printf("");

    if (nilp(o))
        str_sprintf(s, "NIL");
       //str_sprintf(s, "()");
    else if (listp(o))
        // we don't use this
        sprint_list(s, (cons_t *) o);
    else if (is_obj(o)) {
        //str_sprintf(s, "obj%d", O_TYPE(o));
        //str_sprintf(s, "[id=%d,type=%d,ptr=%016x]", O_ID(o), O_TYPE(o), o);
        switch (O_TYPE(o)) {
        // each string type will stop printing at first \0
        case str_type:
            str_sprintf(s, "\"%s\"", ((str_t *) o)->data);
            break;
        case token_type:
            str_sprintf(s, "%s", TOKEN_DATA((token_t *) o));
            break;
        case stoken_type:
            str_sprintf(s, "%s", STOKEN_DATA((stoken_t *) o));
            break;
        case cfn_type:
            str_sprintf(s, "<cfn %zx>", ((cfn_t *)o)->ptr);
            break;
        default:
            str_sprintf(s, "[id=%d,type=%d,ptr=%016zx]", O_ID(o), O_TYPE(o), (size_t)o);
        }
    } else {
        // remaining tagged pointers
        // 0 = cons, 1-3 fixnum, ...
        //str_sprintf(s, "%016x", ((size_t) o) & ~3);
        switch (TAG(o)) {
        case fixnum_tag:
            //str_sprintf(s, "%d<%zd>", (int) TAG(o), immediate(o));
            str_sprintf(s, "%d<%lld>", (int) TAG(o), immediate(o));
            break;
        case fn_tag:
            str_sprintf(s, "<lambda %016zx>", (size_t)o);
            break;
        default:
            str_sprintf(s, "%016zx", (size_t)o);
        }
    }

    return s;
}

void print_obj(object_t *o)
{
    printf("%s", sprint_obj(NULL, o)->data);
}

// callback - used for regular list
// prints (1 2 3 ...[ . z]) and not (1 (2 (3 ...)))
int dump_car(cons_t *c, str_t **s)
{
    *s = sprint_obj(*s, c->car);

    if (nilp(c->cdr))
        ; // end of list
    else if (!listp(c->cdr)) {
        // last cdr has a value
        str_sprintf(*s, " . ");
        sprint_obj(*s, c->cdr);
    } else // (listp(c->cdr))
        // next yield
        str_sprintf(*s, " ");

    return 0;
}
