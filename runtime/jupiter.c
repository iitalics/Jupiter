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
juc ju_make (ju_int tag, ju_int nmems, ...)
{
	ju_obj* obj = malloc(sizeof(ju_obj) + nmems * sizeof(juc));

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