/* avltree.c: Private file. Implementation of AVL trees.
 *
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
 *
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "avltree.h"
#include <string.h>
#include <fcntl.h>

#ifdef BOOL
#	undef BOOL
#endif

#ifdef TRUE
#	undef TRUE
#endif

#ifdef FALSE
#	undef FALSE
#endif

#define BOOL int
#define TRUE 1
#define FALSE 0


/**
 * AvlNode
 *
 */

enum Balanced
{
	BALANCED = 0,
	LEFT_IS_HEAVY = -1,
	RIGHT_IS_HEAVY = 1
};
typedef enum Balanced Balanced;

enum Visit
{
	NONE_IS_VISITED = 0,
	LEFT_IS_VISITED = 1,
	RIGHT_IS_VISITED = 2,
	ALL_IS_VISITED = 3
};
typedef enum Visit Visit;

struct Links
{
	AvlNode *left;
	AvlNode *right;
	AvlNode *parent;
};
typedef struct Links Links;

struct AvlNode
{
	Visit visit;
	Links links;
	Balanced balance;
	KEY_TYPE key;
	VALUE_TYPE value;
};

static AvlNode *create_avl_node(const KEY_TYPE key, AvlNode *parent, DuplicateKey duplicateKey)
{
	AvlNode *res = malloc(sizeof(AvlNode));

	res->key = duplicateKey(key);
	res->value = 0;
	res->balance = BALANCED;
	res->links.left = 0;
	res->links.right = 0;
	res->links.parent = parent;
	res->visit = NONE_IS_VISITED;

	return res;
}

static void free_avl_node(AvlNode *node, DestroyKey destroyKey)
{
	destroyKey(node->key);
	free(node);
}

static void free_avl_node_and_value(AvlNode *node, DestroyKey destroyKey, DestroyValue destroyValue)
{
	destroyKey(node->key);
	destroyValue(node->value);
	free(node);
}

static void left_left_rotation(AvlNode *parent, AvlNode *child, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;

	if (child->links.parent = parent->links.parent)
		if (child->links.parent->links.left == parent)
			child->links.parent->links.left = child;
		else
			child->links.parent->links.right = child;
	else
		(*root) = child;

	if (parent->links.right = child->links.left)
		parent->links.right->links.parent = parent;

	child->links.left = parent;
	parent->links.parent = child;
}

static void right_left_rotation(AvlNode *parent, AvlNode *child, AvlNode *grandchild, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;
	grandchild->balance = BALANCED;

	if (grandchild->links.parent = parent->links.parent)
		if (grandchild->links.parent->links.left == parent)
			grandchild->links.parent->links.left = grandchild;
		else
			grandchild->links.parent->links.right = grandchild;
	else
		(*root) = grandchild;

	if (parent->links.right = grandchild->links.left)
		parent->links.right->links.parent = parent;

	if (child->links.left = grandchild->links.right)
		child->links.left->links.parent = child;

	grandchild->links.left = parent;
	grandchild->links.right = child;

	parent->links.parent = grandchild;
	child->links.parent = grandchild;
}

static void right_right_rotation(AvlNode *parent, AvlNode *child, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;

	if (child->links.parent = parent->links.parent)
		if (child->links.parent->links.left == parent)
			child->links.parent->links.left = child;
		else
			child->links.parent->links.right = child;
	else
		(*root) = child;

	if (parent->links.left = child->links.right)
		parent->links.left->links.parent = parent;

	child->links.right = parent;
	parent->links.parent = child;
}

static void left_right_rotation(AvlNode *parent, AvlNode *child, AvlNode *grandchild, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;
	grandchild->balance = BALANCED;

	if (grandchild->links.parent = parent->links.parent)
		if (grandchild->links.parent->links.left == parent)
			grandchild->links.parent->links.left = grandchild;
		else
			grandchild->links.parent->links.right = grandchild;
	else
		(*root) = grandchild;

	if (parent->links.left = grandchild->links.right)
		parent->links.left->links.parent = parent;

	if (child->links.right = grandchild->links.left)
		child->links.right->links.parent = child;

	grandchild->links.right = parent;
	grandchild->links.left = child;

	parent->links.parent = grandchild;
	child->links.parent = grandchild;
}

static void rebalance_grew(AvlNode *this_node, AvlNode **root)
{
	AvlNode *previous_node = this_node;
	this_node = this_node->links.parent;

	while (this_node)
	{
		if (this_node->links.left == previous_node)
			if (this_node->balance == RIGHT_IS_HEAVY)
			{
				this_node->balance = BALANCED;
				return;
			}
			else
				if (this_node->balance == BALANCED)
					this_node->balance = LEFT_IS_HEAVY;
				else
				{
					if (previous_node->balance == LEFT_IS_HEAVY)
						right_right_rotation(this_node, previous_node, root);
					else
						left_right_rotation(this_node, previous_node, previous_node->links.right, root);

					return;
				}
		else
			if (this_node->balance == LEFT_IS_HEAVY)
			{
				this_node->balance = BALANCED;
				return;
			}
			else
				if (this_node->balance == BALANCED)
					this_node->balance = RIGHT_IS_HEAVY;
				else
				{
					if (previous_node->balance == RIGHT_IS_HEAVY)
						left_left_rotation(this_node, previous_node, root);
					else
						right_left_rotation(this_node, previous_node, previous_node->links.left, root);

					return;
				}

		previous_node = this_node;
		this_node = this_node->links.parent;
	}
}

static void rebalance_shrunk(AvlNode *this_node, AvlNode **root)
{
	Links previous_node_links = {};
	AvlNode *previous_node = 0;
	AvlNode *node_to_remove = this_node;
	this_node = this_node->links.left;

	while (this_node)
	{
		previous_node = this_node;
		this_node = this_node->links.right;
	}

	if (previous_node == 0)
		previous_node = node_to_remove->links.right;

	if (previous_node)
	{
		if (previous_node->links.parent == node_to_remove)
			this_node = previous_node;
		else
			this_node = previous_node->links.parent;

		previous_node_links = previous_node->links;
		previous_node->balance = node_to_remove->balance;
		previous_node->links = node_to_remove->links;

		if (previous_node->links.parent == 0)
			(*root) = previous_node;
		else
			if (previous_node->links.parent->links.left == node_to_remove)
				previous_node->links.parent->links.left = previous_node;
			else
				previous_node->links.parent->links.right = previous_node;
	}
	else
	{
		this_node = node_to_remove->links.parent;
		previous_node = node_to_remove;
	}

	if (this_node)
	{
		if (this_node->links.left == previous_node)
		{
			this_node->links.left = previous_node_links.left;

			if (this_node->balance == LEFT_IS_HEAVY)
				this_node->balance = BALANCED;
			else
				if (this_node->balance == BALANCED)
					this_node->balance = RIGHT_IS_HEAVY;
				else
					if (this_node->links.right->balance == RIGHT_IS_HEAVY)
					{
						node_to_remove = this_node->links.right;
						left_left_rotation(this_node, this_node->links.right, root);
						this_node = node_to_remove;
					}
					else
					{
						node_to_remove = this_node->links.right->links.left;
						right_left_rotation(this_node, this_node->links.right, this_node->links.right->links.left, root);
						this_node = node_to_remove;
					}
		}
		else
		{
			this_node->links.right = previous_node_links.right;

			if (this_node->balance == RIGHT_IS_HEAVY)
				this_node->balance = BALANCED;
			else
				if (this_node->balance == BALANCED)
					this_node->balance = LEFT_IS_HEAVY;
				else
					if (this_node->links.left->balance == LEFT_IS_HEAVY)
					{
						node_to_remove = this_node->links.left;
						right_right_rotation(this_node, this_node->links.left, root);
						this_node = node_to_remove;
					}
					else
					{
						node_to_remove = this_node->links.left->links.right;
						left_right_rotation(this_node, this_node->links.left, this_node->links.left->links.right, root);
						this_node = node_to_remove;
					}
		}

		previous_node = this_node;
		this_node = this_node->links.parent;

		while (this_node)
		{
			if (this_node->links.left == previous_node)
				if (this_node->balance == LEFT_IS_HEAVY)
					this_node->balance = BALANCED;
				else
					if (this_node->balance == BALANCED)
						this_node->balance = RIGHT_IS_HEAVY;
					else
						if (this_node->links.right->balance == RIGHT_IS_HEAVY)
						{
							node_to_remove = this_node->links.right;
							left_left_rotation(this_node, this_node->links.right, root);
							this_node = node_to_remove;
						}
						else
						{
							node_to_remove = this_node->links.right->links.left;
							right_left_rotation(this_node, this_node->links.right, this_node->links.right->links.left, root);
							this_node = node_to_remove;
						}
			else
				if (this_node->balance == RIGHT_IS_HEAVY)
					this_node->balance = BALANCED;
				else
					if (this_node->balance == BALANCED)
						this_node->balance = LEFT_IS_HEAVY;
					else
						if (this_node->links.left->balance == LEFT_IS_HEAVY)
						{
							node_to_remove = this_node->links.left;
							right_right_rotation(this_node, this_node->links.left, root);
							this_node = node_to_remove;
						}
						else
						{
							node_to_remove = this_node->links.left->links.right;
							left_right_rotation(this_node, this_node->links.left, this_node->links.left->links.right, root);
							this_node = node_to_remove;
						}

			previous_node = this_node;
			this_node = this_node->links.parent;
		}
	}
	else
		(*root) = 0;
}


/**
 * AvlTree
 *
 */

static void destroy_subtree(AvlTree *tree, AvlNode **subtree_root)
{
	if ((*subtree_root) == 0)
		return;

	AvlNode *this_node = (*subtree_root);

	while (TRUE)
		if (this_node->links.right)
			this_node = this_node->links.right;
		else
			if (this_node->links.left)
				this_node = this_node->links.left;
			else
			{
				AvlNode *node_to_delete = this_node;
				this_node = this_node->links.parent;

				free_avl_node(node_to_delete, tree->destroyKey);

				if (node_to_delete == (*subtree_root) || this_node == 0)
					break;
				else
					if (node_to_delete == this_node->links.right)
						this_node->links.right = 0;
					else
						this_node->links.left = 0;
			}

	(*subtree_root) = 0;
}

static void destroy_subtree_and_values(AvlTree *tree, AvlNode **subtree_root, DestroyValue destroyValue)
{
	if ((*subtree_root) == 0)
		return;

	AvlNode *this_node = (*subtree_root);

	while (TRUE)
		if (this_node->links.right)
			this_node = this_node->links.right;
		else
			if (this_node->links.left)
				this_node = this_node->links.left;
			else
			{
				AvlNode *node_to_delete = this_node;
				this_node = this_node->links.parent;

				free_avl_node_and_value(node_to_delete, tree->destroyKey, destroyValue);

				if (node_to_delete == (*subtree_root) || this_node == 0)
					break;
				else
					if (node_to_delete == this_node->links.right)
						this_node->links.right = 0;
					else
						this_node->links.left = 0;
			}

	(*subtree_root) = 0;
}

static AvlNode **search_routine(const KEY_TYPE value_to_search, AvlNode **root, AvlNode **out_parent_node, CompareKeys compareKeys)
{
	int res;
	AvlNode *this_parent_node = 0;
	AvlNode **this_node_pointer = root;

	while ((*this_node_pointer) && (res = compareKeys((*this_node_pointer)->key, value_to_search)) != 0)
	{
		this_parent_node = (*this_node_pointer);

		if (res < 0)
			this_node_pointer = &(*this_node_pointer)->links.right;
		else
			this_node_pointer = &(*this_node_pointer)->links.left;
	}

	(*out_parent_node) = this_parent_node;
	return this_node_pointer;
}

AvlTree *create_avl_tree(DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys)
{
	AvlTree *res = malloc(sizeof(AvlTree));

	init_avl_tree(res, duplicateKey, destroyKey, compareKeys);

	return res;
}

void free_avl_tree(AvlTree *tree)
{
	destroy_subtree(tree, &tree->tree_root);
	free(tree);
}

void free_avl_tree_and_values(AvlTree *tree, DestroyValue destroyValue)
{
	destroy_subtree_and_values(tree, &tree->tree_root, destroyValue);
	free(tree);
}

void init_avl_tree(AvlTree *tree, DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys)
{
	tree->tree_root = NULL;
	tree->duplicateKey = duplicateKey;
	tree->destroyKey = destroyKey;
	tree->compareKeys = compareKeys;
}

void clear_avl_tree(AvlTree *tree)
{
	destroy_subtree(tree, &tree->tree_root);
}

void clear_avl_tree_and_values(AvlTree *tree, DestroyValue destroyValue)
{
	destroy_subtree_and_values(tree, &tree->tree_root, destroyValue);
}

VALUE_TYPE *search_or_create_node(AvlTree *tree, const KEY_TYPE key)
{
	AvlNode *parent_node = 0;
	AvlNode **this_node = search_routine(key, &tree->tree_root, &parent_node, tree->compareKeys);

	if ((*this_node))
		return &(*this_node)->value;
	else
	{
		if (((*this_node) = parent_node = create_avl_node(key, parent_node, tree->duplicateKey)))
			rebalance_grew(parent_node, &tree->tree_root);

		return &parent_node->value;
	}
}

VALUE_TYPE *search_node(const AvlTree *tree, const KEY_TYPE key)
{
	AvlNode *parent_node = 0;
	AvlNode *res = *search_routine(key, &((AvlTree *)tree)->tree_root, &parent_node, tree->compareKeys);

	if (res)
		return &res->value;
	else
		return 0;
}

VALUE_TYPE delete_node(AvlTree *tree, const KEY_TYPE key)
{
	AvlNode *parent_node = 0;
	AvlNode *this_node = *search_routine(key, &tree->tree_root, &parent_node, tree->compareKeys);

	if (this_node)
	{
		VALUE_TYPE res = this_node->value;

		rebalance_shrunk(this_node, &tree->tree_root);
		free_avl_node(this_node, tree->destroyKey);

		return res;
	}
	else
		return 0;
}

static void write_empty_node_to_file(int fd, AvlNode *stack_node)
{
	memset(stack_node, 0, sizeof(AvlNode));
	write(fd, stack_node, sizeof(AvlNode));
}

static void write_node_to_file(int fd, AvlNode *stack_node, const AvlNode *source_node, WriteKey writeKey, WriteValue writeValue)
{
	memcpy(stack_node, source_node, sizeof(AvlNode));
	stack_node->visit = ALL_IS_VISITED;

	write(fd, stack_node, sizeof(AvlNode));

	writeKey(fd, stack_node->key);
	writeValue(fd, stack_node->value);
}

static void write_subtree_to_file(int fd, AvlNode *subtree_root, WriteKey writeKey, WriteValue writeValue)
{
	AvlNode stack_node;
	AvlNode *this_node = subtree_root;
	this_node->visit = NONE_IS_VISITED;

	write_node_to_file(fd, &stack_node, this_node, writeKey, writeValue);

	/* Depth-first search (DFS) algorithm */
	while (TRUE)
		if (this_node->visit & LEFT_IS_VISITED)
			if (this_node->visit & RIGHT_IS_VISITED)
			{
				this_node = this_node->links.parent;

				if (this_node == 0)
					break;
			}
			else
			{
				this_node->visit |= RIGHT_IS_VISITED;

				if (this_node->links.right)
				{
					write_node_to_file(fd, &stack_node, this_node->links.right, writeKey, writeValue);
					this_node = this_node->links.right;
					this_node->visit = NONE_IS_VISITED;
				}
				else
					write_empty_node_to_file(fd, &stack_node);
			}
		else
		{
			this_node->visit |= LEFT_IS_VISITED;

			if (this_node->links.left)
			{
				write_node_to_file(fd, &stack_node, this_node->links.left, writeKey, writeValue);
				this_node = this_node->links.left;
				this_node->visit = NONE_IS_VISITED;
			}
			else
				write_empty_node_to_file(fd, &stack_node);
		}
}

static void map_subtree_from_memory(void **memory, AvlTree *tree, ReadKey readKey, ReadValue readValue, void *user_data)
{
	AvlNode *stack_node = (*memory);

	if (stack_node->visit)
	{
		AvlNode *this_node = tree->tree_root = stack_node;

		(*memory) += sizeof(AvlNode);
		this_node->key = readKey(memory, user_data);
		this_node->value = readValue(memory, user_data);

		this_node->visit = NONE_IS_VISITED;
		stack_node = (*memory);

		/* Depth-first search (DFS) algorithm */
		while (TRUE)
			if (this_node->visit & LEFT_IS_VISITED)
				if (this_node->visit & RIGHT_IS_VISITED)
				{
					this_node = this_node->links.parent;

					if (this_node == 0)
						break;
				}
				else
					if (stack_node->visit)
					{
						this_node->links.right = stack_node;
						stack_node->links.parent = this_node;
						this_node->visit |= RIGHT_IS_VISITED;
						this_node = stack_node;

						(*memory) += sizeof(AvlNode);
						this_node->key = readKey(memory, user_data);
						this_node->value = readValue(memory, user_data);

						this_node->visit = NONE_IS_VISITED;
						stack_node = (*memory);
					}
					else
					{
						this_node->visit |= RIGHT_IS_VISITED;
						stack_node = ((*memory) += sizeof(AvlNode));
					}
			else
				if (stack_node->visit)
				{
					this_node->links.left = stack_node;
					stack_node->links.parent = this_node;
					this_node->visit |= LEFT_IS_VISITED;
					this_node = stack_node;

					(*memory) += sizeof(AvlNode);
					this_node->key = readKey(memory, user_data);
					this_node->value = readValue(memory, user_data);

					this_node->visit = NONE_IS_VISITED;
					stack_node = (*memory);
				}
				else
				{
					this_node->visit |= LEFT_IS_VISITED;
					stack_node = ((*memory) += sizeof(AvlNode));
				}
	}
	else
		(*memory) += sizeof(AvlNode);
}

const AvlTree *map_from_memory(void **memory, ReadKey readKey, ReadValue readValue, CompareKeys compareKeys, void *user_data)
{
	AvlTree *res = (*memory);
	(*memory) += sizeof(AvlTree);

	init_avl_tree(res, NULL, NULL, compareKeys);
	map_subtree_from_memory(memory, res, readKey, readValue, user_data);

	return res;
}

void write_to_file(int fd, const AvlTree *tree, WriteKey writeKey, WriteValue writeValue)
{
	AvlTree saved_tree;
	memset(&saved_tree, (unsigned int)-1, sizeof(AvlTree));
	write(fd, &saved_tree, sizeof(AvlTree));

	if (tree->tree_root)
		write_subtree_to_file(fd, (AvlNode *)tree->tree_root, writeKey, writeValue);
	else
	{
		AvlNode stack_node;
		write_empty_node_to_file(fd, &stack_node);
	}
}
