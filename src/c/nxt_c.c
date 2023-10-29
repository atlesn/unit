
/*
 * Copyright (C) Atle Solbakken
 * Copyright (C) NGINX, Inc.
 */

#include <nxt_main.h>
#include <nxt_application.h>
#include <nxt_unit.h>
#include <nxt_time.h>

typedef struct {
    nxt_unit_ctx_t           *ctx;
    nxt_task_t               *task;
    nxt_c_app_conf_t         *c;
    nxt_pid_t                cc_pid;
    nxt_bool_t               cc_done;
} nxt_c_ctx_t;


static uint32_t  nxt_c_compat[] = {
    NXT_VERNUM, NXT_DEBUG,
};

static nxt_int_t nxt_c_start(nxt_task_t *task,
    nxt_process_data_t *data);
static void nxt_c_request_handler(nxt_unit_request_info_t *req);
static int nxt_c_ready_handler(nxt_unit_ctx_t *ctx);

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

static void
nxt_c_ctx_cleanup(nxt_c_ctx_t *cctx)
{
    // Nothing to do yet
}

static nxt_int_t
nxt_c_compile(nxt_task_t *task, nxt_c_app_conf_t *c, nxt_pid_t *cc_pid)
{
    nxt_int_t              rc;
    size_t                 name_length;
    char                   buf1[4], buf2[4], buf3[128], buf4[128];
    char                   *argv[6], *envp[1];

    rc = NXT_OK;

    memset(argv, 0, sizeof(argv));
    memset(envp, 0, sizeof(envp));

    name_length = strlen(c->name);

    if (name_length + 3 > sizeof(buf3)) {
	nxt_alert(task, "Name %s is too long", c->name);
	goto fail;
    }

    nxt_memcpy(buf1, "-c", 3);
    nxt_memcpy(buf2, "-o", 3);
    nxt_memcpy(buf3, c->name, name_length);
    nxt_memcpy(buf3 + name_length, ".o", 3);
    nxt_memcpy(buf4, c->name, name_length);
    nxt_memcpy(buf4 + name_length, ".c", 3);

    argv[0] = c->cc;
    argv[1] = buf1;
    argv[2] = buf2;
    argv[3] = buf3;
    argv[4] = buf4;
    argv[5] = NULL;
    envp[0] = NULL;

    nxt_log(task, NXT_LOG_INFO, "C: Running compiler: %s %s %s %s %s", argv[0], argv[1],
    	argv[2], argv[3], argv[4]);

    rc = nxt_app_transient_process_execute(task, c->cc, argv, envp, cc_pid);
    if (rc != NXT_OK) {
        nxt_alert(task, "Failed to execute %s", c->cc);
	goto fail;
    }

    goto out;

    fail:

    rc = NXT_ERROR;

    out:

    return rc;
}

static nxt_int_t
nxt_v_compile_check(nxt_unit_ctx_t *ctx)
{
    nxt_int_t              rc;
    nxt_int_t              cc_rc;
    nxt_c_ctx_t            *cctx;
    nxt_c_app_conf_t       *c;
    nxt_task_t             *task;

    cctx = ctx->data;
    c = cctx->c;
    task = cctx->task;

    /* TODO : Get output from compiler */
    /* TODO : Check if output is newer than source */

    if (cctx->cc_done)
        return NXT_UNIT_OK;

    if (!cctx->cc_pid) {
        rc = nxt_c_compile(task, c, &cctx->cc_pid);
        if (nxt_slow_path(rc != NXT_OK)) {
            nxt_alert(task, "C: Failed to run compiler %s", c->name);
    	    return NXT_UNIT_ERROR;
        }
	return NXT_UNIT_AGAIN;
    }

    switch (nxt_app_transient_process_wait(task, &cc_rc)) {
        case NXT_OK:
	    if (cc_rc != 0) {
	        nxt_alert(task, "C: Compiler failed with return value %i", cc_rc);
	        return NXT_UNIT_ERROR;
	    }
	    nxt_log(task, NXT_LOG_INFO, "C: Compiler finished successfully");
	    break;
	case NXT_AGAIN:
	    nxt_log(task, NXT_LOG_INFO, "C: Compiler not finished yet");
	    return NXT_UNIT_AGAIN;
    };

    cctx->cc_pid = 0;
    cctx->cc_done = 1;

    return NXT_UNIT_OK;
}

static nxt_int_t
nxt_c_start(nxt_task_t *task, nxt_process_data_t *data)
{
    int                    rc;
    nxt_unit_ctx_t         *unit_ctx;
    nxt_unit_init_t        c_init;
    nxt_c_ctx_t            cctx;
    nxt_c_app_conf_t       *c;
    nxt_common_app_conf_t  *common_conf;

    common_conf = data->app;
    c = &common_conf->u.c;

    memset(&cctx, 0, sizeof(cctx));

    nxt_unit_default_init(task, &c_init, common_conf);

    c_init.callbacks.request_handler = nxt_c_request_handler;
    c_init.callbacks.ready_handler = nxt_c_ready_handler;
    c_init.data = c;
    c_init.ctx_data = &cctx;

    nxt_log(task, NXT_LOG_INFO, "C: Starting in %s name is %s working dir is %s cc is %s",
    	__func__, c->name, common_conf->working_directory, c->cc);

    unit_ctx = nxt_unit_init(&c_init);
    if (nxt_slow_path(unit_ctx == NULL)) {
	goto fail;
    }

    cctx.ctx = unit_ctx;
    cctx.task = task;
    cctx.c = c;

    rc = nxt_v_compile_check(unit_ctx);
    nxt_assert(rc == NXT_UNIT_AGAIN);

    rc = nxt_unit_run(unit_ctx);

    nxt_unit_done(unit_ctx);

    nxt_c_ctx_cleanup(&cctx);

    exit(rc);

    return NXT_OK;


    fail:

    nxt_c_ctx_cleanup(&cctx);

    return NXT_ERROR;
}

static void
nxt_c_request_handler(nxt_unit_request_info_t *req)
{
    nxt_int_t           rc;

    rc = nxt_v_compile_check(req->ctx);
    if (rc != NXT_UNIT_OK) {
	nxt_unit_request_done(req, NXT_UNIT_ERROR);
	return;
    }

    nxt_unit_request_done(req, NXT_UNIT_OK);
}

static int
nxt_c_ready_handler(nxt_unit_ctx_t *ctx) {
    return NXT_UNIT_OK;
}
