#include "libudev.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <poll.h>
#include <pthread.h>

#include <libevdev/libevdev.h>


struct udev_device {
  struct udev *udev;
  int refcount;
  char syspath[32];
  dev_t devnum;
  char const *sysname;
  char const *action;
  char const *subsystem;
  struct udev_list_entry *properties_list;
};
struct udev {
  int refcount;
};
struct udev_list_entry {
  char name[32];
  int has_value;
  char value[32];
  struct udev_list_entry *next;
};
struct udev_monitor {
  struct udev *udev;
  int refcount;
  int scan_for_input;
  int pipe_fds[2];
  pthread_t devd_thread;
  int devd_socket;
};
struct udev_enumerate {
  int refcount;
  int scan_for_input;
  struct udev_list_entry *dev_list;
};



static struct udev_list_entry *create_list_entry_name_value(
      char const *name, char const *value) {
  struct udev_list_entry *le = calloc(1, sizeof(struct udev_list_entry));
  if (!le)
    return NULL;
  snprintf(le->name, sizeof(le->name), "%s", name);
  if (value) {
    le->has_value = 1;
    snprintf(le->value, sizeof(le->value), "%s", value);
  }
  return le;
}

static struct udev_list_entry *create_list_entry_name(char const *name) {
  return create_list_entry_name_value(name, NULL);
}

static void free_dev_list(struct udev_list_entry **list) {
  if (!*list)
    return;

  if ((*list)->next)
    free_dev_list(&(*list)->next);

  free(*list);
  *list = NULL;
}



struct udev_device *udev_device_new_from_devnum(struct udev *udev, char type,
                                                dev_t devnum) {
  fprintf(stderr, "udev_device_new_from_devnum %d\n", (int) devnum);

  char path[32];
  struct stat st;

  for (int i = 0; i < 100; ++i) {
    snprintf(path, sizeof(path), "/dev/input/event%d", i);

    fprintf(stderr, "path: %s\n", path);

    if (stat(path, &st) == 0 && st.st_rdev == devnum) {
      fprintf(stderr, "  %s\n", path + 11);
      return udev_device_new_from_syspath(udev, path);
    }
  }

  return NULL;
}

char const *udev_device_get_devnode(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_devnode\n");
  return udev_device->syspath;
}

dev_t udev_device_get_devnum(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_devnum\n");
  return udev_device->devnum;
}

char const *udev_device_get_property_value(struct udev_device *dev,
                                           char const *property) {
  fprintf(stderr, "udev_device_get_property_value %s\n", property);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, dev->properties_list) {
    if (strcmp(entry->name, property) == 0) {
      return entry->value;
    }
  }

  return NULL;
}

struct udev *udev_device_get_udev(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_udev\n");
  return udev_device->udev;
}

static int populate_properties_list(struct udev_device *udev_device) {
  int ret = 0;

  char const *ids[] = { "ID_INPUT", "ID_INPUT_TOUCHPAD",
    "ID_INPUT_MOUSE", "ID_INPUT_KEYBOARD" };

  int fd = open(udev_device->syspath, O_RDONLY | O_NONBLOCK);
  if (fd == -1) {
    return -1;
  }

  struct libevdev *evdev = NULL;
  if (libevdev_new_from_fd(fd, &evdev) != 0) {
    fprintf(stderr,
            "udev_device_get_property_value: could not create evdev\n");
    close(fd);
    return -1;
  }

  for (int i = 0; i < nitems(ids); ++i) {
    char const *id = ids[i];
    struct udev_list_entry *le;

    if (strcmp(id, "ID_INPUT") == 0) {
      le = create_list_entry_name_value(id, NULL);
    } else if (strcmp(id, "ID_INPUT_TOUCHPAD") == 0) {
      if (libevdev_has_event_code(evdev, EV_ABS, ABS_X) &&
          libevdev_has_event_code(evdev, EV_ABS, ABS_Y) &&
          libevdev_has_event_code(evdev, EV_KEY, BTN_TOOL_FINGER) &&
          !libevdev_has_event_code(evdev, EV_KEY, BTN_STYLUS) &&
          !libevdev_has_event_code(evdev, EV_KEY, BTN_TOOL_PEN)) {
        le = create_list_entry_name_value(id, NULL);
      } else {
        continue;
      }
    } else if (strcmp(id, "ID_INPUT_MOUSE") == 0) {
      int is_mouse = 0;
      if (libevdev_has_event_code(evdev, EV_REL, REL_X) &&
          libevdev_has_event_code(evdev, EV_REL, REL_Y) &&
          libevdev_has_event_code(evdev, EV_KEY, BTN_MOUSE)) {
        is_mouse = 1;
      }
      if (libevdev_has_event_code(evdev, EV_ABS, ABS_X) &&
          libevdev_has_event_code(evdev, EV_ABS, ABS_Y) &&
          !libevdev_has_event_code(evdev, EV_KEY, BTN_TOOL_FINGER) &&
          !libevdev_has_event_code(evdev, EV_KEY, BTN_STYLUS) &&
          !libevdev_has_event_code(evdev, EV_KEY, BTN_TOOL_PEN) &&
          libevdev_has_event_code(evdev, EV_KEY, BTN_MOUSE)) {
        is_mouse = 1;
      }
      if (is_mouse) {
        le = create_list_entry_name_value(id, NULL);
      } else {
        continue;
      }
    } else if (strcmp(id, "ID_INPUT_KEYBOARD") == 0) {
      bool is_keyboard = true;
      for (int i = KEY_ESC; i <= KEY_D; ++i) {
        if (!libevdev_has_event_code(evdev, EV_KEY, i)) {
          is_keyboard = false;
          break;
        }
      }
      if (is_keyboard) {
        le = create_list_entry_name_value(id, NULL);
      } else {
        continue;
      }
    } else {
      continue;
    }

    if (!le) {
      free_dev_list(&udev_device->properties_list);
      ret = -1;
      goto out;
    }

    struct udev_list_entry **list_end = &udev_device->properties_list;
    while (*list_end) {
      list_end = &((*list_end)->next);
    }

    *list_end = le;
  }

out:
  libevdev_free(evdev);
  close(fd);

  return ret;
}

struct udev_device *udev_device_new_from_syspath(struct udev *udev,
                                                        const char *syspath) {
  fprintf(stderr, "udev_device_new_from_syspath %s\n", syspath);
  struct udev_device *u = calloc(1, sizeof(struct udev_device));
  if (u) {
    struct stat st;
    if (stat(syspath, &st) == 0) {
      u->devnum = st.st_rdev;
    } else {
      free(u);
      return NULL;
    }

    // TODO: increase refcount?
    u->udev = udev;
    u->refcount = 1;
    snprintf(u->syspath, sizeof(u->syspath), "%s", syspath);
    u->sysname = (char const *)u->syspath + 11;
    u->subsystem = "input";

    if (populate_properties_list(u) == -1) {
      udev_device_unref(u);
      return NULL;
    }

    return u;
  }
  return NULL;
}

const char *udev_device_get_syspath(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_syspath\n");
  return udev_device->syspath;
}

const char *udev_device_get_sysname(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_sysname\n");
  return udev_device->sysname;
}

const char *udev_device_get_subsystem(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_subsystem\n");
  return udev_device->subsystem;
}

const char *udev_device_get_sysattr_value(
    struct udev_device *udev_device, const char *sysattr) {
  fprintf(stderr, "stub: udev_device_get_sysattr_value %s\n", sysattr);
  return NULL;
}

struct udev_list_entry *udev_device_get_properties_list_entry(
    struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_properties_list_entry\n");
  return udev_device->properties_list;
}

struct udev_device *udev_device_ref(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_ref\n");
  ++udev_device->refcount;
  return udev_device;
}

void udev_device_unref(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_unref %p %d\n", udev_device,
          udev_device->refcount);

  --udev_device->refcount;
  if (udev_device->refcount == 0) {
    free_dev_list(&udev_device->properties_list);
    free(udev_device);
  }
}

struct udev_device *udev_device_get_parent(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_parent %p %d\n", udev_device,
          udev_device->refcount);
  return NULL;
}

int udev_device_get_is_initialized(struct udev_device *udev_device) {
  fprintf(stderr, "udev_device_get_is_initialized %p %d\n", udev_device,
          udev_device->refcount);
  return 1;
}

struct udev *udev_ref(struct udev *udev) {
  fprintf(stderr, "udev_ref\n");
  ++udev->refcount;
  return udev;
}

void udev_unref(struct udev *udev) {
  fprintf(stderr, "udev_unref\n");
  --udev->refcount;
  if (udev->refcount == 0) {
    free(udev);
  }
}

struct udev *udev_new(void) {
  fprintf(stderr, "udev_new\n");
  struct udev *u = calloc(1, sizeof(struct udev));
  if (u) {
    u->refcount = 1;
    return u;
  }
  return NULL;
}

struct udev_enumerate *udev_enumerate_new(struct udev *udev) {
  fprintf(stderr, "udev_enumerate_new\n");
  struct udev_enumerate *u = calloc(1, sizeof(struct udev_enumerate));
  if (u) {
    u->refcount = 1;
    return u;
  }
  return NULL;
}

int udev_enumerate_add_match_subsystem(
    struct udev_enumerate *udev_enumerate, const char *subsystem) {
  fprintf(stderr, "udev_enumerate_add_match_subsystem\n");
  if (strcmp(subsystem, "input") != 0) {
    return -1;
  } else {
    udev_enumerate->scan_for_input = 1;
    return 0;
  }
}

int udev_enumerate_scan_devices(struct udev_enumerate *udev_enumerate) {
  fprintf(stderr, "udev_enumerate_scan_devices\n");

  if (!udev_enumerate->scan_for_input) {
    return 0;
  }

  char path[32];

  for (int i = 0; i < 100; ++i) {
    snprintf(path, sizeof(path), "/dev/input/event%d", i);

    if (access(path, R_OK) != 0) {
      continue;
    }

    struct udev_list_entry *le = create_list_entry_name(path);
    if (!le) {
      free_dev_list(&udev_enumerate->dev_list);
      return -1;
    }

    fprintf(stderr, "udev_enumerate_scan_devices, added %s\n", path);

    struct udev_list_entry **list_end = &udev_enumerate->dev_list;
    while (*list_end) {
      list_end = &((*list_end)->next);
    }

    *list_end = le;
  }

  struct udev_list_entry *entry;
  udev_list_entry_foreach(entry, udev_enumerate->dev_list) {
    fprintf(stderr, "udev_enumerate_scan_devices, list %s\n", entry->name);
  }
  fprintf(stderr, "udev_enumerate_scan_devices, list end\n");

  return 0;
}

struct udev_list_entry *udev_enumerate_get_list_entry(
    struct udev_enumerate *udev_enumerate) {
  return udev_enumerate->dev_list;
}

int udev_enumerate_add_match_sysname(
    struct udev_enumerate *udev_enumerate, const char *sysname) {
  fprintf(stderr, "stub: udev_enumerate_add_match_sysname\n");
  return -1;
}

const char *udev_list_entry_get_name(
    struct udev_list_entry *list_entry) {
  fprintf(stderr, "udev_list_entry_get_name\n");
  return list_entry->name;
}

const char *udev_list_entry_get_value(struct udev_list_entry *list_entry) {
  fprintf(stderr, "udev_list_entry_get_name\n");
  return list_entry->has_value ? list_entry->value : NULL;
}

struct udev_list_entry *udev_list_entry_get_next(
    struct udev_list_entry *list_entry) {
  return list_entry->next;
}

void udev_enumerate_unref(struct udev_enumerate *udev_enumerate) {
  fprintf(stderr, "udev_enumerate_unref\n");
  --udev_enumerate->refcount;
  if (udev_enumerate->refcount == 0) {
    free_dev_list(&udev_enumerate->dev_list);
    free(udev_enumerate);
  }
}

struct udev_monitor *udev_monitor_new_from_netlink(struct udev *udev,
                                                          const char *name) {
  fprintf(stderr, "udev_monitor_new_from_netlink %p\n", udev);

  if (name == NULL || strcmp(name, "udev") != 0) {
    return NULL;
  }

  struct udev_monitor *u = calloc(1, sizeof(struct udev_monitor));
  if (!u) {
    return NULL;
  }

  if (pipe2(u->pipe_fds, O_CLOEXEC) == -1) {
    free(u);
    return NULL;
  }

  // TODO: increase refcount?
  u->udev = udev;
  u->devd_socket = -1;
  u->refcount = 1;

  return u;
}

int udev_monitor_filter_add_match_subsystem_devtype(
    struct udev_monitor *udev_monitor, const char *subsystem,
    const char *devtype) {
  fprintf(stderr, "udev_monitor_filter_add_match_subsystem_devtype\n");

  if (devtype != NULL) {
    return -1;
  }

  if (subsystem == NULL || strcmp(subsystem, "input") != 0) {
    return -1;
  } else {
    udev_monitor->scan_for_input = 1;
    return 0;
  }
}

static void *devd_listener(void *arg) {
  struct udev_monitor *udev_monitor = (struct udev_monitor *)arg;

  for (;;) {
    int cmp;
    ssize_t len;
    char event[1024];

    struct pollfd pfd = {udev_monitor->devd_socket, POLLIN};
    int ret = poll(&pfd, 1, INFTIM);
    if (ret <= 0 || !(pfd.revents & POLLIN)) {
      return NULL;
    }

    len = recv(udev_monitor->devd_socket, event, sizeof(event) - 1, MSG_WAITALL);
    if (len == -1) {
      perror("recv");
      return (void *) 1;
    }

    if (!udev_monitor->scan_for_input) {
      continue;
    }

    event[len] = '\0';

    if (len >= 1 && event[len - 1] == '\n') {
      event[len - 1] = '\0';
    }

    char *device = strstr(event, "cdev=input/event");
    if (!device) {
      continue;
    }

    char msg[32] = {0};
    snprintf(&msg[1], sizeof(msg) - 1, "%s", device + 5);
    if (strstr(event, "type=CREATE") != NULL) {
      msg[0] = '+';
    } else if (strstr(event, "type=DESTROY") != NULL) {
      msg[0] = '-';
    } else {
      continue;
    }

    write(udev_monitor->pipe_fds[1], msg, 32);
  }

  return NULL;
}

int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor) {
  fprintf(stderr, "udev_monitor_enable_receiving\n");

  struct sockaddr_un devd_addr;

  memset(&devd_addr, 0, sizeof(devd_addr));
  devd_addr.sun_family = PF_LOCAL;
  strlcpy(devd_addr.sun_path, "/var/run/devd.seqpacket.pipe",
          sizeof(devd_addr.sun_path));

  udev_monitor->devd_socket = socket(PF_LOCAL, SOCK_SEQPACKET, 0);
  if (udev_monitor->devd_socket == -1) {
    perror("socket");
    return -1;
  }
  int error = connect(udev_monitor->devd_socket, (struct sockaddr *)&devd_addr,
                      SUN_LEN(&devd_addr));
  if (error == -1) {
    perror("connect");
    close(udev_monitor->devd_socket);
    udev_monitor->devd_socket = -1;
    return -1;
  }

  if (pthread_create(&udev_monitor->devd_thread, NULL, devd_listener,
      udev_monitor) == 0) {
    return 0;
  } else {
    close(udev_monitor->devd_socket);
    udev_monitor->devd_socket = -1;
    return -1;
  }
}

int udev_monitor_get_fd(struct udev_monitor *udev_monitor) {
  fprintf(stderr, "udev_monitor_get_fd\n");
  return udev_monitor->pipe_fds[0];
}

struct udev *udev_monitor_get_udev(struct udev_monitor *udev_monitor) {
  fprintf(stderr, "udev_monitor_get_udev\n");
  return udev_monitor->udev;
}

struct udev_device *udev_monitor_receive_device(
    struct udev_monitor *udev_monitor) {
  fprintf(stderr, "udev_monitor_receive_device\n");

  char msg[32];
  if (read(udev_monitor->pipe_fds[0], msg, 32) != 32) {
    return NULL;
  }

  char path[32];
  snprintf(path, sizeof(path), "/dev/%s", &msg[1]);

  struct udev_device *udev_device =
      udev_device_new_from_syspath(udev_monitor->udev, path);

  if (!udev_device) {
    return NULL;
  }

  if (msg[0] == '+') {
    udev_device->action = "add";
  } else if (msg[0] == '-') {
    udev_device->action = "remove";
  }

  return udev_device;
}

const char *udev_device_get_action(struct udev_device *udev_device) {
  fprintf(stderr, "stub: udev_device_get_action\n");
  return udev_device->action;
}

void udev_monitor_unref(struct udev_monitor *udev_monitor) {
  fprintf(stderr, "udev_monitor_unref\n");
  --udev_monitor->refcount;
  if (udev_monitor->refcount == 0) {
    if (udev_monitor->devd_socket != -1) {
      close(udev_monitor->devd_socket);
      pthread_join(udev_monitor->devd_thread, NULL);
      udev_monitor->devd_socket = -1;
    }
    close(udev_monitor->pipe_fds[0]);
    close(udev_monitor->pipe_fds[1]);
    free(udev_monitor);
  }
}
