/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * ubs-virt-enpu is licensed under Mulan PSL v2.
 */
#include "enpu_dcmi.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>

static pthread_once_t g_dcmi_once = PTHREAD_ONCE_INIT;
static enpu_dcmi_api_t g_dcmi_api;
static void *g_dcmi_library;
static char g_dcmi_error[512];

static void load_dcmi(void)
{
    /* Retain the handle for the lifetime of the resolved function pointers. */
    g_dcmi_library = dlopen("libdcmi.so", RTLD_NOW | RTLD_LOCAL);
    if (g_dcmi_library == NULL) {
        const char *error = dlerror();
        (void)snprintf(g_dcmi_error, sizeof(g_dcmi_error), "%s", error != NULL ? error : "unknown error");
        return;
    }

#define ENPU_DCMI_BIND(name, args) g_dcmi_api.name = (int(*) args)dlsym(g_dcmi_library, #name);
    ENPU_DCMI_SYMBOLS(ENPU_DCMI_BIND)
#undef ENPU_DCMI_BIND
}

const enpu_dcmi_api_t *enpu_dcmi_get_api(const char **error)
{
    (void)pthread_once(&g_dcmi_once, load_dcmi);
    if (error != NULL) {
        *error = g_dcmi_library == NULL ? g_dcmi_error : NULL;
    }
    return g_dcmi_library != NULL ? &g_dcmi_api : NULL;
}
