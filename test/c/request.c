#include <c/nxt_c.h>

static nxt_int_t nxt_c_request_request_handler(nxt_unit_request_info_t *req);

NXT_EXPORT nxt_c_mod_t nxt_c_module = {
    .request_handler = nxt_c_request_request_handler,
};


static nxt_int_t
nxt_c_request_request_handler(nxt_unit_request_info_t *req)
{
    (void)(req);
    nxt_unit_debug(req->ctx, "request handler");
    return NXT_UNIT_OK;
}
