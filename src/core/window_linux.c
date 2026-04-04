#include "core.h"
#include "../debug/debug.h"
#include "../platform/xdg_linux.h"
#include "../platform/xdg_deco_linux.h"
#include "../input/input.h"

#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <wayland-client-core.h>
#include <wayland-client.h>
#include <wayland-egl.h>

typedef struct _window_store
{
    struct wl_display* wl_display;
    struct wl_registry* wl_registry;
    struct wl_compositor* wl_compositor;
    struct wl_surface* wl_surface;

    struct wl_seat* wl_seat;
    struct wl_keyboard* wl_keyboard;
    struct wl_pointer* wl_pointer;

    struct xdg_wm_base* xdg_wm_base;
    struct xdg_surface* xdg_surface;
    struct xdg_toplevel* xdg_toplevel;
    struct zxdg_decoration_manager_v1* xdg_deco;

    EGLDisplay* egl_display;
    EGLContext* egl_context;
    struct wl_egl_window* egl_window;
    EGLSurface* egl_surface;

    double time_began;
    double last_frame_time;
    double cur_frame_time;
    double delta_time;

    int xdg_ack;
    int axis_handled;
}
window_store;

// private source
// ---------------------------------------------------------------- 

static struct wl_seat_listener wl_listener_seat;
static struct wl_keyboard_listener wl_listener_keyboard;
static struct wl_pointer_listener wl_listener_pointer;

static void wl_registry_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    if (strcmp(interface, wl_compositor_interface.name) == 0)
        lt_store->window->wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);

    if (strcmp(interface, xdg_wm_base_interface.name) == 0)
        lt_store->window->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);

    if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
        lt_store->window->xdg_deco = wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, version);

    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        lt_store->window->wl_seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
        wl_seat_add_listener(lt_store->window->wl_seat, &wl_listener_seat, NULL);
    }
}

static void wl_registry_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{}

static void wl_seat_capabilities(void* data, struct wl_seat* seat, enum wl_seat_capability caps)
{
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        lt_store->window->wl_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(lt_store->window->wl_keyboard, &wl_listener_keyboard, NULL);
    }

    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
        lt_store->window->wl_pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(lt_store->window->wl_pointer, &wl_listener_pointer, NULL);
    }
}

static void wl_seat_name(void* data, struct wl_seat* seat, const char* name)
{}

static void wl_keyboard_key(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    lx_keycode kc = code_to_key(key);

    if (kc == LX_KEY_UNKNOWN)
        return;

    change_key_state(kc, state == WL_KEYBOARD_KEY_STATE_PRESSED ? LX_PRESSED : LX_RELEASED);
}

static void wl_keyboard_keymap(void* data, struct wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size)
{
    close(fd);
}

static void wl_keyboard_enter(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
{}

static void wl_keyboard_leave(void* data, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface)
{}

static void wl_keyboard_modifiers(void* data, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{}

static void wl_keyboard_repeat_info(void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay)
{}

static void wl_pointer_motion(void* data, struct wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    update_mouse_position(wl_fixed_to_double(sx), wl_fixed_to_double(sy));
}

static void wl_pointer_button(void* data, struct wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    lx_keycode kc = code_to_key(button);

    if (kc == LX_KEY_UNKNOWN)
        return;

    change_key_state(kc, state == WL_POINTER_BUTTON_STATE_PRESSED ? LX_PRESSED : LX_RELEASED);
}

static void wl_pointer_axis(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL || lt_store->window->axis_handled == 1)
        return;

    update_mouse_scroll(wl_fixed_to_double(value) / 15.0);
    lt_store->window->axis_handled = 1;
}

static void wl_pointer_axis_discrete(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t discrete)
{
    if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL || lt_store->window->axis_handled == 1)
        return;

    update_mouse_scroll((double)discrete);
    lt_store->window->axis_handled = 1;
}

static void wl_pointer_frame(void* data, struct wl_pointer* pointer)
{
    lt_store->window->axis_handled = 0;
}

static void wl_pointer_enter(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{}

static void wl_pointer_leave(void* data, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface)
{}

static void wl_pointer_axis_source(void* data, struct wl_pointer* pointer, uint32_t axis_source)
{}

static void wl_pointer_axis_stop(void* data, struct wl_pointer* pointer, uint32_t time, uint32_t axis)
{}

static void wl_pointer_axis_value120(void* data, struct wl_pointer* pointer, uint32_t axis, int32_t value120)
{}

static void wl_pointer_axis_relative_direction(void* data, struct wl_pointer* wl_pointer, uint32_t axis, uint32_t direction)
{}

static void xdg_wm_base_ping(void* data, struct xdg_wm_base* wm_base, uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}

static void xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial)
{
    xdg_surface_ack_configure(surface, serial);
    lt_store->window->xdg_ack = 1;
}

static void xdg_toplevel_close(void* data, struct xdg_toplevel* toplevel)
{
    lt_store->alive = 0;
}

static void xdg_toplevel_configure(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states)
{
    if (width <= 0 || height <= 0)
        return;

    lt_props.width = width;
    lt_props.height = height;

    wl_egl_window_resize(lt_store->window->egl_window, width, height, 0, 0);

    if (lt_props.on_resize != NULL && lt_store->gl_version > 0)
        lt_props.on_resize(width, height);
}

static void xdg_toplevel_configure_bounds(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height)
{}

static void xdg_toplevel_wm_capabilities(void* data, struct xdg_toplevel* toplevel, struct wl_array* capabilities)
{}

static struct wl_registry_listener wl_listener_registry =
{
    .global = wl_registry_global,
    .global_remove = wl_registry_global_remove 
};

static struct wl_seat_listener wl_listener_seat =
{
    .capabilities = wl_seat_capabilities,
    .name = wl_seat_name
};

static struct wl_keyboard_listener wl_listener_keyboard =
{
    .key = wl_keyboard_key,
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info
};

static struct wl_pointer_listener wl_listener_pointer =
{
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .axis_discrete = wl_pointer_axis_discrete,
    .frame = wl_pointer_frame,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_value120 = wl_pointer_axis_value120,
    .axis_relative_direction = wl_pointer_axis_relative_direction
};

static struct xdg_wm_base_listener xdg_listener_wm_base =
{
    .ping = xdg_wm_base_ping
};

static struct xdg_surface_listener xdg_listener_surface =
{
    .configure = xdg_surface_configure
};

static struct xdg_toplevel_listener xdg_listener_toplevel =
{
    .close = xdg_toplevel_close,
    .configure = xdg_toplevel_configure,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities = xdg_toplevel_wm_capabilities 
};

static int create_wayland_window()
{
    lt_store->window->wl_display = wl_display_connect(NULL);
    if (!lt_store->window->wl_display)
    {
        lx_error("failed to connect to wayland display");
        return 0;
    }

    lt_store->window->wl_registry = wl_display_get_registry(lt_store->window->wl_display);
    if (!lt_store->window->wl_registry)
    {
        lx_error("failed to get wayland display registry");
        return 0;
    }

    wl_registry_add_listener(lt_store->window->wl_registry, &wl_listener_registry, NULL);

    if (wl_display_roundtrip(lt_store->window->wl_display) < 0)
    {
        lx_error("failed to complete wayland round trip");
        return 0;
    }

    if (!lt_store->window->wl_compositor)
    {
        lx_error("failed to get wayland compositor");
        return 0;
    }

    lt_store->window->wl_surface = wl_compositor_create_surface(lt_store->window->wl_compositor);
    if (!lt_store->window->wl_surface)
    {
        lx_error("failed to create wayland surface");
        return 0;
    }

    return 1;
}

static void destroy_wayland_window()
{
    if (lt_store->window->wl_pointer != NULL)
        wl_pointer_destroy(lt_store->window->wl_pointer);

    if (lt_store->window->wl_keyboard != NULL)
        wl_keyboard_destroy(lt_store->window->wl_keyboard);

    if (lt_store->window->wl_seat != NULL)
        wl_seat_destroy(lt_store->window->wl_seat);

    if (lt_store->window->wl_surface != NULL)
        wl_surface_destroy(lt_store->window->wl_surface);

    if (lt_store->window->wl_compositor != NULL)
        wl_compositor_destroy(lt_store->window->wl_compositor);

    if (lt_store->window->wl_registry != NULL)
        wl_registry_destroy(lt_store->window->wl_registry);

    if (lt_store->window->wl_display != NULL)
        wl_display_disconnect(lt_store->window->wl_display);
}

static int create_xdg_shell()
{
    if (!lt_store->window->xdg_wm_base)
    {
        lx_error("failed to get xdg wm base");
        return 0;
    }

    xdg_wm_base_add_listener(lt_store->window->xdg_wm_base, &xdg_listener_wm_base, NULL);

    lt_store->window->xdg_surface = xdg_wm_base_get_xdg_surface(lt_store->window->xdg_wm_base, lt_store->window->wl_surface);
    if (!lt_store->window->xdg_surface)
    {
        lx_error("failed to create xdg surface");
        return 0;
    }

    xdg_surface_add_listener(lt_store->window->xdg_surface, &xdg_listener_surface, NULL);

    lt_store->window->xdg_toplevel = xdg_surface_get_toplevel(lt_store->window->xdg_surface);
    if (!lt_store->window->xdg_toplevel)
    {
        lx_error("failed to create xdg toplevel");
        return 0;
    }

    xdg_toplevel_set_title(lt_store->window->xdg_toplevel, lt_props.title);
    xdg_toplevel_set_app_id(lt_store->window->xdg_toplevel, lt_props.title);
    xdg_toplevel_add_listener(lt_store->window->xdg_toplevel, &xdg_listener_toplevel, NULL);

    // TODO: this only works on KDE, see if you can implement libdecor to deal with it for us
    if (lt_store->window->xdg_deco)
    {
        struct zxdg_toplevel_decoration_v1* deco = zxdg_decoration_manager_v1_get_toplevel_decoration(lt_store->window->xdg_deco, lt_store->window->xdg_toplevel); 
        //zxdg_toplevel_decoration_v1_set_mode(deco, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    wl_surface_commit(lt_store->window->wl_surface);
    while (lt_store->window->xdg_ack == 0)
        wl_display_dispatch(lt_store->window->wl_display);

    return 1;
}

static void destroy_xdg_shell()
{
    if (lt_store->window->xdg_deco != NULL)
        zxdg_decoration_manager_v1_destroy(lt_store->window->xdg_deco);

    if (lt_store->window->xdg_toplevel != NULL)
        xdg_toplevel_destroy(lt_store->window->xdg_toplevel);

    if (lt_store->window->xdg_surface != NULL)
        xdg_surface_destroy(lt_store->window->xdg_surface);

    if (lt_store->window->xdg_wm_base != NULL)
        xdg_wm_base_destroy(lt_store->window->xdg_wm_base);
}

static int create_egl_surface()
{
    lt_store->window->egl_display = eglGetDisplay((EGLNativeDisplayType)lt_store->window->wl_display); 
    if (lt_store->window->egl_display == EGL_NO_DISPLAY)
    {
        lx_error("failed to create egl display");
        return 0;
    }

    if (eglInitialize(lt_store->window->egl_display, NULL, NULL) != EGL_TRUE)
    {
        lx_error("failed to initialise egl");
        return 0;
    }

    EGLConfig config;
    EGLint num_configs;
    EGLint attribs[] =
    {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    if (eglChooseConfig(lt_store->window->egl_display, attribs, &config, 1, &num_configs) != EGL_TRUE)
    {
        lx_error("failed to choose egl config");
        return 0;
    }

    EGLint context_attribs[] = { EGL_NONE };
    eglBindAPI(EGL_OPENGL_API);
    
    lt_store->window->egl_context = eglCreateContext(lt_store->window->egl_display, config, EGL_NO_CONTEXT, context_attribs);
    if (!lt_store->window->egl_context)
    {
        lx_error("failed to create egl context");
        return 0;
    }

    lt_store->window->egl_window = wl_egl_window_create(lt_store->window->wl_surface, lt_props.width, lt_props.height);
    if (!lt_store->window->egl_window)
    {
        lx_error("failed to create egl window");
        return 0;
    }

    lt_store->window->egl_surface = eglCreateWindowSurface(lt_store->window->egl_display, config, (EGLNativeWindowType)lt_store->window->egl_window, NULL);
    if (lt_store->window->egl_surface == EGL_NO_SURFACE)
    {
        lx_error("failed to create egl surface");
        return 0;
    }

    eglMakeCurrent(lt_store->window->egl_display, lt_store->window->egl_surface, lt_store->window->egl_surface, lt_store->window->egl_context);

    if (eglSwapInterval(lt_store->window->egl_display, 1) == EGL_FALSE)
    {
        lx_error("failed to enable vertical sync");
    }

    return 1;
}

static void destroy_egl_surface()
{
    if (lt_store->window->egl_surface != NULL)
        eglDestroySurface(lt_store->window->egl_display, lt_store->window->egl_surface);

    if (lt_store->window->egl_window != NULL)
        wl_egl_window_destroy(lt_store->window->egl_window);

    if (lt_store->window->egl_context != NULL)
        eglDestroyContext(lt_store->window->egl_display, lt_store->window->egl_context);

    if (lt_store->window->egl_display != NULL)
        eglTerminate(lt_store->window->egl_display);
}

static double get_time()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec + (time.tv_usec / 1000000.0);
}

// private header
// ---------------------------------------------------------------- 

int window_create()
{
    lt_store->window = malloc(sizeof(window_store));
    if (lt_store->window == NULL)
    {
        lx_error("failed to allocate internal window store");
        return 0;
    }

    if (!create_wayland_window() || !create_xdg_shell() || !create_egl_surface())
        return 0;

    double now = get_time();
    lt_store->window->time_began = now;
    lt_store->window->last_frame_time = now;
    lt_store->window->cur_frame_time = now;
    lt_store->window->delta_time = 0.0; 

    return 1;
}

void window_destroy()
{
    destroy_egl_surface();
    destroy_xdg_shell();
    destroy_wayland_window();
    
    free(lt_store->window);
    lt_store->window = NULL;
}

void window_poll_events()
{
    lt_store->window->last_frame_time = lt_store->window->cur_frame_time;
    lt_store->window->cur_frame_time = get_time();
    lt_store->window->delta_time = lt_store->window->cur_frame_time - lt_store->window->last_frame_time;

    reset_mouse_scroll();

    wl_display_dispatch_pending(lt_store->window->wl_display);
    wl_display_flush(lt_store->window->wl_display);
}

void window_swap_buffers()
{
    eglSwapBuffers(lt_store->window->egl_display, lt_store->window->egl_surface);
}

double window_get_time()
{
    return get_time() - lt_store->window->time_began;
}

double window_get_fps()
{
    if (lt_store->window->delta_time == 0.0)
        return 0;

    return 1.0 / lt_store->window->delta_time;
}

double window_get_delta()
{
    return lt_store->window->delta_time;
}
