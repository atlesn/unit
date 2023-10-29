
/*
 * Copyright (C) Atle Solbakken
 * Copyright (C) NGINX, Inc.
 */

#ifndef _NXT_C_H_INCLUDED_
#define _NXT_C_H_INCLUDED_

#include <nxt_unit.h>
#include <nxt_clang.h>
#include <nxt_types.h>

struct nxt_c_mod_s {
    nxt_int_t (*request_handler)(nxt_unit_request_info_t *req);
};

typedef struct nxt_c_mod_s nxt_c_mod_t;

#endif /* _NXT_C_H_INCLUDED_ */
