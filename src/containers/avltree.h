/* avltree.h: Private file. Declaration of AVL trees.
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

#ifndef AVLTREE_H_
#define AVLTREE_H_


/**
 * Main data types.
 *
 */
#define KEY_TYPE   char *
#define VALUE_TYPE void *


/**
 * Functions types for keys and values management.
 *
 */
typedef KEY_TYPE (*DuplicateKey)(const KEY_TYPE key);
typedef void (*DestroyKey)(KEY_TYPE key);
typedef int (*CompareKeys)(const KEY_TYPE key1, const KEY_TYPE key2);
typedef void (*DestroyValue)(VALUE_TYPE value);

typedef KEY_TYPE (*ReadKey)(void **memory, void *user_data);
typedef VALUE_TYPE (*ReadValue)(void **memory, void *user_data);
typedef void (*WriteKey)(int fd, const KEY_TYPE key);
typedef void (*WriteValue)(int fd, const VALUE_TYPE value);


/**
 * Main data structures.
 *
 */
typedef struct AvlNode AvlNode;

struct AvlTree
{
	AvlNode *tree_root;
	DuplicateKey duplicateKey;
	DestroyKey destroyKey;
	CompareKeys compareKeys;
};
typedef struct AvlTree AvlTree;


/**
 * Functions for tree management.
 *
 */
AvlTree *create_avl_tree(DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys);
void init_avl_tree(AvlTree *tree, DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys);
void free_avl_tree(AvlTree *tree);
void free_avl_tree_and_values(AvlTree *tree, DestroyValue destroyValue);

void clear_avl_tree(AvlTree *tree);
void clear_avl_tree_and_values(AvlTree *tree, DestroyValue destroyValue);

VALUE_TYPE *search_or_create_node(AvlTree *tree, const KEY_TYPE key);
VALUE_TYPE *search_node(const AvlTree *tree, const KEY_TYPE key);
VALUE_TYPE delete_node(AvlTree *tree, const KEY_TYPE key);

const AvlTree *map_from_memory(void **memory, ReadKey readKey, ReadValue readValue, CompareKeys compareKeys, void *user_data);
void write_to_file(int fd, const AvlTree *tree, WriteKey writeKey, WriteValue writeValue);

#endif /* AVLTREE_H_ */
