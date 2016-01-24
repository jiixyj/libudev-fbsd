/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _PSMOUSE_H
#define _PSMOUSE_H
#define PSMOUSE_CMD_SETSCALE11 0x00e6
#define PSMOUSE_CMD_SETSCALE21 0x00e7
#define PSMOUSE_CMD_SETRES 0x10e8
#define PSMOUSE_CMD_GETINFO 0x03e9
#define PSMOUSE_CMD_SETSTREAM 0x00ea
#define PSMOUSE_CMD_SETPOLL 0x00f0
#define PSMOUSE_CMD_POLL 0x00eb
#define PSMOUSE_CMD_RESET_WRAP 0x00ec
#define PSMOUSE_CMD_GETID 0x02f2
#define PSMOUSE_CMD_SETRATE 0x10f3
#define PSMOUSE_CMD_ENABLE 0x00f4
#define PSMOUSE_CMD_DISABLE 0x00f5
#define PSMOUSE_CMD_RESET_DIS 0x00f6
#define PSMOUSE_CMD_RESET_BAT 0x02ff
#define PSMOUSE_RET_BAT 0xaa
#define PSMOUSE_RET_ID 0x00
#define PSMOUSE_RET_ACK 0xfa
#define PSMOUSE_RET_NAK 0xfe
enum psmouse_state {
 PSMOUSE_IGNORE,
 PSMOUSE_INITIALIZING,
 PSMOUSE_RESYNCING,
 PSMOUSE_CMD_MODE,
 PSMOUSE_ACTIVATED,
};
typedef enum {
 PSMOUSE_BAD_DATA,
 PSMOUSE_GOOD_DATA,
 PSMOUSE_FULL_PACKET
} psmouse_ret_t;
enum psmouse_type {
 PSMOUSE_NONE,
 PSMOUSE_PS2,
 PSMOUSE_PS2PP,
 PSMOUSE_THINKPS,
 PSMOUSE_GENPS,
 PSMOUSE_IMPS,
 PSMOUSE_IMEX,
 PSMOUSE_SYNAPTICS,
 PSMOUSE_ALPS,
 PSMOUSE_LIFEBOOK,
 PSMOUSE_TRACKPOINT,
 PSMOUSE_TOUCHKIT_PS2,
 PSMOUSE_CORTRON,
 PSMOUSE_HGPK,
 PSMOUSE_ELANTECH,
 PSMOUSE_FSP,
 PSMOUSE_SYNAPTICS_RELATIVE,
 PSMOUSE_CYPRESS,
 PSMOUSE_AUTO
};
#endif
