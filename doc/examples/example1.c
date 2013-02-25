#include <xdg/xdg.h>
#include <stdio.h>


void print_app(const XdgApp *app)
{
	const XdgList *values;
	const XdgAppGroup *group = xdg_app_group_lookup(app, "Desktop Entry");

	if (values = xdg_list_head(xdg_app_localized_entry_lookup(group, "Name", "ru", "RU", NULL)))
		do
			printf("Name = %s", xdg_list_item_app_group_entry_value(values));
		while (values = xdg_list_next(values));

	if (values = xdg_list_head(xdg_app_localized_entry_lookup(group, "GenericName", "ru", "RU", NULL)))
		do
			printf("GenericName = %s", xdg_list_item_app_group_entry_value(values));
		while (values = xdg_list_next(values));
}

void print_applications(const char *mime)
{
	const XdgJointList *apps;

	if (apps = xdg_joint_list_head(xdg_apps_lookup(mime)))
		do
			print_app(xdg_joint_list_item_app(apps));
		while (apps = xdg_joint_list_next(apps));

	if (apps = xdg_joint_list_head(xdg_known_apps_lookup(mime)))
		do
			print_app(xdg_joint_list_item_app(apps));
		while (apps = xdg_joint_list_next(apps));
}

void main()
{
	xdg_init();
	print_applications("text/plain");
	xdg_shutdown();
}
