#include <nxt_unit.h>

struct nxt_c_mod_s {
    void (*request_handler)(nxt_unit_request_info_t *req);
};

typedef struct nxt_c_mod_s nxt_c_mod_t;
