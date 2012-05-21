/** @mainpage
 *
 * @section intro_sec Introduction
 *
 * This library is a fork of the official freedesktop.org xdgmime library which is implementation of "Shared MIME-info Database" specification.
 *
 * In libxdg were added some missing features of the original version:
 *
 * @li C-implementation of AVL trees;
 * @li indexed (by using of AVL trees) access to all ".desktop" files and its contents (implementation of "Desktop Entry Specification");
 * @li cache all (include indexes) gathered information from all ".desktop" and ".list" files ("Desktop Entry Specification") into one single binary file which then can be mmap'ed without a single allocation of memory (just updates pointers in mmap'ed memory);
 * @li indexed (by using of AVL trees) access to icon themes and its contents (implemetation of "Icon Theme Specification" and "Icon Naming Specification");
 * @li implementation of "MIME actions specification".
 *
 * Library is licensed under GNU-LGPL and AFL v2.0.
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
 *
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
 *	- 4 \c bytes for version of a cache file.
 *
 *  - List of XdgFileWatcher structures (at least one zeroed structure, i.e NULL-terminated C list).
 *
 *  - AVL tree of all \a ".desktop" files located in the same directory as cache file.
 *	@n Each key of this tree is a name of \a ".desktop" file (e.g. \a kde4-kate.desktop).
 *	@n Each value of this tree is an AVL tree of XdgApp, which in turns is an AVL tree of
 *	XdgAppGroup, which is an AVL tree of XdgAppGroupEntry.
 *	@note This tree serves as container of \a ".desktop" files for other two major trees in cache.
 *
 *  - AVL tree of associations of mime type with \a ".desktop" files.
 *	@n Each key of this tree is a name of a mime type (e.g. \a text).
 *	@n Each value of this tree is an AVL tree of sub types (e.g. \a html), which contains a
 *	list of pointers to XdgApp structures.
 *	@note Global version of this tree is used to resolve \a ".desktop" files according to
 *	the given mime type.
 *
 *  - AVL tree of all \a ".list" files located in the same directory as cache file.
 *	@n Each key of this tree is a name of a group from \a ".list" file (e.g. \a Default \a Applications).
 *	@n Each value of this tree is an AVL tree of entries, which in turns is an AVL tree of
 *	{mime type, \a ".desktop" files} pairs in the format: \a "text/plain=kde4-kate.desktop;diffuse.desktop;".
 *	@note Global version of this tree is used to resolve \a ".desktop" files according to
 *	the given mime type.
 *
 */
