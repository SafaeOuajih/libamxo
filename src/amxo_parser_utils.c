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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_common.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"
#include "amxo_assert.h"
#include "amxo_parser.tab.h"

static char *amxo_parser_get_resolver_name(const char *data) {
    amxc_string_t full_data;
    size_t length = strlen(data);
    amxc_llist_t parts;
    amxc_llist_it_t *it = NULL;
    char *name = NULL;

    amxc_llist_init(&parts);
    amxc_string_init(&full_data, length + 1);
    amxc_string_set_at(&full_data, 0, data, length, amxc_string_overwrite);
    amxc_string_split_to_llist(&full_data, &parts, ':');
    it = amxc_llist_take_first(&parts);
    amxc_llist_clean(&parts, amxc_string_list_it_free);
    amxc_string_clean(&full_data);

    amxc_string_trim(amxc_string_from_llist_it(it), NULL);
    name = amxc_string_take_buffer(amxc_string_from_llist_it(it));
    amxc_llist_it_clean(it, amxc_string_list_it_free);

    return name;
}

static void amxc_parser_push(amxo_parser_t *parent,
                             amxo_parser_t *child) {
    child->resolvers = parent->resolvers;
    child->hooks = parent->hooks;
    child->include_stack = parent->include_stack;
    child->entry_points = parent->entry_points;
    amxc_var_copy(&child->config, &parent->config);
}

static void amxc_parser_pop(amxo_parser_t *parent,
                            amxo_parser_t *child) {
    parent->resolvers = child->resolvers;
    parent->entry_points = child->entry_points;
    child->resolvers = NULL;
    child->include_stack = NULL;
    child->hooks = NULL;
    child->entry_points = NULL;
    amxc_llist_for_each(it, (&child->global_config)) {
        amxc_string_t *str_name = amxc_string_from_llist_it(it);
        const char *name = amxc_string_get(str_name, 0);
        amxc_var_t *option = amxc_var_get_path(&child->config,
                                               name,
                                               AMXC_VAR_FLAG_DEFAULT);
        amxc_var_set_key(&parent->config, name, option, AMXC_VAR_FLAG_UPDATE);
        amxc_llist_append(&parent->global_config, &str_name->it);
    }
}

static amxc_var_t *amxo_parser_can_include(amxo_parser_t *pctx,
                                           const char *full_path) {
    amxc_var_t *incstack = NULL;
    if(amxc_var_get_key(pctx->include_stack, full_path, AMXC_VAR_FLAG_DEFAULT) != NULL) {
        goto exit;
    }
    if(pctx->include_stack == NULL) {
        amxc_var_new(&pctx->include_stack);
        amxc_var_set_type(pctx->include_stack, AMXC_VAR_ID_HTABLE);
    }
    incstack = amxc_var_add_key(bool, pctx->include_stack, full_path, true);

exit:
    return incstack;
}

bool amxo_parser_file_exists(amxc_var_t *dir,
                             const char *file_path,
                             char **full_path) {
    const char *incdir = amxc_var_constcast(cstring_t, dir);
    bool retval = false;
    amxc_string_t concat_path;
    amxc_string_init(&concat_path, 0);

    if(dir != NULL) {
        amxc_string_setf(&concat_path, "%s/%s", incdir, file_path);
    } else {
        amxc_string_setf(&concat_path, "%s", file_path);
    }
    *full_path = realpath(amxc_string_get(&concat_path, 0), NULL);
    if(*full_path != NULL) {
        retval = true;
    }
    amxc_string_clean(&concat_path);

    return retval;
}

bool amxo_parser_find_file(const amxc_llist_t *dirs,
                           const char *file_path,
                           char **full_path) {
    bool retval = false;
    if(file_path[0] != '/') {
        amxc_llist_for_each(it, dirs) {
            if(amxo_parser_file_exists(amxc_var_from_llist_it(it),
                                       file_path,
                                       full_path)) {
                break;
            }
        }
        when_null(*full_path, exit);
    } else {
        if(!amxo_parser_file_exists(NULL, file_path, full_path)) {
            goto exit;
        }
    }
    retval = true;

exit:
    return retval;
}

void amxo_parser_msg(amxo_parser_t *parser, const char *format, ...) {
    va_list args;
    va_start(args, format);
    amxc_string_vsetf(&parser->msg, format, args);
    va_end(args);
}

int amxo_parser_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return 0;
}

int amxo_parser_set_config_internal(amxo_parser_t *parser,
                                    const char *name,
                                    amxc_var_t *value) {
    return amxc_var_set_key(&parser->config,
                            name,
                            value,
                            AMXC_VAR_FLAG_UPDATE);
}

int amxo_parser_include(amxo_parser_t *pctx, const char *file_path) {
    int retval = -1;
    amxo_parser_t parser;
    amxc_var_t *config = amxo_parser_get_config(pctx, "include-dirs");
    const amxc_llist_t *incdirs = amxc_var_constcast(amxc_llist_t, config);
    char *full_path = NULL;
    amxc_var_t *incstack = NULL;
    amxc_string_t res_file_path;
    amxc_string_init(&res_file_path, 0);
    if(amxc_string_set_resolved(&res_file_path, file_path, &pctx->config) > 0) {
        file_path = amxc_string_get(&res_file_path, 0);
    }

    if(!amxo_parser_find_file(incdirs, file_path, &full_path)) {
        retval = 2;
        pctx->status = amxd_status_file_not_found;
        amxo_parser_msg(pctx, "Include file not found \"%s\"", file_path);
        goto exit;
    }

    incstack = amxo_parser_can_include(pctx, full_path);
    if(incstack == NULL) {
        pctx->status = amxd_status_recursion;
        amxo_parser_msg(pctx, "Recursive include detected \"%s\"", file_path);
        goto exit;
    }

    amxo_parser_child_init(&parser);
    amxo_hooks_start_include(pctx, full_path);
    amxc_parser_push(pctx, &parser);
    retval = amxo_parser_parse_file_impl(&parser, full_path, pctx->object);
    amxc_parser_pop(pctx, &parser);
    amxo_hooks_end_include(pctx, full_path);
    amxo_parser_clean(&parser);

    if(retval != 0) {
        retval = 3;
        if(pctx->status == amxd_status_ok) {
            pctx->status = amxd_status_unknown_error;
        }
        amxo_parser_msg(pctx, "Error found in %s", file_path);
    }

exit:
    amxc_string_clean(&res_file_path);
    amxc_var_delete(&incstack);
    free(full_path);
    return retval;
}

int amxo_parser_resolve_internal(amxo_parser_t *pctx,
                                 const char *fn_name,
                                 const char *data) {
    int retval = -1;
    char *name = NULL;
    const char *res_data = NULL;

    if((data == NULL) || (data[0] == '\0')) {
        amxo_parser_msg(pctx, "Resolver name must be provide (is empty)");
        pctx->status = amxd_status_invalid_name;
        goto exit;
    }

    name = amxo_parser_get_resolver_name(data);
    if((name == NULL) || (name[0] == '\0')) {
        amxo_parser_msg(pctx, "Resolver name must be provide (is empty)");
        pctx->status = amxd_status_invalid_name;
        goto exit;
    }
    res_data = data + strlen(name);
    while(isspace(res_data[0]) || res_data[0] == ':') {
        res_data++;
    }

    pctx->resolved_fn = NULL;
    retval = amxo_parser_resolve(pctx, name, fn_name, res_data);
    if(retval == -1) {
        pctx->status = amxd_status_invalid_name;
        amxo_parser_msg(pctx, "No function resolver found with name \"%s\"", name);
    } else if(retval == 1) {
        pctx->status = amxd_status_function_not_found;
        amxo_parser_msg(pctx,
                        "No function implemention found for \"%s\" using \"%s\"",
                        fn_name,
                        name);
    }

exit:
    free(name);
    return retval;
}

int amxo_parser_call_entry_point(amxo_parser_t *pctx,
                                 const char *lib_name,
                                 const char *fn_name) {
    int retval = -1;
    amxc_string_t data;
    amxc_string_init(&data, 0);
    amxc_string_setf(&data, "%s", lib_name);

    pctx->resolved_fn = NULL;
    retval = amxo_parser_resolve(pctx, "import", fn_name, amxc_string_get(&data, 0));
    if(retval == 1) {
        amxo_parser_msg(pctx,
                        "No entry point \"%s\" found using \"%s\"",
                        fn_name,
                        "import");
        pctx->status = amxd_status_function_not_found;
    }

    if(pctx->resolved_fn != NULL) {
        amxo_entry_point_t fn = (amxo_entry_point_t) pctx->resolved_fn;
        retval = amxo_parser_add_entry_point(pctx, fn);
    }

    amxc_string_clean(&data);
    return retval;
}

bool amxo_parser_set_data_option(amxo_parser_t *pctx,
                                 const char *key,
                                 amxc_var_t *value) {
    bool retval = false;
    amxc_var_t *data = NULL;
    if(pctx->data == NULL) {
        when_failed(amxc_var_new(&pctx->data), exit);
        when_failed(amxc_var_set_type(pctx->data,
                                      key == NULL ? AMXC_VAR_ID_LIST : AMXC_VAR_ID_HTABLE),
                    exit);
    }

    if(key == NULL) {
        when_true(amxc_var_type_of(pctx->data) != AMXC_VAR_ID_LIST, exit);
        data = amxc_var_add_new(pctx->data);
    } else {
        when_true(amxc_var_type_of(pctx->data) != AMXC_VAR_ID_HTABLE, exit);
        data = amxc_var_add_new_key(pctx->data, key);
    }
    when_null(data, exit);
    amxc_var_copy(data, value);

    retval = true;

exit:
    return retval;
}

int amxo_parser_get_action_id(amxo_parser_t *pctx,
                              const char *action_name) {
    static const char *names[] = {
        "read",
        "write",
        "validate",
        "list",
        "describe",
        "add-inst",
        "del-inst",
        "destroy"
    };
    int action_id = -1;

    for(int i = 0; i <= action_max; i++) {
        if(strcmp(action_name, names[i]) == 0) {
            action_id = i;
            break;
        }
    }

    if(action_id < 0) {
        pctx->status = amxd_status_invalid_action;
        amxo_parser_msg(pctx,
                        "Invalid action name \"%s\"",
                        action_name);
    }

    return action_id;
}

char *amxo_parser_build_import_resolver_data(const char *function,
                                             const char *library) {
    amxc_string_t data_txt;
    char *data = NULL;
    amxc_string_init(&data_txt, 0);

    amxc_string_appendf(&data_txt, "import:%s:%s", library, function);

    data = amxc_string_take_buffer(&data_txt);
    amxc_string_clean(&data_txt);

    return data;
}