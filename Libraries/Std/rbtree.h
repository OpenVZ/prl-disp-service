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

/*
 * RB tree in C, easy to use implementation.
 * Author: Kirill Korotaev <dev@parallels.com>
 */

/*
 * Main primitives:
 *   rb_search_node - find node with given key
 *   rb_delete      - remove rb_node from the tree
 *   rb_insert      - insert rb_node to the tree (no duplicates allowed!)
 *   rb_first/rb_last - returns min/max nodes
 *   rb_for_each    - loop over all elements in the tree in sorted order
 * NOTE: Caller must protect tree operations with appropriate locks
 */

/*
 * Linux kernel has its own rbtree implementation, with conflicting API names.
 * If you need to use this header in kernel module compiled on linux
 * simply create a file w/o linux headers for your needs, but with this header
 * and then link with it.
 */
#if defined(LINUX_VERSION_CODE) && defined(__KERNEL__)
#error "Conflict with Linux kernel rbtrees"
#endif

#ifndef	__PRL_RBTREE_H__
#define	__PRL_RBTREE_H__


#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define	RBTREE_RED		0
#define	RBTREE_BLACK	1

struct rb_node
{
	ULONG_PTR rb_parent;		/* the lowest bit is used for color */
	struct rb_node *rb_right;
	struct rb_node *rb_left;
};

struct rb_root
{
	struct rb_node *rb_node;
};

#define	rb_entry(ptr, type, member) container_of(ptr, type, member)

#define rb_empty(root)	((root)->rb_node == NULL)

static inline void rb_init(struct rb_root *root)
{
	root->rb_node = NULL;
}

void rb_insert_fixup(struct rb_root *, struct rb_node *);
void rb_delete(struct rb_root *, struct rb_node *);

struct rb_node *rb_next(struct rb_node *);
struct rb_node *rb_prev(struct rb_node *);
struct rb_node *rb_first(struct rb_root *);
struct rb_node *rb_last(struct rb_root *);

/* comparator function for node element: should return {<0, 0, >0} if
 * node->key { <key, ==key, >key} */
typedef int (*rb_cmp_fn_t)(struct rb_node *node, ULONG_PTR key);

static inline struct rb_node *rb_search_node(struct rb_root *root,
					rb_cmp_fn_t cmp_fn, ULONG_PTR key)
{
	int cmp_res;
	struct rb_node *node = root->rb_node;

	while (node) {
		cmp_res = cmp_fn(node, key);
		if (cmp_res < 0)
			node = node->rb_left;
		else if (cmp_res > 0)
			node = node->rb_right;
		else
			return node;
	}
	return NULL;
}

static inline void rb_tree_insert(struct rb_node *parent,
		struct rb_node **parent_link, struct rb_node *new_node)
{
	/* insert new node below the parent, where parent_link is either
	 * &parent->rb_right or &parent->rb_left */
	new_node->rb_parent = (ULONG_PTR)parent;	/* RED initially */
	new_node->rb_left = new_node->rb_right = NULL;
	*parent_link = new_node;
}

/* if node with given key exists, returns it. Otherwise inserts new node */
static inline struct rb_node *rb_insert_node(struct rb_root *root,
					struct rb_node *new_node,
					rb_cmp_fn_t cmp_fn, ULONG_PTR key)
{
	int cmp_res;
	struct rb_node **node = &root->rb_node;
	struct rb_node *parent = NULL;

	while (*node) {
		parent = *node;

		cmp_res = cmp_fn(*node, key);
		if (cmp_res < 0)
			node = &(*node)->rb_left;
		else if (cmp_res > 0)
			node = &(*node)->rb_right;
		else
			return *node;	/* collision? should we insert at all? */
	}

	/* insert & rebalance rb-tree */
	rb_tree_insert(parent, node, new_node);
	rb_insert_fixup(root, new_node);

	return NULL;
}

#define rb_for_each(typeof_pos, pos, root, member)	\
	for (pos = rb_entry(rb_first(root), typeof_pos, member); \
			&pos->member != NULL; \
			pos = rb_entry(rb_next(&pos->member), typeof_pos, member))

#ifdef __cplusplus
}
#endif

#endif

