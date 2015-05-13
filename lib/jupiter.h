#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// VERSION  0.0.7

typedef    void* juc;
typedef int32_t  ju_int;
typedef  double  ju_real;

struct ju_obj;

typedef struct ju_obj {

	ju_int nmems;
	ju_int tag;
	struct {
		int color;
		struct ju_obj* prev;
		struct ju_obj* next;
	} gc_info;
	juc mems[0];

} ju_obj;


#define ju_null   ((juc) 0x0)
#define ju_zero   ((juc) 0x1)
#define ju_one    ((juc) 0x3)
#define ju_true   ju_zero
#define ju_false  ju_null
#define ju_unit   ju_null


void   ju_init ();
void   ju_destroy ();

void   juGC_init ();
void   juGC_destroy ();
void   juGC_sweep ();
void   juGC_begin ();
void   juGC_step ();
void   juGC_end ();
void   juGC_root (juc* root);
void   juGC_unroot (int n);
void   juGC_store (juc* root, juc value);

ju_int ju_get_tag (juc cell);
bool   ju_is_gc (juc cell);
bool   ju_is_int (juc cell);
#define ju_is_obj(_c) !ju_is_int(_c)

ju_int ju_to_int (juc cell);
bool   ju_to_bool (juc cell);
juc    ju_from_int (ju_int i);
juc    ju_from_bool (bool b);

juc    ju_make_buf (ju_int tag, size_t aug, ju_int nmems, ...);
#define ju_make(tag, ...) ju_make_buf(tag, 0, __VA_ARGS__)
juc    ju_make_str (const char* buf, size_t size);
juc    ju_make_real (ju_real r);

juc    ju_get (juc obj, ju_int i);
juc    ju_safe_get (juc obj, char* tagname, ju_int tag, ju_int i);
char*  ju_get_buffer (juc obj);
size_t ju_get_length (juc obj);
ju_real ju_get_real (juc obj);

