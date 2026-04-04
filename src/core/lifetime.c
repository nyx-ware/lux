#include "lux/core.h"
#include "lux/gl.h"
#include "core.h"
#include "../debug/debug.h"
#include "../gl/gl.h"

#include <stdlib.h>
#include <string.h>

// private header 
// ---------------------------------------------------------------- 

lx_init_props lt_props = {};
global_store* lt_store = NULL;

// public header
// ----------------------------------------------------------------

int lx_init(lx_init_props props)
{
    lt_props = props;
    
    GUARD(lt_store != NULL, ("failed to initialise lux, it has already been initialised"), 0);
    GUARD(props.title == NULL, ("failed to initialise lux with a null title"), 0);
    GUARD(props.width > 8192 || props.width <= 0, ("failed to initialise lux with invalid width of %d (0-8192)", props.width), 0);
    GUARD(props.height > 4320 || props.height <= 0, ("failed to initialise lux with invalid height of %d (0-4320)", props.height), 0);
    GUARD(props.on_resize == NULL, ("failed to initialise lux with null resize callback"), 0);
    GUARD(props.on_error == NULL, ("failed to initialise lux with null error callback"), 0);

    lt_store = malloc(sizeof(global_store));
    if (lt_store == NULL)
    {
        lx_error("failed to allocate internal store");
        return 1;
    }

    if (!window_create() || !gl_load())
        return 1;

    memset(lt_store->key_tracker, LX_RELEASED, sizeof(lt_store->key_tracker));
    lt_store->mouse_tracker = (lx_mousepos){ 0, 0 };
    lt_store->scroll_amount = 0;

    glViewport(0, 0, props.width, props.height);

    lt_store->alive = 1;
    return 0;
}

void lx_quit()
{
    GUARD(lt_store == NULL, ("failed to quit lux, it has not been initialised"));

    gl_unload();
    window_destroy();

    free(lt_store);
    lt_store = NULL;
}

int lx_is_alive()
{
    return lt_store == NULL ? 0 : lt_store->alive;
}

double lx_get_loaded_gl_version()
{
    if (lt_store == NULL)
        return 0;

    int major = lt_store->gl_version / 10000;
    int minor = lt_store->gl_version % 10000;

    return lt_store == NULL ? 0 : major + (minor / 10.0);
}
