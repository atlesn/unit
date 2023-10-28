
/*
 * Copyright (C) Atle Solbakken
 * Copyright (C) NGINX, Inc.
 */

#include <nxt_main.h>
#include <nxt_application.h>

static uint32_t  nxt_c_compat[] = {
    NXT_VERNUM, NXT_DEBUG,
};

static nxt_int_t nxt_c_start(nxt_task_t *task,
    nxt_process_data_t *data);

NXT_EXPORT nxt_app_module_t  nxt_app_module = {
    sizeof(nxt_c_compat),
    nxt_c_compat,
    nxt_string("c"),
    "0.1",
    NULL,
    0,
    NULL,
    nxt_c_start,
};

static nxt_int_t
nxt_c_start(nxt_task_t *task, nxt_process_data_t *data)
{
    (void)(task);
    (void)(data);
    printf("In %s\n", __func__);
    nxt_assert(0);
    return NXT_OK;
    //fail: return NXT_ERROR;
}
