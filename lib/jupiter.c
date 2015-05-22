#include "jupiter.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// TODO: hash function in compiler AND runtime
#define JU_TAG_STR     0x1
#define JU_TAG_REAL    0x2
#define JU_TAG_CLOSURE 0x3


// utilities
bool ju_is_gc (juc cell)
{
	return cell != NULL && ju_is_obj(cell);
}
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
ju_int ju_get_tag (juc cell)
{
	if (ju_is_int(cell) || cell == ju_null)
		return ju_to_int(cell);
	else
		return ((ju_obj*) cell)->tag;
}
char* ju_get_buffer (juc cell)
{
	if (cell == ju_null || ju_is_int(cell))
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
ju_fnp ju_get_fn (juc obj)
{
	return *((ju_fnp*) ju_get_buffer(obj));
}



// global runtime management
void ju_init ()
{
	juGC_init();
}
void ju_destroy ()
{
	juGC_destroy();
}







// object management

static juc ju_vmake (ju_int tag, size_t aug, ju_int nmems, va_list vl)
{
	ju_obj* obj = malloc(sizeof(ju_obj) + nmems * sizeof(juc) + aug);

	obj->nmems = nmems;
	obj->tag = tag;
	juGC_init_obj(obj);

	ju_int i;
	for (i = 0; i < nmems; i++)
		obj->mems[i] = va_arg(vl, juc);

	return (juc) obj;
}

juc ju_make_buf (ju_int tag, size_t aug, ju_int nmems, ...)
{
	// very small objects are represented as just their tags
	if (aug == 0 && nmems == 0)
		return ju_from_int(tag);

	va_list vl;
	juc res;

	va_start(vl, nmems);
	res = ju_vmake(tag, aug, nmems, vl);
	va_end(vl);

	return res;
}

juc ju_make_str (const char* buf, size_t size)
{
	juc obj = ju_make_buf(JU_TAG_STR, size, 1, ju_from_int((ju_int) size));

	if (buf != NULL)
		memcpy(ju_get_buffer(obj), buf, size);
	
	return obj;
}

juc ju_make_real (ju_real r)
{
	juc obj = ju_make_buf(JU_TAG_REAL, sizeof(ju_real), 0);
	*((ju_real*) ju_get_buffer(obj)) = r;
	return obj;
}

juc ju_closure (ju_fnp fn, ju_int nmems, ...)
{
	va_list vl;
	juc obj;

	va_start(vl, nmems);
	obj = ju_vmake(JU_TAG_CLOSURE, sizeof(ju_fnp), nmems, vl);
	va_end(vl);

	*((ju_fnp*) ju_get_buffer(obj)) = fn;
	return obj;
}






juc ju_get (juc cell, ju_int i)
{
	if (cell == ju_null || ju_is_int(cell))
		// TODO: die here instead?
		return ju_null;

	ju_obj* const obj = cell;

	if (i < 0 || i >= obj->nmems)
		return ju_null;

	return obj->mems[i];
}
juc ju_safe_get (juc cell, char* tagname, ju_int tag, ju_int i)
{
	if (ju_get_tag(cell) != tag)
	{
		fprintf(stderr, "RUNTIME ERROR: expected object tag \"%s\"\n", tagname);
		exit(1);
	}

	return ju_get(cell, i);
}
