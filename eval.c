#include "eval.h"

#include <assert.h>
#include <strings.h>

// beware of code alignment, asserting every constructor, hardcoded and lambda
#define CHECK_PTR(ptr) assert(!TAG(ptr))

// list where car = (name . object)
// primitive, slow hash
// global variables
// beware - we are not tagging pointers to functions so never print this!
//          pointers in (name . ptr) car will be assumed to be cons_t
//          we should either tag or create _t
cons_t *funs = NIL;
cons_t *stack = NIL;

#include "math.c"
#include "fn.c"

void bind(cons_t *arglist, cons_t *args)
{
    cons_t **env = &CTX_VARS;
    while (!nilp(arglist)) {
        assert(!nilp(args));
        *env = def(*env, car(arglist), car(args));
        // next
        arglist = cdr(arglist);
        args = cdr(args);
    }
}

cons_t *def(cons_t *defs, token_t *name, object_t *obj)
{
    // let's only accept immutable token_t
    if (!is_a(token, name)) {
        raise("trying to define a non-token name: %s", dump_obj(name));
    }

    cons_t *var = cons((object_t *) name, (object_t *) obj);
    defs = cons_append(defs, var);
    return defs;
}

// callback
int def_cmp(cons_t *c, const char *data)
{
    assert(!nilp(c->car));
    assert(listp(c->car));
    assert(!nilp(c->car->car));
    assert(is_obj(c->car->car));
    //assert(!listp(c->car->car));
    //assert(token_type == O_TYPE(c->car->car));
    assert(token_type == O_TYPE(caar(c)));
    return 0 == strcasecmp(data, (char *) TOKEN_DATA((token_t *) c->car->car));
}

cons_t *def_find(cons_t *defs, const char *s)
{
    cons_t *result = iterate_list(defs, &def_cmp, (void *) s);
    if (nilp(result))
        return NIL;
    // return definition
    return result->car;
}

// callback
int cb_eval(cons_t *c, cons_t **results)
{
    // discarding results
    object_t *value = eval_token(c->car);
    debug("\nARGeval %s=%s\n", dump_obj(c->car), dump_obj(value));
    if (results != NULL)
        *results = cons_append(*results, value);

    return 0;
}

void eval_code(cons_t *code, cons_t **results)
{
    debug("STACK=%s\n", dump_obj(stack));
    iterate_list(code, &cb_eval, results);
}

// note that we will allow to redeclare with new value in same context
object_t *fn_defvar(const char *fn, cons_t *args)
{
    // TODO check for argc=2
    token_t *name = car(args);
    assert(is_a(token, name));
    object_t *token = cadr(args);

    debug("defvar %s with %s\n", TOKEN_DATA(name),dump_obj(token));
    // no more args
    if (!nilp(args->cdr->cdr)) {
        raise("defvar: too many arguments");
    }

    object_t *value = eval_token(token);
    // current env
    CTX_VARS = def(CTX_VARS, name, value);

    debug("defvar %s=%s", dump_obj(name), dump_obj(value));
    return value;
}

object_t *fn_setq(const char *fn, cons_t *args)
{
    // TODO check for argc=2
    token_t *name = car(args);
    assert(is_a(token, name));
    object_t *token = cadr(args);

    debug("setq %s with %s\n", TOKEN_DATA(name), dump_obj(token));
    cons_t *def = getvar(TOKEN_DATA(name), NULL);
    if (nilp(def))
        raise("%s: undefined variable %s", fn, TOKEN_DATA(name));

    object_t *value = eval_token(token);
    def->cdr = value;
    return value;
}

object_t *fn_lambda(const char *fn, cons_t *args)
{
    // TODO check for argc=2
    // try to redo cons only with tag=0
    cons_t *arglist = car(args);
    assert(listp(arglist));
    token_t *body = cadr(args);

    cons_t *lambda = cons(arglist, body);
    CHECK_PTR(lambda);
    return (object_t *) to_fn(lambda);
}

object_t *eval_lambda(cons_t *lambda, cons_t *args, token_t *name)
{
    debug("fn pointer: %s\n%s\n", dump_obj(lambda),
           dump_obj((object_t *) clr_tag(lambda)));
    lambda = (cons_t *) clr_tag(lambda);
    cons_t *arglist = car(lambda);
    cons_t *body = cdr(lambda);

    debug("arglist: %s\nbody: %s\nargs: %s\n",
           dump_obj(arglist), dump_obj(body), dump_obj(args));

    int expected = length(arglist);
    int got = length(args);
    if (expected != got)
        raise("lambda expected %d arguments, got %d", expected, got);

    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);

    PUSH((name == NULL)? new_token_c("lambda", 0) : name);
    bind(arglist, args_eval);
    object_t *result = eval_token(body);
    POP;

    debug("result: %s\n", dump_obj(result));
    return result;
}

object_t *fn_defun(const char *fn, cons_t *args)
{
    token_t *name = (token_t *) car(args);
    assert(is_a(token, name));
    // TODO no argument is required!  it will crash... need car/cdr functions
    //      it will make things easier
    object_t *arglist = cadr(args);
    object_t *body = caddr(args);

    debug("defun %s with %s in env TODO\n", TOKEN_DATA(name), dump_obj(body));
    // no more args
    if (length(args) > 3) {
        raise("defun: too many arguments");
    }

    cons_t *lambda_args = cons(arglist, cons(body, NIL));
    object_t *lambda = fn_lambda(fn, lambda_args);
    funs = def(funs, name, lambda);

    debug("defun %s=%s\n", dump_obj(name), dump_obj(lambda));
    return lambda;
}

struct defun_init {
    char *name;
    // TODO rettype (*ptr)(fn, args)
    object_t *(*ptr)(const char *fn, cons_t *args);
} defun_inits[] = {
    /* like this
    {
        .name = "fn_...",
        .ptr = &fn_...,
    },
    */
    {
        .name = "+",
        // TODO should tag this as funcallable or something else
        .ptr = &fn_add,
    },
    {
        .name = "-",
        .ptr = &fn_sub,
    },
    {
        .name = "*",
        .ptr = &fn_mul,
    },
    {
        .name = "/",
        .ptr = &fn_div,
    },
    {
        .name = "mod",
        .ptr = &fn_mod,
    },
    {
        .name = "=",
        .ptr = &fn_eq,
    },
    {
        .name = "list",
        .ptr = &fn_list,
    },
    {
        .name = "car",
        .ptr = &fn_car,
    },
    {
        .name = "cdr",
        .ptr = &fn_cdr,
    },
    {
        .name = "nth",
        .ptr = &fn_nth,
    },
    {
        .name = "length",
        .ptr = &fn_length,
    },
    {
        .name = "last",
        .ptr = &fn_last,
    },
    {
        .name = "print",
        .ptr = &fn_print,
    },
    {
        .name = "puts",
        .ptr = &fn_puts,
    },
    {
        .name = "defvar",
        .ptr = &fn_defvar,
    },
    {
        .name = "let",
        .ptr = &fn_defvar,
    },
    {
        .name = "setq",
        .ptr = &fn_setq,
    },
    {
        .name = "defun",
        .ptr = &fn_defun,
    },
    {
        .name = "lambda",
        .ptr = &fn_lambda,
    },
    {
        .name = "progn",
        .ptr = &fn_progn,
    },
    {
        .name = "if",
        .ptr = &fn_if,
    },
    {
        .name = "boundp",
        .ptr = &fn_boundp,
    },
    {
        .name = "not",
        .ptr = &fn_not,
    },
    {
        .name = "function",
        .ptr = &fn_function,
    },
    {
        .name = "funcall",
        .ptr = &fn_funcall,
    },
    {
        .name = "mapcar",
        .ptr = &fn_mapcar,
    },
    {
        .name = "reduce",
        .ptr = &fn_reduce,
    },
    {
        .name = "apply",
        .ptr = &fn_apply,
    },
    {
        .name = "backtrace",
        .ptr = &fn_backtrace,
    },
    {
        .name = "cons",
        .ptr = &fn_cons,
    },
    {
        .name = "exit",
        .ptr = &fn_exit,
    },
};

//#define DEFUN(token, lambda) funs = def(funs, token, lambda)
// this only allows us C names
//#define DEFUN(NAME) funs = def(funs, new_token_c(#NAME, 0), &fn_##NAME)
// TODO useful for testing but not for evaluating
//#define DEFUN(NAME, FN) funs = def(funs, new_token_c(#NAME, 0), &FN)
#define DEFUN(NAME, FN) funs = def(funs, new_token_c(#NAME, 0), wrap_fn((void *) &FN))
// need to wrap hardcoded pointers
void *wrap_fn(void *ptr)
{
    if (is_fn(ptr)) {
        return ptr;
    }

    NEW(cfn, fn);
    fn->ptr = ptr;
    return fn;
}

int print_def(cons_t *c, void *data)
{
    /*
    assert(!nilp(c->car));
    assert(listp(c->car));
    assert(!nilp(c->car->car));
    assert(is_obj(c->car->car));
    assert(!listp(c->car->car));
    assert(token_type == O_TYPE(c->car->car));
    */
    debug("\t%s=%zx\n", TOKEN_DATA((token_t *) c->car->car),
           (size_t) c->car->cdr);
    return 0;
}

// fn_t for hardcoded or is_fn() pointer to lambda
// ^ this doesn't work well with evaluating, need to wrap with cfn_t
//   so we return fn_tagged cons_t* or cfn_t*
void *getfun(const char *name)
{
    cons_t *defun = def_find(funs, name);
    debug("FUNS %s\n", dump_obj(funs));
    if (nilp(defun))
        raise("unknown function %s", name);

    void *fn = defun->cdr;
    return fn;
}

object_t *eval_sexp(cons_t *sexp)
{
    debug("eval_sexp: %s\n", dump_obj(sexp));
    token_t *name = sexp->car;
    cons_t *args = sexp->cdr;
    object_t *fn = name;

    // name token, sexp
    if (is_a(token, name)) {
        // beware this might misinterpret the function pointer in cdr
        void *ptr = getfun(TOKEN_DATA(name));
        if (is_fn(ptr)) {
            debug("lambda fn+tag '%s' %d\n", dump_obj(name), (int) TAG(ptr));
            fn = ptr;
        } else if (is_a(cfn, ((object_t *) ptr))) {
            // we could tag this too and look at type of cons_t/cfn_t below
            debug("native fn '%s' %s\n", dump_obj(name), dump_obj(ptr));
            fn = ptr;
        } else {
            raise("unexpected function type for %s: %zx (tag %d)",
                  TOKEN_DATA(name), (size_t) ptr, (int) TAG(ptr));
        }
    } else if (is_a(cons, name)) {
        // could be (lambda ...)
        fn = eval_token((object_t *) name);
    }

    if (is_a(cfn, fn)) {
        void *ptr = fn;
        debug("\nexecuting '%s' at %zx\n", TOKEN_DATA(name),(size_t) name);
        //fn_t native_fn = (fn_t) ptr;
        fn_t native_fn = ((cfn_t *) ptr)->ptr;
        debug("%s\n", dump_obj(args));
        // they take care of args evaluating or creating context
        return native_fn(TOKEN_DATA(name), args);
    } else if (is_fn(fn))
        return eval_lambda(fn, args, is_a(token, name)? name : NULL);

    debug("%d", (int) TAG(fn));
    raise("not callable: %s=%s", dump_obj(name), dump_obj(fn));
    return NIL;
}

void *getvar(const char *name, object_t **value)
{
    cons_t *list = stack;
    //debug("STACK %s\n", dump_obj(stack));
    while (1) {
        cons_t *frame = car(list);
        token_t *stack_name = car(frame);
        debug("STACK %s", dump_obj(stack));
        assert(is_a(token, stack_name));
        cons_t *defs = cdr(frame);
        //debug("FIND %s in (%s) %s\n", name, TOKEN_DATA(stack_name), dump_obj(frame));
        cons_t *def = def_find(defs, name);
        if (!nilp(def)) {
            if (value != NULL)
                *value = def->cdr;
            return def;
        }

        if (nilp(cdr(list)))
            break;
        else
            list = cdr(list);
    }

    return NIL;
}

object_t *eval_token(object_t *token)
{
    // expecting: NIL, sexp (cons), evaluated objects, variable (token)
    if (nilp(token))
        return (object_t *) NIL;
    else if (listp(token))
        return eval_sexp(token);
    else if (TAG(token) || !is_a(token, token))
        // already evaluated from token
        return token;

    object_t *value = NIL;
    cons_t *def = getvar(TOKEN_DATA((token_t *) token), &value);
    if (nilp(def))
        raise("eval_token: unknown variable %s", TOKEN_DATA((token_t *) token));

    return value;
}

void init_eval()
{
    if (stack == NIL) {
        PUSH(new_token_c("global", 0));
    }

    if (!nilp(funs))
        return;

   for (int i=0; i<sizeof(defun_inits)/sizeof(struct defun_init); i++) {
       token_t *name = new_token_c(defun_inits[i].name, 0);
       info("defun %s at %zx\n", TOKEN_DATA(name),
              (size_t) defun_inits[i].ptr);
       CHECK_PTR(defun_inits[i].ptr);
       funs = def(funs, name, wrap_fn((void *) defun_inits[i].ptr));
   }
}

object_t *test_fn(const char *name, cons_t *args)
{
    printf("in %s\n", nilp(name)? "lambda" : name);
    printf("%s\n", dump_obj(args));
    return (object_t *) NIL;
}

void test_eval()
{
    DEFUN(test_fn, test_fn);

    cons_t *args = cons(new_str_c("args", 0),
                        cons(new_str_c("test", 0), NIL));
    cons_t *sexp = cons(new_token_c("test_fn", 0),
                        args);
    printf("%s\n", dump_obj(eval_sexp(sexp)));
    printf("%s\n", dump_obj(eval_token(sexp)));

    cons_t *args2 = cons((object_t *) to_fixnum(1),
                         cons((object_t *) to_fixnum(2), NIL));
    cons_t *sexp2 = cons(new_token_c("+", 0),
                         args2);
    printf("%s\n", dump_obj(eval_sexp(sexp2)));
    printf("%s\n", dump_obj(eval_token(sexp2)));

    cons_t *args3 = cons((object_t *) to_fixnum(1),
                         cons((object_t *) to_fixnum(2),
                              cons((object_t *) to_fixnum(3), NIL)));
    cons_t *sexp3 = cons(new_token_c("list", 0),
                         args3);
    printf("%s\n", dump_obj(eval_sexp(sexp3)));
    printf("%s\n", dump_obj(eval_token(sexp3)));
}
