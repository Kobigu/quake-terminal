/* Shim for the Debian-dropped X11/Xlib-xcb.h (X11-xcb merged into libX11 >= 1.8) */
#ifndef _X11_XLIB_XCB_SHIM_H_
#define _X11_XLIB_XCB_SHIM_H_
#include <X11/Xlib.h>
#include <X11/Xfuncproto.h>
#include <xcb/xcb.h>
_XFUNCPROTOBEGIN
typedef enum { XCBOwnsEventQueue, XlibOwnsEventQueue } XEventQueueOwner;
extern xcb_connection_t *XGetXCBConnection(Display *dpy);
extern void XSetEventQueueOwner(Display *dpy, XEventQueueOwner owner);
_XFUNCPROTOEND
#endif
