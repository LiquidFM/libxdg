#include <xdg/xdg.h>
#include <stdio.h>


void print_app(const XdgApp *app)
{
	const XdgList *values;
	const XdgAppGroup *group = xdg_app_group_lookup(app, "Desktop Entry");

	if (values = xdg_list_begin(xdg_app_localized_entry_lookup(group, "Name", "ru", "RU", NULL)))
		do
		{
			printf("Name = %s", xdg_list_item_app_group_entry_value(values));
		}
		while (values = xdg_list_next(values));

	if (values = xdg_list_begin(xdg_app_localized_entry_lookup(group, "GenericName", "ru", "RU", NULL)))
		do
		{
			printf("GenericName = %s", xdg_list_item_app_group_entry_value(values));
		}
		while (values = xdg_list_next(values));
}

void print_applications(const char *mime)
{
	const XdgApp *app;
	const XdgJointList *apps;
	const XdgJointList *removed_apps = xdg_removed_apps_lookup(mime);

	if (apps = xdg_joint_list_begin(xdg_added_apps_lookup(mime)))
		do
		{
			app = xdg_joint_list_item_app(apps);

			if (xdg_joint_list_contains_app(removed_apps, app) == 0)
				print_app(app);
		}
		while (apps = xdg_joint_list_next(apps));

	if (apps = xdg_joint_list_begin(xdg_default_apps_lookup(mime)))
		do
		{
			app = xdg_joint_list_item_app(apps);

			if (xdg_joint_list_contains_app(removed_apps, app) == 0)
				print_app(app);
		}
		while (apps = xdg_joint_list_next(apps));


	if (apps = xdg_joint_list_begin(xdg_known_apps_lookup(mime)))
		do
		{
			app = xdg_joint_list_item_app(apps);

			if (xdg_joint_list_contains_app(removed_apps, app) == 0)
				print_app(app);
		}
		while (apps = xdg_joint_list_next(apps));
}

void main()
{
	xdg_init();
	print_applications("text/plain");
	xdg_shutdown();
}
