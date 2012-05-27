/** @internal @file avltree_p.h
 *  @brief Private file.
 *
 * Data structure for storing an AVL tree.
 *
 * @copyright
 * Copyright (C) 2012  Dmitriy Vilkov <dav.daemon@gmail.com>
 * @n@n
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 * @n@n
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * @n@n
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 * @n@n
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef AVLTREE_P_H_
#define AVLTREE_P_H_

#include "avltree.h"


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

struct AvlTree
{
	AvlNode *tree_root;
	DuplicateKey duplicateKey;
	DestroyKey destroyKey;
	CompareKeys compareKeys;
};


void init_avl_tree(AvlTree *tree, DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys);
void clear_avl_tree(AvlTree *tree);
void clear_avl_tree_and_values(AvlTree *tree, DestroyValue destroyValue);

#endif /* AVLTREE_P_H_ */
