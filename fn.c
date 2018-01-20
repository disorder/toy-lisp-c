
// (list [expression1 ...]) => nil/cons_t
object_t *fn_list(const char *name, cons_t *args)
{
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return args_eval;
}

// callback
int cb_print(cons_t *c, void *data)
{
    object_t *value = c->car;

    if (is_obj(value) && str_type == O_TYPE(value))
        printf("%s", ((str_t *) value)->data);
    else
        print_obj(value);
    // TODO only in between
    putchar(' ');
    // continue iteration
    return 0;
}

// (print [expression1 ...]) => NIL
object_t *fn_print(const char *name, cons_t *args)
{
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);

    iterate_list(args_eval, &cb_print, NULL);
    return NIL;
}

// callback
int cb_puts(cons_t *c, void *data)
{
    object_t *value = c->car;

    if (is_obj(value) && str_type == O_TYPE(value))
        printf("%s", ((str_t *) value)->data);
    else
        print_obj(value);

    // TODO only in between
    putchar('\n');
    // continue iteration
    return 0;
}

// (puts [expression1 ...]) => NIL
object_t *fn_puts(const char *name, cons_t *args)
{
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);

    iterate_list(args_eval, &cb_puts, NULL);

    return NIL;
}

// (progn [expression1 ...]) => NIL/evaluation of last expression
object_t *fn_progn(const char *name, cons_t *args)
{
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return car(last(args_eval));
}

// expecting normal fn_ args - name and args, declares int argc
#define ARGC(cnt)                                                       \
    int argc = length(args);                                            \
    if (argc != cnt)                                                    \
        raise("%s: expected " #cnt " arguments, got %d",                \
              name, argc)
#define ARGCr(min, max)                                                 \
    int argc = length(args);                                            \
    if (argc < min || argc > max)                                       \
        raise("%s: expected " #min "-" #max " arguments, got %d",       \
              name, argc);
#define ARGCm(min)                                                      \
    int argc = length(args);                                            \
    if (argc < min)                                                     \
        raise("%s: expected at least " #min " arguments, got %d",       \
              name, argc)

// (if cond then_expression [else_expression])
object_t *fn_if(const char *name, cons_t *args)
{
    ARGCr(2,3);

    object_t *cond = car(args);
    object_t *then = cadr(args);
    object_t *otherwise = caddr(args);

    if (eval_token(cond))
        return eval_token(then);
    return eval_token(otherwise);
}

// (boundp defvar-name) => NIL/1
object_t *fn_boundp(const char *name, cons_t *args)
{
    ARGC(1);
    token_t *var = args->car;
    assert(is_a(token, var));

    cons_t *frame = getvar(TOKEN_DATA((token_t *) var), NULL);
    if (!nilp(frame))
        // TODO T?
        return (object_t *) to_fixnum(1);
    return NIL;
}

// (not expression) => NIL/1
object_t *fn_not(const char *name, cons_t *args)
{
    ARGC(1);
    object_t *value =  eval_token(args->car);

    if (!nilp(value))
        // TODO T?
        return (object_t *) to_fixnum(1);

    return NIL;
}

// (funcall funcallable [arg1 ...]) => evaluation of funcall
object_t *fn_funcall(const char *name, cons_t *args)
{
    ARGCm(1);
    // (fn arg1 arg2 ...)
    object_t *fn = eval_token(car(args));
    cons_t *call = cons(fn, cdr(args));
    return eval_sexp(call);
}

typedef struct {
    object_t *fn;
    cons_t *results;
} map_fn_t;

// callback
int cb_map(cons_t *c, map_fn_t *spec)
{
    cons_t *call = cons(spec->fn, cons(c->car, NIL));
    object_t *result = fn_funcall(NULL, call);
    spec->results = cons_append(spec->results, result);
    return 0;
}

// (mapcar funcallable list) => list of funcall results
object_t *fn_mapcar(const char *name, cons_t *args)
{
    ARGC(2);

    // eval fn+args
    object_t *fn = eval_token(args->car);
    cons_t *list = eval_token(cadr(args));

    map_fn_t spec = {
        .fn = fn,
        .results = NIL,
    };
    iterate_list(list, &cb_map, &spec);
    return spec.results;
}

// (function token/lambda) => funcallable
object_t *fn_function(const char *name, cons_t *args)
{
    ARGC(1);
    token_t *fn_name = args->car;

    if (is_a(token, fn_name)) {
        void *ptr = getfun(TOKEN_DATA(fn_name));
        return (object_t *) ptr;
    }

    // TODO this can be a variable?
    return eval_sexp(fn_name);
}

// (reduce funcallable list)
// returns for list length:
// 0: (fn)
// 1: arg1
// 2: (fn #1   arg2)
// 3: (fn #2   arg3)
// N: (fn #N-1 argN)
object_t *fn_reduce(const char *name, cons_t *args)
{
    ARGC(2);

    object_t *fn = eval_token(car(args));
    cons_t *result = NIL;
    cons_t *c = cdr(args);
    if (nilp(c)) {
        cons_t *call = cons(fn, NIL);
        result = fn_funcall(NULL, call);
    } else {
        result = car(c);
        while (!nilp(cdr(c))) {
            cons_t *call = cons(fn, NIL);
            c = cdr(c);
            call->cdr = cons(result, cons(car(c), NIL));
            result = fn_funcall(NULL, call);
        }
    }
    return result;
}

// (apply funcallable [arg1 ...] list)
object_t *fn_apply(const char *name, cons_t *args)
{
    ARGCm(2);

    // eval fn+args
    object_t *fn = eval_token(args->car);
    cons_t *args_eval = NIL;
    eval_code(args->cdr, &args_eval);

    cons_t *call = cons(fn, NIL);
    cons_t *c = args_eval;
    while (!nilp(c)) {
        if (!nilp(cdr(c))) {
            cons_append(call, c->car);
            c = cdr(c);
        } else {
            // last
            c = c->car;

            if (!listp(c))
                raise("%s: last argument has to be a list", name);

            while (!nilp(c)) {
                cons_append(call, c->car);
                c = cdr(c);
            }
            break;
        }
    }

    // TODO real pointer does not go well with fn_funcall
    debug("call %s\n", dump_obj(call));
    return fn_funcall(NULL, call);
}

// (backtrace) => str_t
object_t *fn_backtrace(const char *name, cons_t *args)
{
    ARGC(0);
    str_t *s = str_sprintf(NULL, "Call stack:\n");
    cons_t *frames = stack;
    while (!nilp(frames)) {
        cons_t *frame = car(frames);
        frames = cdr(frames);

        token_t *name = car(frame);
        cons_t *vars = cdr(frame);
        str_sprintf(s, "\t%s\n", dump_obj(name));
        while (!nilp(vars)) {
            cons_t *var = car(vars);
            vars = cdr(vars);

            cons_t *name = car(var);
            cons_t *value = cdr(var);
            str_sprintf(s, "\t\t%s\t%s\n", dump_obj(name), dump_obj(value));
        }
    }
    return s;
}

// (cons a b) => cons_t
object_t *fn_cons(const char *name, cons_t *args)
{
    ARGC(2);
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return cons(args_eval->car, args_eval->cdr->car);
}

// (mod a b) => fixnum_t
object_t *fn_mod(const char *name, cons_t *args)
{
    ARGC(2);
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);

    fixnum_t a = (fixnum_t) args_eval->car;
    fixnum_t b = (fixnum_t) args_eval->cdr->car;
    assert(is_fixnum(a));
    assert(is_fixnum(b));
    return (object_t *) to_fixnum(immediate(a) % immediate(b));
}

// TODO this is just for fixnums
// (= a b) => NIL/1
object_t *fn_eq(const char *name, cons_t *args)
{
    ARGC(2);
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);

    fixnum_t a = (fixnum_t) args_eval->car;
    fixnum_t b = (fixnum_t) args_eval->cdr->car;
    assert(is_fixnum(a));
    assert(is_fixnum(b));
    if (a == b)
        return (object_t *) to_fixnum(1);
    return NIL;
}

// (exit [code])
object_t *fn_exit(const char *name, cons_t *args)
{
    ARGCr(0,1);
    int code = 0;
    if (argc > 0) {
        cons_t *args_eval = NIL;
        eval_code(args, &args_eval);
        code = (int) immediate(args_eval->car);
    }

    exit(code);
    return NIL;
}

// (car list)
object_t *fn_car(const char *name, cons_t *args)
{
    ARGC(1);
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return caar(args_eval);
}

// (cdr list)
object_t *fn_cdr(const char *name, cons_t *args)
{
    ARGC(1);
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return cdr(car(args_eval));
}

// (nth fixnum list)
object_t *fn_nth(const char *name, cons_t *args)
{
    ARGC(2);
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    assert(is_fixnum(car(args_eval)));
    return nth(immediate(car(args_eval)), cadr(args_eval));
}

// (length list) => fixnum_t
object_t *fn_length(const char *name, cons_t *args)
{
    ARGC(1);
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return (object_t *) to_fixnum(length(car(args_eval)));
}

// (last list) => list
object_t *fn_last(const char *name, cons_t *args)
{
    ARGC(1);
    // eval args
    cons_t *args_eval = NIL;
    eval_code(args, &args_eval);
    return last(last(car(args_eval)));
}
