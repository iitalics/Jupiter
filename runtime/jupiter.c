#include "jupiter.h"
#include <string.h>
#include <stdarg.h>

// utilities
bool ju_is_int (juc cell)
{
	return ((ju_int)cell) & 1;
}
ju_int ju_to_int (juc cell)
{
	return ((ju_int)cell) >> 1;
}
bool ju_to_bool (juc cell)
{
	return cell != NULL;
}
juc ju_from_int (ju_int i)
{
	return (juc) ((i << 1) | 1);
}
juc ju_from_bool (bool b)
{
	return b ? ju_true : ju_false;
}


// global runtime management
void ju_init () {}
void ju_destroy () {}


// object management
juc ju_make_buf (ju_int tag, size_t aug, ju_int nmems, ...)
{
	ju_obj* obj = malloc(sizeof(ju_obj) + nmems * sizeof(juc) + aug);

	obj->nmems = nmems;
	obj->tag = tag;
	// obj->gc_info...

	ju_int i;
	va_list vl;
	
	va_start(vl, nmems);
	for (i = 0; i < nmems; i++)
		obj->mems[i] = va_arg(vl, juc);
	va_end(vl);

	return obj;
}

juc ju_make_str (const char* buf, size_t size)
{
	juc obj = ju_make_buf(0, size, 1, ju_from_int((ju_int) size));
	memcpy(ju_get_buffer(obj), buf, size);
	return obj;
}

juc ju_make_real (ju_real r)
{
	juc obj = ju_make_buf(0, sizeof(ju_real), 0);
	*((ju_real*) ju_get_buffer(obj)) = r;
	return obj;
}

juc ju_get (juc cell, ju_int i)
{
	if (cell == NULL || ju_is_int(cell))
		// TODO: die here instead?
		return ju_null;

	ju_obj* const obj = cell;

	if (i < 0 || i >= obj->nmems)
		return ju_null;

	return obj->mems[i];
}

char* ju_get_buffer (juc cell)
{
	if (cell == NULL || ju_is_int(cell))
		return NULL;

	ju_obj* const obj = cell;

	// memory located after sub-members
	return (char*)(obj->mems + obj->nmems);
}

size_t ju_get_length (juc obj)
{
	return ju_to_int(ju_get(obj, 0));
}

ju_real ju_get_real (juc obj)
{
	return *((ju_real*) ju_get_buffer(obj));
}