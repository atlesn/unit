
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
} nxt_c_ctx_t;


static uint32_t  nxt_c_compat[] = {
    NXT_VERNUM, NXT_DEBUG,
};

static nxt_int_t nxt_c_start(nxt_task_t *task,
    nxt_process_data_t *data);
static void nxt_c_psgi_request_handler(nxt_unit_request_info_t *req);
static int nxt_c_psgi_ready_handler(nxt_unit_ctx_t *ctx);

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
/*
nxt_perl_psgi_ready_handler(nxt_unit_ctx_t *ctx)
{
    int                  res;
    uint32_t             i;
    nxt_perl_app_conf_t  *c;
    nxt_perl_psgi_ctx_t  *pctx;

    c = ctx->unit->data;
    nxt_perl_psgi_ctx_t  *pctx;

    c = ctx->unit->data;
*/
static nxt_int_t
nxt_c_compile(nxt_unit_ctx_t *ctx)
{
    int                    rc;
    nxt_c_app_conf_t       *c;
    nxt_c_ctx_t            *cctx;
    char                   *in_file, *out_file;
    size_t                 name_length;
    const char             *argv[5], *envp[1];
//    pid_t                  cc_pid;
  //  nxt_monotonic_time_t   start_time, end_time;

    rc = NXT_OK;

    memset(argv, 0, sizeof(argv));
    memset(envp, 0, sizeof(envp));

    c = ctx->unit->data;
    cctx = ctx->data;

    (void)(cctx);

    in_file = NULL;
    out_file = NULL;
    name_length = strlen(c->name);

    in_file = nxt_unit_malloc(ctx, name_length + 3);
    if (nxt_slow_path(in_file == NULL)) {
        nxt_unit_alert(ctx, "Failed to allocate memory for name in %s", __func__);
	goto fail;
    }

    out_file = nxt_unit_malloc(ctx, name_length + 3);
    if (nxt_slow_path(out_file == NULL)) {
	nxt_unit_alert(ctx, "Failed to allocate memory for name in %s", __func__);
	goto fail;
    }

    nxt_memcpy(in_file, c->name, name_length);
    nxt_memcpy(in_file + name_length, ".c", 3);
    nxt_memcpy(out_file, c->name, name_length);
    nxt_memcpy(out_file + name_length, ".o", 3);

    argv[0] = c->cc;
    argv[1] = "-c";
    argv[2] = "-o";
    argv[3] = out_file;
    argv[4] = in_file;
    envp[0] = NULL;

    nxt_unit_debug(ctx, "Running compiler: %s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);

/*
    start_time = {0};
    end_time = start_time;
    end_time.nsec += 2000000000; // 2 seconds

    cc_pid = nxt_process_execute(ctx->task, c->cc, argv, envp);

    while (start_time.monotonic < end_time.monotonic) {
	nxt_unit_debug(ctx, "Waiting for cc to finish");
	usleep(100000);	// 100 ms
	nxt_monotonic_time(&start_time);
    }
*/
    goto out;

    fail:

    rc = NXT_ERROR;


    out:

    if (in_file != NULL)
	nxt_unit_free(ctx, in_file);

    if (out_file != NULL)
        nxt_unit_free(ctx, out_file);


    return rc;
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

    c_init.callbacks.request_handler = nxt_c_psgi_request_handler;
    c_init.callbacks.ready_handler = nxt_c_psgi_ready_handler;
    c_init.data = c;
    c_init.ctx_data = &cctx;

    unit_ctx = nxt_unit_init(&c_init);
    if (nxt_slow_path(unit_ctx == NULL)) {
	goto fail;
    }

    cctx.ctx = unit_ctx;

    printf("In %s name is %s working dir is %s cc is %s\n", __func__, c->name, common_conf->working_directory, c->cc);

    rc = nxt_c_compile(unit_ctx);
    if (nxt_slow_path(rc != NXT_OK)) {
        nxt_unit_alert(unit_ctx, "Failed to compile %s", c->name);
	goto fail;
    }

    for (int i = 0; i < 5; i++) {
        printf("loop %i\n", i);
	usleep(1000000);
    }

    rc = 0;

    nxt_unit_done(unit_ctx);

    nxt_c_ctx_cleanup(&cctx);

    exit(rc);

    return NXT_OK;


    fail:

    nxt_c_ctx_cleanup(&cctx);

    return NXT_ERROR;
}

static void
nxt_c_psgi_request_handler(nxt_unit_request_info_t *req)
{
    (void)(req);
    printf("%s not implemented\n", __func__);
    nxt_assert(0);
}
static int
nxt_c_psgi_ready_handler(nxt_unit_ctx_t *ctx) {
    (void)(ctx);
    printf("%s not implemented\n", __func__);
    nxt_assert(0);
    return 0;
}
