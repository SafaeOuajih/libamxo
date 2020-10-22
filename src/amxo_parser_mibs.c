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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <dirent.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxp/amxp_expression.h>
#include <amxo/amxo.h>
#include <amxo/amxo_mibs.h>

#include "amxo_assert.h"
#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"

typedef struct _mib_info {
    amxc_htable_it_t hit;
    char* expression;
    char* file;
} mib_info_t;

static void amxo_parser_add_mib_info(amxc_htable_t* mibs,
                                     const char* name,
                                     const char* expression,
                                     const char* file) {
    mib_info_t* info = NULL;
    when_true(amxc_htable_contains(mibs, name), exit);

    info = (mib_info_t*) calloc(1, sizeof(mib_info_t));
    when_null(info, exit);

    info->expression = strdup(expression);
    info->file = strdup(file);

    amxc_htable_insert(mibs, name, &info->hit);

exit:
    return;
}

static FILE* amxo_parser_open_mib_file(char* name,
                                       amxc_string_t* filename,
                                       char** first_line) {
    ssize_t read = 0;
    size_t len = 0;
    FILE* odlfile = fopen(amxc_string_get(filename, 0), "r");

    name[strlen(name) - 4] = 0;
    read = getline(first_line, &len, odlfile);
    if(read == -1) {
        free(*first_line);
        *first_line = NULL;
        fclose(odlfile);
        odlfile = NULL;
    }

    return odlfile;
}

static bool amxo_parser_build_mib_info(amxo_parser_t* parser,
                                       struct dirent* ep,
                                       amxc_string_t* filename,
                                       char* first_line) {
    bool retval = true;
    const char* name = NULL;
    const char* expression = NULL;
    amxp_expr_t expr;

    name = basename(ep->d_name);
    expression = name;
    if(strncmp(first_line, "/*expr:", 7) == 0) {
        first_line[ strlen(first_line) - 3 ] = 0;
        expression = first_line + 7;
    } else if(strncmp(first_line, "#expr:", 6) == 0) {
        first_line[ strlen(first_line) - 3 ] = 0;
        expression = first_line + 6;
    }
    if(amxp_expr_init(&expr, expression) == amxp_expr_status_ok) {
        amxo_parser_add_mib_info(&parser->mibs,
                                 name,
                                 expression,
                                 amxc_string_get(filename, 0));
    } else {
        retval = false;
    }
    amxp_expr_clean(&expr);

    return retval;
}

static int amxo_parser_scan(amxo_parser_t* parser,
                            const char* path) {
    int retval = -1;
    DIR* dp;
    struct dirent* ep;
    FILE* odlfile = NULL;
    char* line = NULL;
    amxc_string_t filename;

    amxc_string_init(&filename, 128);

    dp = opendir(path);
    when_null(dp, exit);

    for(ep = readdir(dp); ep; ep = readdir(dp)) {
        const char* extension = strstr(ep->d_name, ".odl");
        if((extension == NULL) || (extension[4] != 0)) {
            continue;
        }

        amxc_string_reset(&filename);
        amxc_string_setf(&filename, "%s/%s", path, ep->d_name);
        odlfile = amxo_parser_open_mib_file(ep->d_name, &filename, &line);
        if(odlfile == NULL) {
            continue;
        }

        amxo_parser_build_mib_info(parser, ep, &filename, line);
        fclose(odlfile);
        free(line);
        line = NULL;
    }
    closedir(dp);

    retval = 0;

exit:
    amxc_string_clean(&filename);
    return retval;
}

void amxo_parser_del_mib_info(AMXO_UNUSED const char* key,
                              amxc_htable_it_t* it) {
    mib_info_t* info = amxc_htable_it_get_data(it, mib_info_t, hit);
    free(info->expression);
    free(info->file);
    free(info);
}

int amxo_parser_scan_mib_dir(amxo_parser_t* parser,
                             const char* path) {
    int retval = -1;
    char* current_wd = getcwd(NULL, 0);
    char* real_path = NULL;

    when_null(parser, exit);
    when_str_empty(path, exit);

    real_path = realpath(path, NULL);
    if(real_path != NULL) {
        retval = amxo_parser_scan(parser, real_path);
    }

exit:
    free(current_wd);
    free(real_path);
    return retval;
}

int amxo_parser_scan_mib_dirs(amxo_parser_t* parser,
                              amxc_var_t* dirs) {
    int retval = -1;
    when_null(parser, exit);
    if(dirs == NULL) {
        dirs = amxo_parser_get_config(parser, "mib-dirs");
    }
    when_null(dirs, exit);
    when_true(amxc_var_type_of(dirs) != AMXC_VAR_ID_LIST, exit);

    amxc_var_for_each(var_path, dirs) {
        const char* path = amxc_var_constcast(cstring_t, var_path);
        if(path == NULL) {
            continue;
        }
        retval = amxo_parser_scan_mib_dir(parser, path);
        if(retval != 0) {
            break;
        }
    }

exit:
    return retval;
}

int amxo_parser_apply_mib(amxo_parser_t* parser,
                          amxd_object_t* object,
                          const char* mib_name) {
    int retval = -1;
    amxd_status_t status = amxd_status_ok;
    amxd_object_t* mib = NULL;
    amxd_dm_t* dm = NULL;

    when_null(parser, exit);
    when_null(object, exit);
    when_str_empty(mib_name, exit);

    dm = amxd_object_get_dm(object);
    when_null(dm, exit);

    mib = amxd_dm_get_mib(dm, mib_name);
    if(mib == NULL) {
        mib_info_t* info = NULL;
        amxc_htable_it_t* it = amxc_htable_get(&parser->mibs, mib_name);
        when_null(it, exit);
        info = amxc_htable_it_get_data(it, mib_info_t, hit);
        retval = amxo_parser_parse_file(parser,
                                        info->file,
                                        amxd_dm_get_root(dm));
        when_true(retval != 0, exit);
    }

    status = amxd_object_add_mib(object, mib_name);
    retval = status == amxd_status_ok ? 0 : -1;

exit:
    return retval;
}

int amxo_parser_apply_mibs(amxo_parser_t* parser,
                           amxd_object_t* object,
                           amxo_evaluate_expr_fn_t fn) {
    int retval = 0;

    when_null(parser, exit);
    when_null(object, exit);
    when_null(fn, exit);

    amxc_htable_for_each(it, (&parser->mibs)) {
        const char* mib_name = amxc_htable_it_get_key(it);
        mib_info_t* info = amxc_htable_it_get_data(it, mib_info_t, hit);
        amxp_expr_t expr;
        amxp_expr_init(&expr, info->expression);
        if(fn(object, &expr)) {
            if(amxo_parser_apply_mib(parser, object, mib_name) == 0) {
                retval++;
            }
        }
        amxp_expr_clean(&expr);
    }

exit:
    return retval;
}
