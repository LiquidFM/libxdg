/** @mainpage
 *
 * @section intro_sec Introduction
 *
 * This library is implementation of several freedesktop.org specifications such as
 *
 * <ul>
 * <li><a href="http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec">"Shared MIME-info Database"</a>;
 * <li><a href="http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html">"Desktop Entry Specification"</a>;
 * <li><a href="http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html">"Icon Theme Specification"</a>;
 * <li><a href="http://www.freedesktop.org/wiki/Specifications/mime-actions-spec">"MIME actions specification"</a>.
 * </ul>
 *
 * Features:
 * <ul>
 * <li>Straight C-implementation of serializable AVL trees;
 * <li>Access to <a href="http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec">"Shared MIME-info Database"</a> by using of freedesktop.org <a href="http://cgit.freedesktop.org/xdg/xdgmime">xdgmime</a> library;
 * <li>Indexed (by using of AVL trees) access to all ".desktop" files and its contents (implementation of <a href="http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html">"Desktop Entry Specification"</a>);
 * <li>Cache all (include indexes) gathered information from all ".desktop" and ".list" files (<a href="http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html">"Desktop Entry Specification"</a>) into one single binary file which then can be mmap'ed without a single allocation of memory (just updates pointers in mmap'ed memory);
 * <li>Indexed (by using of AVL trees) access to icon themes and its contents (implemetation of <a href="http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html">"Icon Theme Specification"</a> and <a href="http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html">"Icon Naming Specification"</a>).
 * </ul>
 *
 **************************************************************************************
 * @n@section cache_formats_sec Cache formats
 *
 * @subpage avl_tree_cache_format_page
 *
 * @subpage desktop_cache_format_page
 *
 **************************************************************************************
 * @n@section examples_sec Examples
 *
 * This sections describes a number of examples which should provide you with necessary information
 * to start using this library.
 *
 * @subpage example1_page
 *
 **************************************************************************************
 * @n@section license_sec License
 *
 * The library is licensed under GNU-LGPL and AFL v2.0.
 */


/** @page example1_page Example1. List all applications able to handle given mime type.
 *
 * @subpage example1_src
 *
 * @dontinclude examples/example1.c
 *
 * First of all we must include the library header:
 * @line #include
 *
 * And \c "stdio.h" header for \c printf() function:
 * @line #include
 *
 * Function \c print_app() is needed for print information about given \p app:
 * @skip print_app
 * @until }
 * @until }
 * @until }
 * @see xdg_list_begin()
 *      @n xdg_list_next()
 *      @n xdg_app_group_lookup()
 *      @n xdg_app_localized_entry_lookup()
 *      @n xdg_list_item_app_group_entry_value()
 *
 * Function \c print_applications() prints information about all applications able to handle given \p mime type:
 * @skip print_applications
 * @until *apps;
 *
 * \c "removed_apps" variable stores a pointer to XdgJointList of removed (by user) applications which could handle given \p mime type:
 * @skipline removed_apps
 * @see xdg_removed_apps_lookup()
 *
 * Print information about \a all \a user defined applications able to handle given \p mime type:
 * @skip xdg_added_apps_lookup
 * @until ));
 * @see xdg_joint_list_item_app
 *      @n xdg_added_apps_lookup()
 *
 * Print information about \a all \a default applications able to handle given \p mime type:
 * @skip xdg_default_apps_lookup
 * @until ));
 * @see xdg_joint_list_item_app
 *      @n xdg_default_apps_lookup()
 *
 * Print information about \a all \a registered applications able to handle given \p mime type:
 * @skip xdg_known_apps_lookup
 * @until }
 * @until }
 * @see xdg_joint_list_item_app
 *      @n xdg_known_apps_lookup()
 *
 *
 * @n
 * Our \c main function starts like this:
 * @skip main
 * @until {
 *
 * First we are initializing the library:
 * @skipline xdg_init
 * @see xdg_init()
 *
 * Then we print information about \a all applications able to handle \c "text/plain" mime type:
 * @skipline print_applications
 *
 * Shutdown the library and exit from \c main function:
 * @skip xdg_shutdown
 * @until }
 * @see xdg_shutdown()
 */

/** @page example1_src Source code of Example1.
 * List all applications able to handle given mime type:
 * @include examples/example1.c
 */


/** @page avl_tree_cache_format_page Format of AVL tree cache.
 * Here is described binary format in which AVL trees are stored on disk.
 * Comprehensive theoretical information about AVL trees can be found
 * <a href="http://en.wikipedia.org/wiki/AVL_tree">here</a>.
 *
 * <ul>
 * <li> AvlTree structure
 * <li> \c If tree is empty
 *   <ul><li>zeroed AvlNode structure</ul>
 * <li> \c Else
 *   <ul>
 *   <li>root node (AvlNode structure)
 *   <li> ... Depth-first search (DFS) algorithm starting from the left child node ...
 *   @n If node does not have left child then empty node will be written (empty AvlNode structure)
 *   @n If node does not have right child then empty node will be written (empty AvlNode structure)
 *   @n If previous two points were true then take parent node and check his right child
 *   @n If right child exists then write it and repeat algorithm for this child from the beginning,
 *   otherwise take parent node and check his right child
 *   @n Algorithm will stop if there is no parent node
 *   </ul>
 * </ul>
 *
 * @note
 * Each node is written or read by ::WriteKey, ::WriteValue and ::ReadKey, ::ReadValue functions.
 *
 * @see write_to_file()
 *      @n map_from_memory()
 */


/** @page desktop_cache_format_page Binary cache of Desktop Entry Specification.
 * The specification could be found
 * <a href="http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html">here</a>.
 *
 * This specification describes directory layout and format of ".desktop" and ".list" files.
 * Purpose of the library is to provide fastest possible access to contents of this files.
 * This goal is achieved by using of AVL trees. They were chosen because AVL trees are more
 * rigidly balanced than red-black trees, leading to slower insertion and removal but faster retrieval.
 *
 * Format of binary cache which contains data from \a ".desktop" and \a ".list" files is:
 * <ul>
 * <li> 4 \c bytes for version of a cache file.
 *
 * <li> List of XdgFileWatcher structures (at least one zeroed structure, i.e NULL-terminated C list).
 *   @n This list is used during loading of cache to determine validity of cache.
 *   @see xdg_app_cache_file_is_valid()
 *        @n xdg_app_refresh()
 *
 * <li> AVL tree of all \a ".desktop" files located in the same directory (and subdirectories) as cache file.
 *   @n Each key of this tree is a name of \a ".desktop" file (e.g. \a kde4-kate.desktop).
 *   @n Each value of this tree is an AVL tree of XdgApp items, which in turns is an AVL tree of
 *   XdgAppGroup items, which is an AVL tree of XdgAppGroupEntry items.
 *   @note This tree serves as container of \a ".desktop" files for other two major trees in cache.
 *
 * <li> AVL tree of associations of mime type with \a ".desktop" files.
 *   @n Each key of this tree is a name of a first part of mime type (e.g. \a text).
 *   @n Each value of this tree is an AVL tree of a second part of mime type (e.g. \a html), which contains a
 *   list of pointers to XdgApp items.
 *   @note Global version of this tree is used to resolve \a ".desktop" files according to
 *   the given mime type.
 *   @see xdg_known_apps_lookup()
 *
 * <li> AVL tree of all \a ".list" files located in the same directory (and subdirectories) as cache file.
 *   @n Each key of this tree is a name of a group from \a ".list" file (e.g. \a Default \a Applications).
 *   @n Each value of this tree is an AVL tree of entries, which in turns is an AVL tree of
 *   {mime type, \a ".desktop" files} pairs in the format: @n\a "text/plain=kde4-kate.desktop;diffuse.desktop;".
 *   @note Global version of this tree is used to resolve \a ".desktop" files according to
 *   the given mime type.
 *   @see xdg_default_apps_lookup()
 *        @n xdg_added_apps_lookup()
 *        @n xdg_removed_apps_lookup()
 * </ul>
 *
 * @n What are the Global AVL trees mentioned above? Well the thing is, or even the problem is that we have
 * several directories with data (\a ".desktop" and \a ".list" files) and separate cache file for each directory.
 * This means that we could have reference to \a ".desktop" file which is located in a different directory.
 * More of that, this \a ".desktop" file could be in a valid cache file of that directory...
 * @n Despite this we would like to have a global picture of the registered applications in the system. That's why we
 * need the Global versions of this trees, which contains references to \a ".desktop" files in appropriate directory.
 * Also this approach gives as a possibility to rebuild the cache only in one data directory.
 *
 * @note Actually, this problem is solved a bit different way. There is no Global AVL trees, there is a list of
 * directories with their data. When data loaded for each directory we are fixing references between data of this
 * directories.
 * @n That's why XdgJointList appeared - to dynamically group (create references) data from different folders without
 * memory allocations.
 * @n Also, this is the root of thread unsafety.
 */
