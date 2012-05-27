/** @file avltree.h
 *  @brief Public file.
 *
 * Declaration of AVL trees.
 *
 * @copyright
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
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

#ifndef AVLTREE_H_
#define AVLTREE_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Data type which is used for keys of AVL trees.
 */
#define KEY_TYPE   char *

/**
 * Data type which is used for values of AVL trees.
 */
#define VALUE_TYPE void *


/**
 * Function type.
 * @n Functions of this type is used for duplicating (\c strdup) keys of AVL trees.
 */
typedef KEY_TYPE (*DuplicateKey)(const KEY_TYPE key);

/**
 * Function type.
 * @n Functions of this type is used for destroying (\c free) keys of AVL trees.
 */
typedef void (*DestroyKey)(KEY_TYPE key);

/**
 * Function type.
 * @n Functions of this type is used for comparison (\c strcmp) keys of AVL trees.
 */
typedef int (*CompareKeys)(const KEY_TYPE key1, const KEY_TYPE key2);

/**
 * Function type.
 * @n Functions of this type is used for destroying (\c free) values of AVL trees.
 */
typedef void (*DestroyValue)(VALUE_TYPE value);

/**
 * Function type.
 * @n Functions of this type is used for traversing through content of AVL trees.
 */
typedef void (*DepthFirstSearch)(const KEY_TYPE key, const VALUE_TYPE value, void *user_data);


/**
 * Function type.
 * @n Functions of this type is used for reading (mapping) keys of AVL trees from memory.
 */
typedef KEY_TYPE (*ReadKey)(void **memory, void *user_data);

/**
 * Function type.
 * @n Functions of this type is used for reading (mapping) values of AVL trees from memory.
 */
typedef VALUE_TYPE (*ReadValue)(void **memory, void *user_data);

/**
 * Function type.
 * @n Functions of this type is used for writing keys of AVL trees to file.
 */
typedef void (*WriteKey)(int fd, const KEY_TYPE key);

/**
 * Function type.
 * @n Functions of this type is used for writing values of AVL trees to file.
 */
typedef void (*WriteValue)(int fd, const VALUE_TYPE value, void *user_data);


/**
 * Data structure which represents a node of AVL tree.
 */
typedef struct AvlNode AvlNode;

/**
 * Data structure which represents an AVL tree.
 */
typedef struct AvlTree AvlTree;


/**
 * Creates an AVL tree.
 *
 * @param duplicateKey pointer to function which will be used for duplication of keys (\c strdup).
 * @param destroyKey pointer to function which will be used for destroying of keys (\c free).
 * @param compareKeys pointer to function which will be used for comparison of keys (\c strcmp).
 *
 * @return pointer to newly created tree.
 */
AvlTree *create_avl_tree(DuplicateKey duplicateKey, DestroyKey destroyKey, CompareKeys compareKeys);

/**
 * Destroys an AVL tree.
 */
void free_avl_tree(AvlTree *tree);

/**
 * Destroys an AVL tree and it's values.
 *
 * @param tree pointer to an AVL tree.
 * @param destroyValue pointer to function which will be used for destruction of values.
 *
 * @note This method is not thread safe.
 */
void free_avl_tree_and_values(AvlTree *tree, DestroyValue destroyValue);

/**
 * Checks if an AVL is empty.
 *
 * @return \c TRUE if tree is empty, \c FALSE otherwise.
 *
 * @note This method is not thread safe.
 */
int is_empty_tree(const AvlTree *tree);

/**
 * Depth-first search (DFS) algorithm to traverse an AVL tree.
 *
 * @param tree pointer to an AVL tree.
 * @param callback pointer to function which will be called for each key/value pair.
 * @param user_data pointer to user data.
 *
 * @note This method is not thread safe.
 */
void depth_first_search(const AvlTree *tree, DepthFirstSearch callback, void *user_data);

/**
 * Searches value for a given key. If key was not found, creates it.
 *
 * @param tree pointer to an AVL tree.
 * @param key pointer to a key value.
 *
 * @return pointer to \c VALUE_TYPE.
 *
 * @note This method is not thread safe.
 */
VALUE_TYPE *search_or_create_node(AvlTree *tree, const KEY_TYPE key);

/**
 * Searches value for a given key.
 *
 * @param tree pointer to an AVL tree.
 * @param key pointer to a key value.
 *
 * @return pointer to \c VALUE_TYPE or \c NULL if node with such key was not found.
 */
VALUE_TYPE *search_node(const AvlTree *tree, const KEY_TYPE key);

/**
 * Searches value for a given key and removes it from tree.
 *
 * @param tree pointer to an AVL tree.
 * @param key pointer to a key value.
 *
 * @return \c VALUE_TYPE of removed node.
 *
 * @note This method is not thread safe.
 */
VALUE_TYPE delete_node(AvlTree *tree, const KEY_TYPE key);


/**
 * Reads (maps) an AVL tree from memory.
 *
 * @param memory pointer to pointer to memory.
 * @param readKey pointer to function which will be used for reading (mapping) a key of an AVL tree from memory.
 * @param readValue pointer to function which will be used for reading (mapping) a value of an AVL tree from memory.
 * @param compareKeys pointer to function which will be used for comparison of keys of an AVL tree.
 * @param user_data pointer to user data.
 *
 * @return \c const pointer to a newly read (mapped) AVL tree.
 */
const AvlTree *map_from_memory(void **memory, ReadKey readKey, ReadValue readValue, CompareKeys compareKeys, void *user_data);

/**
 * Writes an AVL tree to file.
 *
 * @param fd file descriptor.
 * @param tree pointer to an AVL tree.
 * @param writeKey pointer to function which will be used for writing a key of an AVL tree to file.
 * @param writeValue pointer to function which will be used for writing a value of an AVL tree to file.
 * @param user_data pointer to user data.
 *
 * @return \c const pointer to a newly read (mapped) AVL tree.
 */
void write_to_file(int fd, const AvlTree *tree, WriteKey writeKey, WriteValue writeValue, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* AVLTREE_H_ */
