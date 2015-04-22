#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef    void*  juc;
typedef intptr_t  ju_int;

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



void   ju_init ();
void   ju_destroy ();

bool   ju_is_int (juc cell);
#define ju_is_obj(_c) !ju_is_int(_c)

ju_int ju_to_int (juc cell);
bool   ju_to_bool (juc cell);
juc    ju_from_int (ju_int i);
juc    ju_from_bool (bool b);

juc    ju_make (ju_int tag, ju_int nmems, ...);
juc    ju_get (juc obj, ju_int i);