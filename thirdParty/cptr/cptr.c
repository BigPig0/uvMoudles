/*
 * Copyright (c) 2014 Moritz Bitsch <moritzbitsch@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "cptr.h"

#include <stdlib.h>

typedef struct _cptr cptr;
typedef struct _cptr_list cptr_list;

struct _cptr_list {
	cptr* head;
	cptr* tail;
};

struct _cptr {
	cptr* next;
	cptr* prev;

	unsigned int refCount;
	cptr_list children;

	void* data;
};

static void addptr(cptr_list* l, cptr* t)
{
	if (l->tail) {
		l->tail->next = t;
		t->prev = l->tail;
	} else {
		l->head = t;
		t->prev = NULL;
	}

	l->tail = t;
	t->next = NULL;
}

void* cptr_alloc(size_t size)
{
	cptr* p;
	p = malloc(sizeof(cptr) + size);
	p->data = ((char*)p) + sizeof(cptr);
	p->refCount = 1;
	p->children.head = NULL;
	p->children.tail = NULL;
	return p->data;
}

void* cptr_retain(void* ptr, void* parent)
{
	cptr* p;
	p = (cptr*)(((char*)ptr) - sizeof(cptr));
	p->refCount++;

	if (parent) {
		cptr* pp = (cptr*)(((char*)parent) - sizeof(cptr));
		addptr(&pp->children, p);
	}

	return ptr;
}

void cptr_release(void* ptr)
{
	cptr* p;
	p = (cptr*)(((char*)ptr) - sizeof(cptr));
	p->refCount--;

	if (p->refCount == 0) {
		while (p->children.head) {
			cptr* next = p->children.head->next;
			cptr_release(((char*)p->children.head) + sizeof(cptr));
			p->children.head = next;
		}

		free(p);
	}
}
