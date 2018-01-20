#ifndef __CONS_H
#define __CONS_H

#include "common.h"
#include "object.h"
#include "str.h"

extern size_t consed;

typedef struct {
    u32 type;
} tag_t;

typedef struct cons_t {
    // I wanted to use tag=0 for cons and something else for objects as
    // that would allow us to get rid of this extra data but it would affect
    // every function entry, structure access, return... and remove type
    // checking since helper macros would just cast to what we want and
    // disregard programming errors.
    //
    // Probably easier to do with objects having tag=0 and cons having tag.
    // We could just fix all ->car/->cdr accesses with function.  It would
    // be less prone to crases if there is bug in code - more stable but could
    // mask some programming errors.  Also we could differentiate between
    // NIL and NULL if that's worth anything, not sure.
    //
    // One more cons - using this type system with any other C code would
    // require that code to do the same.  non-object_t data in conses
    // still needs to be tagged.
    //
    // We can add smaller tag here since instance_t for cons is not useful.
    //instance_t obj;
    // this will match access to instance_t for all object_t type checks
    // but shaves off all other fields - just be sure to not access anything
    // else but type
    tag_t obj;
    struct cons_t *car;
    struct cons_t *cdr;
} cons_t;

typedef object_t *(*fn_t)(const char *name, cons_t *args);
typedef struct {
    tag_t obj;
    fn_t ptr;
} cfn_t;

// should this be cons_t or object_t
#define NIL ((cons_t *) NULL)
#define nilp(obj) ((void*) obj == (void*) NIL)
object_t *car(cons_t *c);
cons_t *cdr(cons_t *c);
int length(cons_t *c);
object_t *nth(u32 n, cons_t *c);
#define cadr(c) nth(1, c)
#define caddr(c) nth(2, c)
#define cadddr(c) nth(3, c)
#define caar(c) car(car(c))
#define caaar(c) car(car(car(c)))
#define caaaar(c) car(car(car(car(c))))
#define cddr(c) cdr(cdr(c))
#define cdddr(c) cdr(cdr(cdr(c)))
#define cddddr(c) cdr(cdr(cdr(cdr(c))))

cons_t *cons(object_t *car, object_t *cdr);
cons_t *last(cons_t *this);

//#define APPEND(c, o) cons_append((cons_t *) c, (object_t *) o)
// destructive
cons_t *cons_append(cons_t *c, object_t *obj);

cons_t *iterate_list(cons_t *this, int (*yield)(cons_t *, object_t *),
                     void *data);

// callbacks
int dump_car(cons_t *c, str_t **s);
int print_cons(cons_t *cons, void *data);
#define dump_obj(obj) sprint_obj(NULL, obj)->data

str_t *sprint_obj(str_t *s, object_t *o);
void print_obj(object_t *o);
void print_list(cons_t *c);

#endif
