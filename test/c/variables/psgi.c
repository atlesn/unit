#include <nxt_c_application.h>

static nxt_int_t
nxt_app_variables_init(nxt_task_t *task, nxt_common_app_conf_t *conf)
{
    (void)(task);
    (void)(conf);
    return NXT_OK;
}

NXT_EXPORT nxt_c_app_t nxt_c_app = {
    .init = nxt_app_variables_init,
};
