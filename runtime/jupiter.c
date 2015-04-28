#include "jupiter.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

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


// global runtime management
void ju_init ()
{
	juGC_init();
}
void ju_destroy ()
{
	juGC_destroy();
}




// tri-color marking garbage collector
#define WHITE   0
#define GREY    1
#define BLACK   2

static struct
{
	juc** list;
	size_t size, cap;
} gc_roots;

ju_obj gc_sets[3];
bool gc_began;
size_t gc_nobjs;

#define first(col) gc_sets[col].gc_info.next


static void move_to_set (ju_obj* obj, ju_obj* set)
{
	ju_obj* prev = obj->gc_info.prev;
	ju_obj* next = obj->gc_info.next;
	ju_obj* set_next = set->gc_info.next;

	if (prev != NULL)
	{
		prev->gc_info.next = next;
		if (next != NULL)
			next->gc_info.prev = prev;
	}

	set->gc_info.next = obj;
	obj->gc_info.prev = set;

	obj->gc_info.next = set_next;
	if (set_next != NULL)
		set_next->gc_info.prev = obj;
}
static void make_white (ju_obj* obj)
{
	obj->gc_info.color = WHITE;
	move_to_set(obj, &gc_sets[WHITE]);
}
static void make_grey (ju_obj* obj)
{
	if (obj->gc_info.color == WHITE)
	{
		obj->gc_info.color = GREY;
		move_to_set(obj, &gc_sets[GREY]);
	}
}
static void make_black (ju_obj* obj)
{
	if (obj->gc_info.color == GREY)
	{
		obj->gc_info.color = BLACK;
		move_to_set(obj, &gc_sets[BLACK]);

		ju_int i, nmems;
		for (i = 0, nmems = obj->nmems; i < nmems; i++)
			if (ju_is_gc(obj->mems[i]))
				make_grey((ju_obj*) obj->mems[i]);
	}
}


static void juGC_init_obj (ju_obj* obj)
{
	juGC_sweep();

	gc_nobjs++;
	obj->gc_info.prev =
	obj->gc_info.next = NULL;
	make_white(obj);
}





void juGC_init ()
{
	gc_roots.size = 0;
	gc_roots.cap = 64;
	gc_roots.list = malloc(sizeof(juc*) * gc_roots.cap);

	gc_sets[0].gc_info.next =
	gc_sets[0].gc_info.prev =
	gc_sets[1].gc_info.next =
	gc_sets[1].gc_info.prev =
	gc_sets[2].gc_info.next =
	gc_sets[2].gc_info.prev = NULL;

	gc_began = false;
	gc_nobjs = 0;
}
void juGC_destroy ()
{
	free(gc_roots.list);
	gc_roots.size =
	gc_roots.cap = 0;

	juGC_sweep();
	if (gc_nobjs > 0)
		fprintf(stderr, "WARNING: not all objects freed upon juGC_destroy()\n");
}
void juGC_root (juc* root)
{
	if (gc_roots.size >= gc_roots.cap)
	{
		gc_roots.cap *= 2;
		gc_roots.list = realloc(gc_roots.list,
		                  sizeof(juc*) * gc_roots.cap);
	}

	gc_roots.list[gc_roots.size++] = root;
}
void juGC_unroot (int n)
{
	if (gc_roots.size < n)
		gc_roots.size = 0;
	else
		gc_roots.size -= n;
}
void juGC_store (juc* root, juc value)
{
	*root = value;
	if (gc_began)
		if (ju_is_gc(value))
			make_grey((ju_obj*) value);
}


void juGC_sweep ()
{
	if (gc_began) return;

	// TODO: interleave juGC_step() calls with execution flow
	juGC_begin();
	while (gc_began)
		juGC_step();
}
void juGC_begin ()
{
	gc_began = true;

	// each object is colored white at this point
	size_t i;
	juc cell;

	// mark each object in the root set
	for (i = 0; i < gc_roots.size; i++)
	{
		cell = *gc_roots.list[i];

		if (ju_is_gc(cell))
			make_grey((ju_obj*) cell);
	}
}
void juGC_step ()
{
	if (first(GREY) != NULL)
		make_black(first(GREY));
	else
		juGC_end();
}
void juGC_end ()
{
	size_t freed = 0;
	ju_obj* obj, * next;

	// remove remaining white objects
	for (obj = first(WHITE); obj != NULL; obj = next)
	{
		next = obj->gc_info.next;
		free(obj);
		freed++;
	}
	first(WHITE) = NULL;

	// color black objects white
	while (first(BLACK) != NULL)
		make_white(first(BLACK));

	gc_began = false;

//	fprintf(stderr, "* freed %d/%d   %d roots\n",
//		freed, gc_nobjs, gc_roots.size);
	gc_nobjs -= freed;
}









// object management
juc ju_make_buf (ju_int tag, size_t aug, ju_int nmems, ...)
{
	ju_obj* obj = malloc(sizeof(ju_obj) + nmems * sizeof(juc) + aug);

	obj->nmems = nmems;
	obj->tag = tag;
	juGC_init_obj(obj);

	ju_int i;
	va_list vl;
	
	va_start(vl, nmems);
	for (i = 0; i < nmems; i++)
		obj->mems[i] = va_arg(vl, juc);
	va_end(vl);

	return (juc) obj;
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