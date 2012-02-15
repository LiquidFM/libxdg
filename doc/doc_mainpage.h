/** @mainpage
 *
 * @section intro_sec Introduction
 *
 * This library is a fork of the official freedesktop.org library xdgmime which is implementation of "Shared MIME-info Database" specification.
 *
 * In exdgmime I've added some missing features of the original version:
 *
 * @li C-implementation of AVL trees;
 * @li indexed (by using of AVL trees) access to all ".desktop" files and its contents (implementation of "Desktop Entry Specification");
 * @li cache all (include indexes) gathered information from all ".desktop" and ".list" files ("Desktop Entry Specification") into one single binary file which then can be mmap'ed without a single allocation of memory (just updates pointers in mmap'ed memory);
 * @li indexed (by using of AVL trees) access to icon themes and its contents (implemetation of "Icon Theme Specification" and "Icon Naming Specification");
 * @li implementatioin of "MIME actions specification".
 *
 * All this code is licensed under GNU-LGPL and AFL v2.0.
 *
 *
 * @n @section example_sec Examples
 *
 * This sections describes a number of examples which should provide you with necessary information
 * to start using this library.
 *
 * @subsection example1 Example1. List all applications able to handle given mime type.
 *
 * @dontinclude examples/example1.c
 * First of all we must include library header:
 * @until >
 * Our \c print_applications function starts like this:
 * @skip print_applications
 * @until {
 * First we are initializing the library:
 * @skipline xdg_init
 * Now we should shutdown the library:
 * @line xdg_shutdown
 * @line }
 *
 *
 *
 *
 */
