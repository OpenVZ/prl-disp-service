/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

/* To compile: gcc rbtree-test.c */

#include <stdlib.h>
#include <stdio.h>

/* for compilation w/o parallels headers */
#define ULONG_PTR unsigned long
#define offsetof(TYPE, MEMBER) ((ULONG_PTR) &((TYPE*)0)->MEMBER)
#define container_of(ptr, type, member) \
		((type *)(((char *)(ptr)) - offsetof(type,member)))

#include "rbtree.h"
#include "rbtree.c"

struct obj {
	int key;
	struct rb_node rb;
};

struct rb_root root;

#define N 20
struct obj objs[N] = {{1}, {5}, {3}, {4}, {2}, {15}, {30}, {20}, {35},
	{45}, {7}, {100}, {50}, {55}, {56}, {-10}, {-20}, {-30}, {-5}, {-2}};

static int height(struct rb_node *n)
{
	int hr, hl;

	if (!n)
		return 0;

	hr = height(n->rb_right);
	hl = height(n->rb_left);

	return 1 + (hr > hl ? hr : hl);
}

static inline int cmp(struct rb_node *node, ULONG_PTR key)
{
	struct obj *o = rb_entry(node, struct obj, rb);

	if (o->key == key)
		return 0;
	else
		return o->key < (int)key ? 1 : -1;
}

void stress()
{
	struct obj *o;
	int i, n = 1000;

	/* insert */
	for (i = 0; i < n/2; i++) {
		o = (struct obj*)malloc(sizeof(*o));
retry:
		o->key = random() % n;
		if (rb_search_node(&root, cmp, o->key))
			goto retry;

		rb_insert_node(&root, &o->rb, cmp, o->key);
	}

	/* delete */
	for (i = 0; i < n/3; i++) {
		struct rb_node *node;
		ULONG_PTR key;
retry2:
		key = random() % n;
		node =  rb_search_node(&root, cmp, key);
		if (!node)
			goto retry2;

		printf("delete %d\n", key);
		rb_delete(&root, node);
	}

	/* insert */
	for (i = 0; i < n/4; i++) {
		o = (struct obj*)malloc(sizeof(*o));
retry3:
		o->key = random() % n;
		if (rb_search_node(&root, cmp, o->key))
			goto retry3;

		rb_insert_node(&root, &o->rb, cmp, o->key);
	}
}

int main()
{
	int i;
	struct obj *o;

	/* insert object into rbtree */
	for (i = 0; i < N; i++)
		rb_insert_node(&root, &objs[i].rb, cmp, objs[i].key);

	printf("key == 3, found obj %p, should be %p\n",
			rb_search_node(&root, cmp, 3),
			&objs[2]);

	rb_for_each(struct obj, o, &root, rb)
		printf(" key %d, obj %p\n", o->key, o);
	printf("tree height = %d\n", height(root.rb_node));

	printf("delete key 2, 3, 15, 7, 100, -10...\n");
	rb_delete(&root, rb_search_node(&root, cmp, 3));
	rb_delete(&root, rb_search_node(&root, cmp, 2));
	rb_delete(&root, rb_search_node(&root, cmp, 15));
	rb_delete(&root, rb_search_node(&root, cmp, 7));
	rb_delete(&root, rb_search_node(&root, cmp, 100));
	rb_delete(&root, rb_search_node(&root, cmp, -20));

	rb_for_each(struct obj, o, &root, rb)
		printf(" key %d, obj %p\n", o->key, o);
	printf("tree height = %d\n", height(root.rb_node));

	stress();
	rb_for_each(struct obj, o, &root, rb)
		printf(" key %d, obj %p\n", o->key, o);
	printf("tree height = %d\n", height(root.rb_node));

	return 0;
}

