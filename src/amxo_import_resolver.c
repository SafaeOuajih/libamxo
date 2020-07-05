/****************************************************************************
**
** Copyright (c) 2020 SoftAtHome
**
** Redistribution and use in source and binary forms, with or
** without modification, are permitted provided that the following
** conditions are met:
**
** 1. Redistributions of source code must retain the above copyright
** notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above
** copyright notice, this list of conditions and the following
** disclaimer in the documentation and/or other materials provided
** with the distribution.
**
** Subject to the terms and conditions of this license, each
** copyright holder and contributor hereby grants to those receiving
** rights under this license a perpetual, worldwide, non-exclusive,
** no-charge, royalty-free, irrevocable (except for failure to
** satisfy the conditions of this license) patent license to make,
** have made, use, offer to sell, sell, import, and otherwise
** transfer this software, where such license applies only to those
** patent claims, already acquired or hereafter acquired, licensable
** by such copyright holder or contributor that are necessarily
** infringed by:
**
** (a) their Contribution(s) (the licensed copyrights of copyright
** holders and non-copyrightable additions of contributors, in
** source or binary form) alone; or
**
** (b) combination of their Contribution(s) with the work of
** authorship to which such Contribution(s) was added by such
** copyright holder or contributor, if, at the time the Contribution
** is added, such addition causes such combination to be necessarily
** infringed. The patent license shall not apply to any other
** combinations which include the Contribution.
**
** Except as expressly stated above, no rights or licenses from any
** copyright holder or contributor is granted under this license,
** whether expressly, by implication, estoppel or otherwise.
**
** DISCLAIMER
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
** CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
** INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
** USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
** AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
** ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#define _GNU_SOURCE
#include <sys/resource.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <dlfcn.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxo/amxo.h>

#include "amxo_assert.h"
#include "amxo_parser_priv.h"

typedef struct _amxo_import_lib {
    amxc_htable_it_t hit;
    uint32_t references;
    void *handle;
} amxo_import_lib_t;

typedef union _fn_caster {
    void *fn;
    amxo_fn_ptr_t amxo_fn;
} fn_caster_t;

static amxc_htable_t import_libs;

static void amxo_import_lib_free(AMXO_UNUSED const char *key,
                                 amxc_htable_it_t *it) {
    amxo_import_lib_t *import =
        amxc_htable_it_get_data(it, amxo_import_lib_t, hit);
    dlerror();
    dlclose(import->handle);

    free(import);
}

static void amxo_resolver_import_defaults(amxo_parser_t *parser,
                                          AMXO_UNUSED void *priv) {
    amxc_var_t *config = amxo_parser_get_config(parser, "import-dirs");

    amxc_var_set_type(config, AMXC_VAR_ID_LIST);
    amxc_var_add(cstring_t, config, ".");

    config = amxo_parser_get_config(parser, "import-dbg");
    amxc_var_set_type(config, AMXC_VAR_ID_BOOL);
    amxc_var_set(bool, config, false);

    config = amxo_parser_get_config(parser, "import-pcb-compat");
    amxc_var_set_type(config, AMXC_VAR_ID_BOOL);
    amxc_var_set(bool, config, false);

    return;
}

static char *amxo_resolver_import_get_symbol(amxo_parser_t *parser,
                                             const char *fn_name) {
    amxc_string_t symbol;
    amxc_string_init(&symbol, 0);

    if(parser->param != NULL) {
        const char *param_name = amxd_param_get_name(parser->param);
        amxc_string_setf(&symbol, "_%s_%s", param_name, fn_name);
    } else {
        const char *obj_name = amxd_object_get_name(parser->object,
                                                    AMXD_OBJECT_NAMED);
        if(obj_name != NULL) {
            amxc_string_setf(&symbol, "_%s_%s", obj_name, fn_name);
        } else {
            amxc_string_setf(&symbol, "_%s", fn_name);
        }
    }

    return amxc_string_take_buffer(&symbol);
}

static amxo_fn_ptr_t amxo_resolver_try(amxo_parser_t *parser,
                                       const char *fn_name,
                                       const char *symbol,
                                       const char *lib_name,
                                       amxo_import_lib_t *lib) {
    char *dl_error = NULL;
    fn_caster_t helper;
    bool dbg = amxc_var_constcast(bool,
                                  amxo_parser_get_config(parser, "import-dbg"));

    helper.fn = NULL;
    dlerror();
    helper.fn = dlsym(lib->handle, symbol);
    dl_error = dlerror();
    if(dl_error == NULL) {
        if(dbg) {
            fprintf(stderr,
                    "[IMPORT-DBG] - symbol %s resolved (for %s) from %s\n",
                    symbol,
                    fn_name,
                    lib_name);
        }
        lib->references++;
    } else {
        if(dbg) {
            fprintf(stderr,
                    "[IMPORT-DBG] - resolving symbol %s (for %s) from %s failed - [%s]\n",
                    symbol,
                    fn_name,
                    lib_name,
                    dl_error);
        }
    }
    return helper.amxo_fn;
}

static amxo_fn_ptr_t amxo_resolver_pcb_compat(amxo_parser_t *parser,
                                              const char *fn_name,
                                              const char *lib_name,
                                              amxo_import_lib_t *lib) {
    amxo_fn_ptr_t fn = NULL;
    amxc_string_t symbol;
    const char *obj_name = amxd_object_get_name(parser->object, AMXD_OBJECT_NAMED);
    amxc_string_init(&symbol, 0);

    // 1 __objectname_function
    amxc_string_setf(&symbol, "__%s_%s", obj_name, fn_name);
    fn = amxo_resolver_try(parser,
                           fn_name, amxc_string_get(&symbol, 0),
                           lib_name,
                           lib);
    when_not_null(fn, exit);

    // 2 objectname_function
    amxc_string_setf(&symbol, "%s_%s", obj_name, fn_name);
    fn = amxo_resolver_try(parser,
                           fn_name,
                           amxc_string_get(&symbol, 0),
                           lib_name,
                           lib);
    when_not_null(fn, exit);

    // 3 __function
    amxc_string_setf(&symbol, "__%s", fn_name);
    fn = amxo_resolver_try(parser,
                           fn_name,
                           amxc_string_get(&symbol, 0),
                           lib_name,
                           lib);
    when_not_null(fn, exit);

    // 4 function
    amxc_string_setf(&symbol, "%s", fn_name);
    fn = amxo_resolver_try(parser,
                           fn_name,
                           amxc_string_get(&symbol, 0),
                           lib_name,
                           lib);

exit:
    amxc_string_clean(&symbol);
    return fn;
}

static void amxo_resolver_import_parse_data(const char *data,
                                            char **lib,
                                            char **symbol) {
    amxc_string_t str_data;
    amxc_llist_t parts;
    size_t length = 0;
    amxc_llist_it_t *it = NULL;

    amxc_llist_init(&parts);
    amxc_string_init(&str_data, 0);
    amxc_string_push_buffer(&str_data, (char *) data, strlen(data) + 1);
    amxc_string_split_to_llist(&str_data, &parts, ':');
    length = amxc_llist_size(&parts);

    when_true(length > 2, exit);

    it = amxc_llist_get_first(&parts);
    amxc_string_trim(amxc_string_from_llist_it(it), NULL);
    *lib = amxc_string_take_buffer(amxc_string_from_llist_it(it));

    if(length > 1) {
        it = amxc_llist_get_last(&parts);
        amxc_string_trim(amxc_string_from_llist_it(it), NULL);
        *symbol = amxc_string_take_buffer(amxc_string_from_llist_it(it));
    }

exit:
    amxc_llist_clean(&parts, amxc_string_list_it_free);
    return;
}

static amxo_fn_ptr_t amxo_resolver_import_data(amxo_parser_t *parser,
                                               amxc_htable_t *import_data,
                                               const char *fn_name,
                                               const char *data) {
    amxo_fn_ptr_t fn = NULL;
    char *lib = NULL;
    char *symbol = NULL;
    amxc_htable_it_t *it = NULL;
    amxo_import_lib_t *import = NULL;
    bool pcb = amxc_var_constcast(bool,
                                  amxo_parser_get_config(parser, "import-pcb-compat"));

    amxo_resolver_import_parse_data(data, &lib, &symbol);
    it = amxc_htable_get(import_data, lib);
    import = amxc_htable_it_get_data(it, amxo_import_lib_t, hit);
    if(import == NULL) {
        amxo_parser_msg(parser, "No import library found with name \"%s\"", lib);
        parser->status = amxd_status_invalid_value;
        goto exit;
    }
    if(symbol != NULL) {
        fn = amxo_resolver_try(parser, fn_name, symbol, lib, import);
    } else {
        if(pcb) {
            fn = amxo_resolver_pcb_compat(parser, fn_name, lib, import);
        } else {
            symbol = amxo_resolver_import_get_symbol(parser, fn_name);
            fn = amxo_resolver_try(parser, fn_name, symbol, lib, import);
        }
    }

exit:
    free(lib);
    free(symbol);

    return fn;
}

static amxo_fn_ptr_t amxo_resolver_import(amxo_parser_t *parser,
                                          const char *fn_name,
                                          const char *data,
                                          AMXO_UNUSED void *priv) {
    amxc_htable_t *import_data = amxo_parser_get_resolver_data(parser, "import");
    bool pcb = amxc_var_constcast(bool,
                                  amxo_parser_get_config(parser, "import-pcb-compat"));
    amxo_fn_ptr_t fn = NULL;

    if((data != NULL) && (data[0] != 0)) {
        fn = amxo_resolver_import_data(parser, import_data, fn_name, data);
    } else {
        amxc_htable_for_each(it, import_data) {
            const char *lib_name = amxc_htable_it_get_key(it);
            amxo_import_lib_t *import =
                amxc_htable_it_get_data(it, amxo_import_lib_t, hit);
            if(pcb) {
                fn = amxo_resolver_pcb_compat(parser, fn_name, lib_name, import);
            } else {
                char *symbol = amxo_resolver_import_get_symbol(parser, fn_name);
                fn = amxo_resolver_try(parser, fn_name, symbol, lib_name, import);
                free(symbol);
            }
            if(fn != NULL) {
                break;
            }
        }
    }

    return fn;
}

static void amxo_resolver_import_clean(amxo_parser_t *parser,
                                       AMXO_UNUSED void *priv) {
    amxc_htable_t *import_data = NULL;
    amxc_htable_it_t *it = NULL;
    import_data = amxo_parser_get_resolver_data(parser, "import");

    it = amxc_htable_take_first(import_data);
    while(it) {
        amxo_import_lib_t *import =
            amxc_htable_it_get_data(it, amxo_import_lib_t, hit);
        const char *key = amxc_htable_it_get_key(it);
        if(import->references != 0) {
            amxc_htable_insert(&import_libs, key, it);
        } else {
            amxc_htable_it_clean(it, amxo_import_lib_free);
        }
        it = amxc_htable_take_first(import_data);
    }
    amxc_htable_clean(import_data, amxo_import_lib_free);
}

static bool amxo_resolver_import_alias_exists(amxc_htable_t *import_data,
                                              const char *alias) {
    bool retval = true;
    amxc_htable_it_t *it = NULL;

    it = amxc_htable_get(&import_libs, alias);
    if(it != NULL) {
        amxc_htable_insert(import_data, alias, it);
        goto exit;
    }
    it = amxc_htable_get(import_data, alias);
    if(it != NULL) {
        goto exit;
    }

    retval = false;

exit:
    return retval;
}

static void *amxo_resolver_import_lib(amxo_parser_t *parser,
                                      const char *so_name,
                                      const char *full_path,
                                      int flags) {
    void *handle = NULL;
    bool dbg = amxc_var_constcast(bool,
                                  amxo_parser_get_config(parser, "import-dbg"));

    if((flags & (RTLD_LAZY | RTLD_NOW)) == 0) {
        flags |= RTLD_LAZY;
    }
    dlerror();
    if(dbg) {
        fprintf(stderr, "[IMPORT-DBG] - dlopen - %s\n", so_name);
    }
    handle = dlopen(full_path, flags);
    if(handle == NULL) {
        char *error = dlerror();
        if(dbg) {
            fprintf(stderr, "[IMPORT-DBG] - failed to load %s - %s\n", so_name, error);
        }
        amxc_string_setf(&parser->msg, "Failed to load lib %s", error);
    }

    return handle;
}

int amxo_resolver_import_open(amxo_parser_t *parser,
                              const char *so_name,
                              const char *alias,
                              int flags) {
    int retval = -1;
    bool dbg = amxc_var_constcast(bool,
                                  amxo_parser_get_config(parser, "import-dbg"));
    void *handle = NULL;
    amxc_var_t *config = amxo_parser_get_config(parser, "import-dirs");
    const amxc_llist_t *impdirs = amxc_var_constcast(amxc_llist_t, config);
    amxo_import_lib_t *import_lib = NULL;
    amxc_htable_t *import_data = amxo_parser_claim_resolver_data(parser, "import");
    char *full_path = NULL;

    when_null(parser, exit);
    when_str_empty(so_name, exit);
    when_true(alias != NULL && alias[0] == 0, exit);

    if(amxo_resolver_import_alias_exists(import_data, alias)) {
        retval = 0;
        goto exit;
    }

    if(!amxo_parser_find_file(impdirs, so_name, &full_path)) {
        if(dbg) {
            fprintf(stderr, "[IMPORT-DBG] - file not found %s\n", so_name);
        }
        parser->status = amxd_status_unknown_error;
        amxo_parser_msg(parser, "Import file not found !!! \"%s\"", so_name);
        goto exit;
    }

    handle = amxo_resolver_import_lib(parser, so_name, full_path, flags);

    import_lib = calloc(1, sizeof(amxo_import_lib_t));
    when_null(import_lib, exit);

    import_lib->handle = handle;
    amxc_htable_insert(import_data, alias, &import_lib->hit);
    retval = 0;

exit:
    free(full_path);
    if((retval != 0) && (handle != NULL)) {
        dlclose(handle);
    }
    return retval;
}

void amxo_resolver_import_close_all(void) {
    amxc_htable_clean(&import_libs, amxo_import_lib_free);
    amxc_htable_init(&import_libs, 10);
}

static amxo_resolver_t import = {
    .get = amxo_resolver_import_defaults,
    .resolve = amxo_resolver_import,
    .clean = amxo_resolver_import_clean
};

AMXO_CONSTRUCTOR(110) static void amxo_import_init(void) {
    amxc_htable_init(&import_libs, 10);
    amxo_register_resolver("import", &import);
}

AMXO_DESTRUCTOR(110) static void amxo_import_cleanup(void) {
    amxc_htable_clean(&import_libs, NULL);
    amxo_unregister_resolver("import");
}
