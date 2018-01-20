
//callback
/*
int cb_add(cons_t *c, fixnum_t *result)
{
    if (nilp(c->car) || !is_fixnum(c->car)) {
        raise("+: not a number: %s", dump_obj(c->car));
    }

    fixnum_t value = clr_tag((fixnum_t) c->car);
    // don't need to shift every number, just add with cleared tag
    *result += clr_tag(value);
    return 0;
}

object_t *fn_add(const char *name, cons_t *args)
{
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);

    fixnum_t result = 0;
    iterate_list(args_eval, &cb_add, &result);

    printf("%s %s = %s\n", name, dump_obj(args_eval),
           dump_obj((object_t *) (result | fixnum_tag)));
    return (object_t *) (result | fixnum_tag);
}
*/

// TODO MINARG implement: if (minarg && length < minarg) raise
#define MATHFN(OP, CSTR, INIT, MINARG) \
int cb_ ##CSTR (cons_t *c, fixnum_t *result) \
{ \
    if (nilp(c->car) || !is_fixnum(c->car)) { \
        raise(#CSTR": not a number: %s", dump_obj(c->car));   \
    } \
\
    fixnum_t value = immediate((fixnum_t) c->car); \
    *result OP##= value; \
    return 0; \
} \
\
object_t *fn_##CSTR(const char *name, cons_t *args) \
{ \
    /* eval args */          \
    cons_t *args_eval = NIL; \
    eval_code(args, &args_eval); \
\
    fixnum_t result = INIT; \
    iterate_list(args_eval, &cb_##CSTR, &result);\
\
    /*printf("%s %s = %s\n", name, dump_obj(args_eval),*/ \
    /*       dump_obj((object_t *) (result | fixnum_tag)));*/ \
    return (object_t *) to_fixnum(result);  \
}

MATHFN(+, add, 0, 0)
MATHFN(-, sub, 0, 1)
MATHFN(*, mul, 1, 0)
MATHFN(/, div, 1, 1)
