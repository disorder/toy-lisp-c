#ifndef __OBJECT_H
#define __OBJECT_H

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "common.h"

typedef enum {
    cons_type = 0,
    str_type, // = 1, // bytestring
    token_type, // = 2, // immutable with size data allocated after struct
    stoken_type, // = 3, // based on string
    cfn_type, // = 4, // cons like lambda but for wrapping hardcoded function
    object_type
} TYPE;

typedef enum {
    obj_tag = 0,
    fixnum_tag = 1,
    fn_tag = 2,
    reserved_tag = 3,
    tag_count
} TAG_VALUE;

#define CHECK_PTR(ptr) assert(!TAG(ptr))

// 2 bits
// non-object_t or special data
#define TAG(ptr) ((size_t) ptr & 3)
// now cons_t is tag=0 and IMM
//#define IMM(object) ((size_t) object & 3)
// not going through with tagging
//#define is_obj(ptr) (obj_tag == TAG(ptr))
#define is_obj(ptr) (TAG(ptr) == 0)
// we can do fixnum math without this, just need to mask 0x3
#define immediate(ptr) ((ssize_t) ptr >> 2)
#define is_fixnum(ptr) (TAG(ptr) == 1)
#define is_fn(ptr) (TAG(ptr) == 2)
#define clr_tag(ptr) ((fixnum_t) ptr & ~((fixnum_t)3))
typedef ssize_t fixnum_t;
#define to_fixnum(value) ((value << 2) | fixnum_tag)
#define to_fn(ptr) ((size_t) ptr | fn_tag)

typedef struct {
    u32 type;
    u32 id;
} instance_t;

// not going through with tagging objects
//#define O(ptr, TYPE) ((TYPE##_t *) ((size_t) ptr & ~3))
//#define OBJ(ptr, FIELD) (O(ptr, object))->obj.FIELD
// this is not a predicate, only field shortcut
#define OBJ(ptr, FIELD) ptr->obj.FIELD
//#define O_TYPE(object) OBJ(object, type)
#define O_TYPE(ptr) (listp(ptr)? cons_type : OBJ(ptr, type))
//#define O_ID(object) OBJ(object, id)
#define O_ID(ptr) (listp(ptr)? 0 : OBJ(ptr, id))

// for object_t types, only instantiated (not NIL)
#define is_a(TYPE, ptr) (!nilp(ptr) && is_obj(ptr) && TYPE##_type == O_TYPE(ptr))
// includes NIL
#define listp(ptr) (nilp(ptr) || (is_obj(ptr) && cons_type == ptr->obj.type))

// generic object which exposes common instance structure
typedef struct
{
    instance_t obj;
} object_t;

extern u32 instance_id;
#define new_instance ++instance_id

object_t *_new(TYPE type, size_t size);

//#define new(TYPE, NAME)                                 \
//    TYPE##_t *NAME = calloc(1, sizeof(TYPE##_t));       \
//    NAME->obj.id = new_instance;                       \
//    NAME->obj.type = TYPE##_type;
#define NEW(TYPE, NAME) TYPE##_t *NAME = _new(TYPE##_type, sizeof(TYPE##_t))

//object_t *_alloc(size_t size);
//#define alloc(TYPE, NAME) TYPE *NAME = calloc(1, sizeof(TYPE))
//#define alloc(TYPE, NAME) TYPE *NAME = _alloc(sizeof(TYPE))

#endif
