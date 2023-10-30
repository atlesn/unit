
/*
 * Copyright (C) Atle Solbakken
 * Copyright (C) NGINX, Inc.
 */

#include <nxt_main.h>
#include <nxt_application.h>
#include <nxt_unit.h>
#include <nxt_time.h>

#include <nxt_c.h>

typedef struct {
    nxt_unit_ctx_t           *ctx;
    nxt_task_t               *task;
    nxt_c_app_conf_t         *c;
    nxt_pid_t                cc_pid;
    nxt_bool_t               cc_done;
    nxt_pid_t                ld_pid;
    nxt_bool_t               ld_done;
    void                     *dl;
    nxt_c_mod_t              *mod;
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
    nxt_conf_value_t       *value;
    nxt_str_t              str;
    uint32_t               i, wpos;
    char                   **argv, *envp[1], **ptr;

    rc = NXT_OK;

    argv = NULL;
    envp[0] = NULL;
    wpos = 0;

    name_length = strlen(c->prefix);

    nxt_assert(c->flags != NULL);

    i = 0;
    while ((value = nxt_conf_get_array_element(c->flags, i++)) != NULL) {
    }

    argv = nxt_zalloc((i + 6) * sizeof(*argv));
    if (argv == NULL) {
	nxt_alert(task, "C: Failed to allocate memory for compiler argv");
        goto fail;
    }

    argv[wpos] = nxt_strdup(c->cc);
    if (argv[wpos++] == NULL) {
        nxt_alert(task, "C: Failed to allocate memory for compiler argument");
        goto fail;
    }

    i = 0;
    while ((value = nxt_conf_get_array_element(c->flags, i++)) != NULL) {
        nxt_conf_get_string(value, &str);
	argv[wpos] = nxt_malloc(str.length + 1);
	if (argv[wpos] == NULL) {
	    nxt_alert(task, "C: Failed to allocate memory for compiler argument %u of length %u",
	    	(nxt_uint_t) i, (nxt_uint_t) str.length);
	    goto fail;
	}
	nxt_cpystrn((u_char *) argv[wpos++], str.start, str.length + 1);
    }

    nxt_assert(i == wpos);

    /*
     * TODO : Might need to set the same compiler arguments as when unit
     *        was compiled, like pie, fPIC, etc.
     */

    if ((argv[wpos++] = nxt_strdup("-shared")) == NULL ||
        (argv[wpos++] = nxt_strdup("-o")) == NULL ||
	(argv[wpos++] = nxt_malloc(name_length + 3)) == NULL ||
	(argv[wpos++] = nxt_malloc(name_length + 3)) == NULL
    ) {
	nxt_alert(task, "C: Failed to allocate memory for compiler argument");
	goto fail;
    }

    nxt_memcpy(argv[wpos - 2], c->prefix, name_length);
    nxt_memcpy(argv[wpos - 2] + name_length, ".o", 3);
    nxt_memcpy(argv[wpos - 1], c->prefix, name_length);
    nxt_memcpy(argv[wpos - 1] + name_length, ".c", 3);

    argv[wpos] = NULL;

    nxt_log(task, NXT_LOG_INFO, "C: Running compiler");
    for (i = 0; i < wpos; i++) {
	nxt_log(task, NXT_LOG_INFO, "C: argv[%ui] = %s", (nxt_uint_t) i, argv[i]);
    }

    rc = nxt_app_transient_process_execute(task, c->cc, argv, envp, cc_pid);
    if (rc != NXT_OK) {
        nxt_alert(task, "Failed to execute %s", c->cc);
	goto fail;
    }

    goto out;

    fail:

    rc = NXT_ERROR;

    out:

    if (argv != NULL) {
        for (ptr = argv; *ptr != NULL; ptr++) {
	    nxt_free(*ptr);
	}
	nxt_free(argv);
    }

    return rc;
}

static nxt_int_t
nxt_c_load_check(nxt_unit_ctx_t *ctx)
{
//    nxt_int_t              rc;
    nxt_c_ctx_t            *cctx;
    nxt_c_app_conf_t       *c;
    nxt_task_t             *task;
    size_t                 name_length;
    char                   buf[128];
    void                   *dl;
    nxt_c_mod_t            *mod;
    char                   *err;

    cctx = ctx->data;
    c = cctx->c;
    task = cctx->task;

    if (cctx->dl != NULL)
	return NXT_UNIT_OK;

    name_length = strlen(c->prefix);
    if (name_length + 3 > sizeof(buf)) {
        nxt_alert(task, "Name %s is too long", c->prefix);
	return NXT_UNIT_ERROR;
    }
    nxt_memcpy(buf, c->prefix, name_length);
    nxt_memcpy(buf + name_length, ".o", 3);

    dl = dlopen(buf, RTLD_GLOBAL | RTLD_LAZY);
    if (nxt_slow_path(dl == NULL)) {
	nxt_alert(task, "dlopen(\"%s\") failed: \"%s\"",
		  buf, dlerror());
	return NXT_UNIT_ERROR;
    }

    nxt_log(task, NXT_LOG_INFO, "C: Module %s loaded, cwd is %s, looking for symbols",
        buf, getcwd(NULL, 0));

    mod = dlsym(dl, "nxt_c_module");
    if (nxt_slow_path(mod == NULL)) {
        err = dlerror();
	nxt_alert(task, "dlsym(\"%s\", \"nxt_c_module\") failed: \"%s\"",
		  buf, err != NULL ? err : "(null)");
        dlclose(dl);
	return NXT_UNIT_ERROR;
    }

    cctx->dl = dl;
    cctx->mod = mod;

    return NXT_UNIT_OK;
}

static void
nxt_c_compile_reset(nxt_unit_ctx_t *ctx)
{
    nxt_int_t              cc_rc;
    nxt_c_ctx_t            *cctx;
    nxt_task_t             *task;

    cctx = ctx->data;
    task = cctx->task;

    nxt_log(task, NXT_LOG_INFO, "C: Resetting compiler state");

    if (cctx->cc_pid > 0) {
        if (nxt_app_transient_process_wait(task, &cc_rc) == NXT_AGAIN) {
	    return;
	}
	cctx->cc_pid = 0;
    }

    cctx->cc_done = 0;

    if (cctx->dl) {
        nxt_trace(task, "C: Closing dynamic library");
        dlclose(cctx->dl);
	cctx->dl = NULL;
	cctx->mod = NULL;
    }
}

static nxt_int_t
nxt_c_compile_check(nxt_unit_ctx_t *ctx)
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
            nxt_alert(task, "C: Failed to run compiler %s", c->prefix);
    	    return NXT_UNIT_ERROR;
        }
	return NXT_UNIT_AGAIN;
    }

    switch (nxt_app_transient_process_wait(task, &cc_rc)) {
        case NXT_OK:
	    if (cc_rc != 0) {
	        nxt_alert(task, "C: Compiler failed with return value %i", cc_rc);
	        return NXT_UNIT_AGAIN;
	    }
	    nxt_log(task, NXT_LOG_INFO, "C: Compiler finished successfully");
	    break;
	case NXT_AGAIN:
	    nxt_log(task, NXT_LOG_INFO, "C: Compiler not finished yet");
	    return NXT_UNIT_AGAIN;
	default:
	    nxt_alert(task, "C: Compiler failed");
	    return NXT_UNIT_ERROR;
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
    	__func__, c->prefix, common_conf->working_directory, c->cc);

    unit_ctx = nxt_unit_init(&c_init);
    if (nxt_slow_path(unit_ctx == NULL)) {
	goto fail;
    }

    cctx.ctx = unit_ctx;
    cctx.task = task;
    cctx.c = c;

    rc = nxt_c_compile_check(unit_ctx);
    if (rc != NXT_UNIT_ERROR) {
        rc = nxt_unit_run(unit_ctx);
    }

    nxt_c_compile_reset(unit_ctx);

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
    nxt_c_ctx_t            *cctx;
    nxt_unit_ctx_t         *ctx;

    ctx = req->ctx;
    cctx = ctx->data;

    switch (nxt_c_compile_check(ctx)) {
        case NXT_UNIT_OK:
	    break;
	case NXT_UNIT_ERROR:
	    nxt_c_compile_reset(ctx);
	    /* Fall through */
	default:
	    nxt_unit_request_done(req, NXT_UNIT_ERROR);
	    return;
    };

    switch (nxt_c_load_check(ctx)) {
    	case NXT_UNIT_OK:
	    break;
	default:
	    nxt_c_compile_reset(ctx);
	    nxt_unit_request_done(req, NXT_UNIT_ERROR);
	    return;
    };

    if (cctx->mod->request_handler(req) != NXT_UNIT_OK) {
        nxt_unit_request_done(req, NXT_UNIT_ERROR);
        return;
    };

    nxt_unit_request_done(req, NXT_UNIT_OK);
}

static int
nxt_c_ready_handler(nxt_unit_ctx_t *ctx) {
    return NXT_UNIT_OK;
}
