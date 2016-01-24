#ifndef LIBUDEV_FBSD_H_
#define LIBUDEV_FBSD_H_

#include <sys/stat.h>
#include <sys/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

struct udev;
struct udev_device;
struct udev_list_entry;
struct udev_monitor;
struct udev_enumerate;

struct udev *udev_new(void);
struct udev *udev_ref(struct udev *udev);
void udev_unref(struct udev *udev);

char const *udev_device_get_devnode(struct udev_device *udev_device);
dev_t udev_device_get_devnum(struct udev_device *udev_device);
char const *udev_device_get_property_value(
    struct udev_device *dummy __unused, char const *property);
struct udev *udev_device_get_udev(struct udev_device *dummy __unused);
struct udev_device *udev_device_new_from_syspath(
    struct udev *udev, char const *syspath);
struct udev_device *udev_device_new_from_devnum(
    struct udev *udev, char type, dev_t devnum);
char const *udev_device_get_syspath(struct udev_device *udev_device);
char const *udev_device_get_sysname(struct udev_device *udev_device);
char const *udev_device_get_subsystem(struct udev_device *udev_device);
char const *udev_device_get_sysattr_value(
    struct udev_device *udev_device, char const *sysattr);
struct udev_list_entry *udev_device_get_properties_list_entry(
    struct udev_device *udev_device);
struct udev_device *udev_device_ref(struct udev_device *udev_device);
void udev_device_unref(struct udev_device *udev_device);
struct udev_device *udev_device_get_parent(struct udev_device *udev_device);
int udev_device_get_is_initialized(struct udev_device *udev_device);
char const *udev_device_get_action(struct udev_device *udev_device);

struct udev_enumerate *udev_enumerate_new(struct udev *udev);
int udev_enumerate_add_match_subsystem(
    struct udev_enumerate *udev_enumerate, char const *subsystem);
int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate);
struct udev_list_entry *udev_enumerate_get_list_entry(
    struct udev_enumerate *udev_enumerate);
int udev_enumerate_add_match_sysname(
    struct udev_enumerate *udev_enumerate, char const *sysname);
void udev_enumerate_unref(struct udev_enumerate *udev_enumerate);

#define udev_list_entry_foreach(list_entry, first_entry)                      \
	for (list_entry = first_entry; list_entry;                            \
	     list_entry = udev_list_entry_get_next(list_entry))
char const *udev_list_entry_get_name(struct udev_list_entry *list_entry);
char const *udev_list_entry_get_value(struct udev_list_entry *list_entry);
struct udev_list_entry *udev_list_entry_get_next(
    struct udev_list_entry *list_entry);

struct udev_monitor *udev_monitor_new_from_netlink(
    struct udev *udev, char const *name);
int udev_monitor_filter_add_match_subsystem_devtype(
    struct udev_monitor *udev_monitor, char const *subsystem,
    char const *devtype);
int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor);
int udev_monitor_get_fd(struct udev_monitor *udev_monitor);
struct udev *udev_monitor_get_udev(struct udev_monitor *udev_monitor);
struct udev_device *udev_monitor_receive_device(
    struct udev_monitor *udev_monitor);
void udev_monitor_unref(struct udev_monitor *udev_monitor);

#endif
