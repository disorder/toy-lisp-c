#include "object.h"
#include "cons.h"

u32 instance_id = 0;

object_t *_new(TYPE type, size_t size)
{
    object_t *o = calloc(1, size);
    if ((void *)o == NULL) {
        raise("new object %d: could not allocate %zu", type, size);
        return NULL;
    }

    o->obj.id = new_instance;
    o->obj.type = type;
    //debug("new %d ########################## %016zx\n", type, (size_t)o);
    return o;
}

/*
object_t *_alloc(size_t size)
{
    // TODO failure and TYPE needs to be set!
    object_t *o = calloc(1, size);
    if ((void *)o == NULL) {
        raise("could not allocate %d", size);
        return NULL;
    }

//    o->obj.id = new_instance;
    //o->obj.type = object_type;
    o->obj.type = object_type;
    debug("alloc########################## %016x\n", o);
    return o;
}
*/
