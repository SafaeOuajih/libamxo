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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include <dirent.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>

#include <amxd/amxd_common.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"
#include "amxo_parser.tab.h"

static void amxo_parser_push(amxo_parser_t* parent,
                             amxo_parser_t* child) {
    child->resolvers = parent->resolvers;
    child->hooks = parent->hooks;
    child->include_stack = parent->include_stack;
    child->entry_points = parent->entry_points;
    child->post_includes = parent->post_includes;
    child->parent = parent;
    amxc_var_copy(&child->config, &parent->config);
}

static void amxo_parser_pop(amxo_parser_t* parent,
                            amxo_parser_t* child) {
    parent->resolvers = child->resolvers;
    parent->entry_points = child->entry_points;
    parent->post_includes = child->post_includes;
    amxc_llist_move(&parent->event_list, &child->event_list);
    amxc_llist_move(&parent->function_names, &child->function_names);
    child->include_stack = NULL;
    child->hooks = NULL;
    child->resolvers = NULL;
    child->entry_points = NULL;
    child->post_includes = NULL;
    child->parent = NULL;
    amxc_llist_for_each(it, (&child->global_config)) {
        amxc_string_t* str_name = amxc_string_from_llist_it(it);
        const char* name = amxc_string_get(str_name, 0);
        amxc_var_t* option = amxc_var_get_path(&child->config,
                                               name,
                                               AMXC_VAR_FLAG_DEFAULT);
        if(!amxc_var_is_null(option)) {
            amxc_var_set_path(&parent->config, name, option, AMXC_VAR_FLAG_UPDATE | AMXC_VAR_FLAG_AUTO_ADD);
            amxc_llist_append(&parent->global_config, &str_name->it);
        }
    }
}

static amxc_var_t* amxo_parser_can_include(amxo_parser_t* pctx,
                                           const char* full_path) {
    amxc_var_t* incstack = NULL;
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

static bool amxo_parser_exists(const char* incdir,
                               const char* file_path,
                               char** full_path) {
    bool retval = false;
    amxc_string_t concat_path;
    amxc_string_init(&concat_path, 0);

    if((incdir != NULL) && (*incdir != 0)) {
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

static int amxo_parser_check(amxo_parser_t* pctx,
                             const char* file_path,
                             amxc_var_t** incstack,
                             char** full_path) {
    int retval = -1;
    amxc_var_t* config = amxo_parser_get_config(pctx, "include-dirs");
    const amxc_llist_t* incdirs = amxc_var_constcast(amxc_llist_t, config);
    amxc_string_t res_file_path;
    struct stat statbuf;

    amxc_string_init(&res_file_path, 0);

    if(amxc_string_set_resolved(&res_file_path, file_path, &pctx->config) > 0) {
        file_path = amxc_string_get(&res_file_path, 0);
    }

    if(!amxo_parser_find(pctx, incdirs, file_path, full_path)) {
        retval = 2;
        pctx->status = amxd_status_file_not_found;
        amxo_parser_msg(pctx, "Include file not found \"%s\"", file_path);
        goto exit;
    }

    if(stat(*full_path, &statbuf) != 0) {
        retval = 2;
        pctx->status = amxd_status_file_not_found;
        amxo_parser_msg(pctx, "Include file not found \"%s\"", file_path);
        goto exit;
    }

    *incstack = amxo_parser_can_include(pctx, *full_path);
    if(*incstack == NULL) {
        pctx->status = amxd_status_recursion;
        amxo_parser_msg(pctx, "Recursive include detected \"%s\"", file_path);
        goto exit;
    }

    retval = 0;

exit:
    amxc_string_clean(&res_file_path);
    return retval;
}

static int amxo_parser_include_file(amxo_parser_t* pctx,
                                    const char* full_path) {
    int retval = -1;
    amxo_parser_t parser;

    amxo_parser_child_init(&parser);
    amxo_hooks_start_include(pctx, full_path);
    amxo_parser_push(pctx, &parser);
    retval = amxo_parser_parse_file_impl(&parser, full_path, pctx->object);
    amxo_parser_pop(pctx, &parser);
    amxo_hooks_end_include(pctx, full_path);
    amxo_parser_clean(&parser);

    if(retval != 0) {
        retval = 3;
        if(pctx->status == amxd_status_ok) {
            pctx->status = amxd_status_unknown_error;
        }
        amxo_parser_msg(pctx, "Error found in %s", full_path);
    }

    return retval;
}

static int amxo_parser_include_dir(amxo_parser_t* pctx,
                                   const char* full_path) {
    int retval = -1;
    int count = 0;
    struct dirent** namelist;
    amxc_string_t file;
    int n;

    amxc_string_init(&file, 0);

    n = scandir(full_path, &namelist, NULL, alphasort);
    if(n == -1) {
        pctx->status = amxd_status_unknown_error;
        goto exit;
    }

    retval = 0;
    for(int i = 0; i < n; i++) {
        int len = 0;
        if(retval != 0) {
            free(namelist[i]);
            continue;
        }
        if(namelist[i]->d_type == DT_DIR) {
            free(namelist[i]);
            continue;
        }
        len = strlen(namelist[i]->d_name);
        if(strncmp(namelist[i]->d_name + len - 4, ".odl", 4) != 0) {
            free(namelist[i]);
            continue;
        }
        amxc_string_setf(&file, "%s/%s", full_path, namelist[i]->d_name);
        retval = amxo_parser_include_file(pctx, amxc_string_get(&file, 0));
        count++;
        free(namelist[i]);
    }

    if(count == 0) {
        pctx->status = amxd_status_ok;
        retval = 4;
    }
    free(namelist);

exit:
    amxc_string_clean(&file);
    return retval;
}

bool amxo_parser_find(amxo_parser_t* parser,
                      const amxc_llist_t* dirs,
                      const char* file_path,
                      char** full_path) {
    bool retval = false;

    if(file_path[0] != '/') {
        amxc_string_t res_path;
        amxc_string_init(&res_path, 0);
        amxc_llist_for_each(it, dirs) {
            amxc_var_t* var_dir = amxc_var_from_llist_it(it);
            const char* dir = amxc_var_constcast(cstring_t, var_dir);
            if(amxc_string_set_resolved(&res_path, dir, &parser->config) > 0) {
                dir = amxc_string_get(&res_path, 0);
            }

            if(amxo_parser_exists(dir, file_path, full_path)) {
                break;
            }
            amxc_string_reset(&res_path);
        }
        amxc_string_clean(&res_path);
        when_null(*full_path, exit);
    } else {
        if(!amxo_parser_exists(NULL, file_path, full_path)) {
            goto exit;
        }
    }
    retval = true;

exit:
    return retval;
}

int amxo_parser_add_post_include(amxo_parser_t* pctx, const char* file_path) {
    int retval = -1;
    char* full_path = NULL;
    amxc_var_t* incstack = NULL;

    retval = amxo_parser_check(pctx, file_path, &incstack, &full_path);
    when_failed(retval, exit);

    if(pctx->post_includes == NULL) {
        amxc_var_new(&pctx->post_includes);
        amxc_var_set_type(pctx->post_includes, AMXC_VAR_ID_LIST);
    }

    amxc_var_add(cstring_t, pctx->post_includes, full_path);

exit:
    amxc_var_delete(&incstack);
    free(full_path);
    return retval;
}

int amxo_parser_include(amxo_parser_t* pctx, const char* file_path) {
    int retval = -1;
    char* full_path = NULL;
    amxc_var_t* incstack = NULL;
    struct stat statbuf;

    retval = amxo_parser_check(pctx, file_path, &incstack, &full_path);
    when_failed(retval, exit);

    stat(full_path, &statbuf);

    if(S_ISDIR(statbuf.st_mode)) {
        retval = amxo_parser_include_dir(pctx, full_path);
    } else {
        retval = amxo_parser_include_file(pctx, full_path);
    }

exit:
    amxc_var_delete(&incstack);
    free(full_path);
    return retval;
}
