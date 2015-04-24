#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef    void*  juc;
typedef int32_t  ju_int;

typedef struct {

	ju_int nmems;
	ju_int tag;
	struct { } gc_info;
	juc mems[0];

} ju_obj;


#define ju_null   ((juc) NULL)
#define ju_zero   ((juc) 0x1)
#define ju_one    ((juc) 0x3)
#define ju_true   ju_one
#define ju_false  ju_null
#define ju_unit   ju_null


void   ju_init ();
void   ju_destroy ();

bool   ju_is_int (juc cell);
#define ju_is_obj(_c) !ju_is_int(_c)

ju_int ju_to_int (juc cell);
bool   ju_to_bool (juc cell);
juc    ju_from_int (ju_int i);
juc    ju_from_bool (bool b);

juc    ju_make_buf (ju_int tag, size_t aug, ju_int nmems, ...);
#define ju_make(tag, ...) ju_make_buf(tag, 0, __VA_ARGS__)
juc    ju_make_str (const char* buf, size_t size);

juc    ju_get (juc obj, ju_int i);
char*  ju_get_buffer (juc obj);
size_t ju_get_length (juc obj);





juc juStd_addInt (juc a, juc b);
juc juStd_negInt (juc a);
juc juStd_ltInt (juc a, juc b);
juc juStd_eqInt (juc a, juc b);
juc juStd_println (juc a);



