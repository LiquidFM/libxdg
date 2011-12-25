/* avltree.c: Private file. Implementation of AVL trees.
 *
 * Copyright (C) 2011  Dmitriy Vilkov <dav.daemon@gmail.com>
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
#include <stdlib.h>


#ifndef BOOL
#	define BOOL unsigned char
#	define TRUE 1
#	define FALSE 0
#endif


/**
 * AvlNode
 *
 */

typedef struct AvlNode AvlNode;

enum Balanced
{
	BALANCED = 0,
	LEFT_IS_HEAVY = -1,
	RIGHT_IS_HEAVY = 1
};
typedef enum Balanced Balanced;

struct Links
{
	AvlNode *left;
	AvlNode *right;
	AvlNode *parent;
};
typedef struct Links Links;

struct AvlNode
{
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

static void destroy_parent_to_child_link(AvlNode *child)
{
	if (child)
	{
		AvlNode *this_parent = child->links.parent;
		child->links.parent = 0;

		if (this_parent)
			if (this_parent->links.left == child)
				this_parent->links.left = 0;
			else
				if (this_parent->links.right == child)
					this_parent->links.right = 0;
	}
}

static void change_left(AvlNode *self, AvlNode *child, AvlNode **out_child_old_parent, AvlNode **out_parent_left_node)
{
	(*out_child_old_parent) = 0;
	(*out_parent_left_node) = 0;

	if (self->links.left)
	{
		(*out_parent_left_node) = self->links.left;
		self->links.left->links.parent = 0;
	}

	if (child)
	{
		(*out_child_old_parent) = child->links.parent;
		destroy_parent_to_child_link(child);
	}

	self->links.left = child;
	if (child)
		child->links.parent = self;
}

static void change_right(AvlNode *self, AvlNode *child, AvlNode **out_child_old_parent, AvlNode **out_parent_right_node)
{
	(*out_child_old_parent) = 0;
	(*out_parent_right_node) = 0;

	if (self->links.right)
	{
		(*out_parent_right_node) = self->links.right;
		self->links.right->links.parent = 0;
	}

	if (child)
	{
		(*out_child_old_parent) = child->links.parent;
		destroy_parent_to_child_link(child);
	}

	self->links.right = child;
	if (child)
		child->links.parent = self;
}

static void replace_node(AvlNode *node_to_replace, AvlNode *new_node, AvlNode **root, Links *new_node_old_links)
{
	AvlNode *old_node_to_replace_parent = node_to_replace->links.parent;
	AvlNode *old_node_to_replace_parent_left = 0;
	AvlNode *old_node_to_replace_parent_right = 0;

	if (old_node_to_replace_parent)
	{
		old_node_to_replace_parent_left = old_node_to_replace_parent->links.left;
		old_node_to_replace_parent_right = old_node_to_replace_parent->links.right;
	}

	AvlNode *node_to_replace_tmp = 0;

	if (new_node)
	{
		change_left(new_node, node_to_replace->links.left, &node_to_replace_tmp, &new_node_old_links->left);
		change_right(new_node, node_to_replace->links.right, &node_to_replace_tmp, &new_node_old_links->right);
	}
	else
	{
		new_node_old_links->left = 0;
		new_node_old_links->right = 0;
		new_node_old_links->parent = 0;
	}

	if (old_node_to_replace_parent)
		if (node_to_replace == old_node_to_replace_parent_left)
			change_left(old_node_to_replace_parent, new_node, &new_node_old_links->parent, &old_node_to_replace_parent_left);
		else
			change_right(old_node_to_replace_parent, new_node, &new_node_old_links->parent, &old_node_to_replace_parent_right);
	else
	{
		(*root) = new_node;

		if (new_node)
			destroy_parent_to_child_link(new_node);
	}
}

static BOOL rebalance_shrunk_routine(Balanced shrunk, AvlNode **node_to_balance, AvlNode **root)
{
	BOOL need_to_stop_balance = FALSE;
	AvlNode *garbage = 0;

	if ((*node_to_balance)->balance == shrunk)
		(*node_to_balance)->balance = BALANCED;
	else
		if ((*node_to_balance)->balance == BALANCED)
		{
			(*node_to_balance)->balance = (Balanced)((int)shrunk*(-1));
			need_to_stop_balance = TRUE;
		}
		else
		{
			AvlNode *child = 0;

			if (shrunk == LEFT_IS_HEAVY)
				child = (*node_to_balance)->links.right;
			else
				child = (*node_to_balance)->links.left;

			Balanced child_balance = child->balance;

			if (child->balance != shrunk)
			{
				Links old_child_links;
				Links old_node_to_balance_links = (*node_to_balance)->links;

				replace_node((*node_to_balance), child, root, &old_child_links);

				if (shrunk == LEFT_IS_HEAVY)
				{
					change_right((*node_to_balance), old_child_links.left, &garbage, &garbage);
					change_left((*node_to_balance), old_node_to_balance_links.left, &garbage, &garbage);

					change_left(child, (*node_to_balance), &garbage, &garbage);
					change_right(child, old_child_links.right, &garbage, &garbage);
				}
				else
				{
					change_left((*node_to_balance), old_child_links.right, &garbage, &garbage);
					change_right((*node_to_balance), old_node_to_balance_links.right, &garbage, &garbage);

					change_right(child, (*node_to_balance), &garbage, &garbage);
					change_left(child, old_child_links.left, &garbage, &garbage);
				}

				if (child_balance == BALANCED)
				{
					(*node_to_balance)->balance = (Balanced)((int)shrunk*(-1));
					child->balance = shrunk;
					need_to_stop_balance = TRUE;
				}
				else
				{
					(*node_to_balance)->balance = BALANCED;
					child->balance = BALANCED;
				}

				(*node_to_balance) = child;
			}
			else
			{
				AvlNode *grandchild = 0;

				if (shrunk == LEFT_IS_HEAVY)
					grandchild = child->links.left;
				else
					grandchild = child->links.right;

				Links old_grandchild_links;
				Balanced grandchild_balance = grandchild->balance;
				Links old_node_to_balance_links = (*node_to_balance)->links;

				replace_node((*node_to_balance), grandchild, root, &old_grandchild_links);

				if (shrunk == LEFT_IS_HEAVY)
				{
					change_left(child, old_grandchild_links.right, &garbage, &garbage);

					change_right((*node_to_balance), old_grandchild_links.left, &garbage, &garbage);
					change_left((*node_to_balance), old_node_to_balance_links.left, &garbage, &garbage);

					change_left(grandchild, (*node_to_balance), &garbage, &garbage);
				}
				else
				{
					change_right(child, old_grandchild_links.left, &garbage, &garbage);

					change_left((*node_to_balance), old_grandchild_links.right, &garbage, &garbage);
					change_right((*node_to_balance), old_node_to_balance_links.right, &garbage, &garbage);

					change_right(grandchild, (*node_to_balance), &garbage, &garbage);
				}

				if (grandchild_balance == (Balanced)((int)shrunk*(-1)))
					(*node_to_balance)->balance = shrunk;
				else
					(*node_to_balance)->balance = BALANCED;

				if (grandchild_balance == shrunk)
					child->balance = (Balanced)((int)shrunk*(-1));
				else
					child->balance = BALANCED;

				grandchild->balance = BALANCED;
				(*node_to_balance) = grandchild;
			}
		}

	return need_to_stop_balance;
}

static BOOL rebalance_left_shrunk(AvlNode **node_to_balance, AvlNode **root)
{
	return rebalance_shrunk_routine(LEFT_IS_HEAVY, node_to_balance, root);
}

static BOOL rebalance_right_shrunk(AvlNode **node_to_balance, AvlNode **root)
{
	return rebalance_shrunk_routine(RIGHT_IS_HEAVY, node_to_balance, root);
}

static void left_left_rotation(AvlNode *parent, AvlNode *child, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;

	if (child->links.parent = parent->links.parent)
		child->links.parent->links.right = child;
	else
		(*root) = child;

	parent->links.right = child->links.left;
	child->links.left = parent;

	parent->links.parent = child;
}

static void right_left_rotation(AvlNode *parent, AvlNode *child, AvlNode *grandchild, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;
	grandchild->balance = BALANCED;

	if (grandchild->links.parent = parent->links.parent)
		grandchild->links.parent->links.right = grandchild;
	else
		(*root) = grandchild;

	parent->links.right = grandchild->links.left;
	child->links.left = grandchild->links.right;

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
		child->links.parent->links.left = child;
	else
		(*root) = child;

	parent->links.left = child->links.right;
	child->links.right = parent;

	parent->links.parent = child;
}

static void left_right_rotation(AvlNode *parent, AvlNode *child, AvlNode *grandchild, AvlNode **root)
{
	parent->balance = BALANCED;
	child->balance = BALANCED;
	grandchild->balance = BALANCED;

	if (grandchild->links.parent = parent->links.parent)
		grandchild->links.parent->links.left = grandchild;
	else
		(*root) = grandchild;

	parent->links.left = grandchild->links.right;
	child->links.right = grandchild->links.left;

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
			this_node->links.left = 0;

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
			this_node->links.right = 0;

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

//static void rebalance_shrunk(AvlNode *this_node, const KEY_TYPE previous_node_value, AvlNode **root, CompareKeys compareKeys)
//{
//	while (this_node != 0)
//	{
//		if (compareKeys(previous_node_value, this_node->key) < 0)
//		{
//			if (rebalance_left_shrunk(&this_node, root))
//				return;
//		}
//		else
//			if (compareKeys(this_node->key, previous_node_value) < 0)
//				if (rebalance_right_shrunk(&this_node, root))
//					return;
//
//		previous_node_value = this_node->key;
//		this_node = this_node->links.parent;
//	}
//}


/**
 * AvlTree
 *
 */

struct AvlTree
{
	AvlNode *tree_root;
	DuplicateKey duplicateKey;
	DestroyKey destroyKey;
	CompareKeys compareKeys;
};


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

static AvlNode *search_or_create(AvlTree *tree, const KEY_TYPE value_to_search, AvlNode **root)
{
	AvlNode *parent_node = 0;
	AvlNode **this_node = search_routine(value_to_search, root, &parent_node, tree->compareKeys);

	if ((*this_node))
		return (*this_node);
	else
	{
		if (((*this_node) = create_avl_node(value_to_search, parent_node, tree->duplicateKey)))
			rebalance_grew((*this_node), root);

		return (*this_node);
	}
}

static AvlNode *search(const KEY_TYPE value_to_search, AvlNode **root, CompareKeys compareKeys)
{
	AvlNode *parent_node = 0;
	return *search_routine(value_to_search, root, &parent_node, compareKeys);
}

AvlTree *create_avl_tree(DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys)
{
	AvlTree *res = malloc(sizeof(AvlTree));

	res->tree_root = NULL;
	res->duplicateKey = duplicateKey;
	res->destroyKey = destroyKey;
	res->compareKeys = compareKeys;

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
	return &search_or_create(tree, key, &tree->tree_root)->value;
}

VALUE_TYPE *search_node(AvlTree *tree, const KEY_TYPE key)
{
	AvlNode *res = search(key, &tree->tree_root, tree->compareKeys);

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
