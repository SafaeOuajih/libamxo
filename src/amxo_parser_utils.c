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
#include <syslog.h>

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

static char* amxo_parser_get_resolver_name(const char* data) {
    amxc_string_t full_data;
    size_t length = strlen(data);
    amxc_llist_t parts;
    amxc_llist_it_t* it = NULL;
    char* name = NULL;

    amxc_llist_init(&parts);
    amxc_string_init(&full_data, length + 1);
    amxc_string_set_at(&full_data, 0, data, length, amxc_string_overwrite);
    amxc_string_split_to_llist(&full_data, &parts, ':');
    it = amxc_llist_take_first(&parts);
    amxc_llist_clean(&parts, amxc_string_list_it_free);
    amxc_string_clean(&full_data);

    when_null(it, exit);

    amxc_string_trim(amxc_string_from_llist_it(it), NULL);
    name = amxc_string_take_buffer(amxc_string_from_llist_it(it));
    amxc_llist_it_clean(it, amxc_string_list_it_free);

exit:
    return name;
}

bool amxo_parser_no_resolve(amxo_parser_t* parser) {
    amxc_var_t* var_resolve = GET_ARG(&parser->config, "odl-resolve");
    bool resolve = true;

    if(var_resolve != NULL) {
        resolve = amxc_var_dyncast(bool, var_resolve);
    }

    return !resolve;
}

void amxo_parser_msg(amxo_parser_t* parser, const char* format, ...) {
    amxc_string_reset(&parser->msg);
    va_list args;
    va_start(args, format);
    amxc_string_vsetf(&parser->msg, format, args);
    va_end(args);
    va_start(args, format);
    vsyslog(LOG_PID | LOG_USER, format, args);
    va_end(args);
}

int amxo_parser_printf(amxo_parser_t* parser, const char* format, ...) {
    bool silent = GET_BOOL(&parser->config, "silent");
    va_list args;
    va_start(args, format);
    if(!silent) {
        vfprintf(stderr, format, args);
    }
    va_end(args);
    va_start(args, format);
    vsyslog(LOG_PID | LOG_USER, format, args);
    va_end(args);
    return 0;
}

int amxo_parser_resolve_internal(amxo_parser_t* pctx,
                                 const char* fn_name,
                                 amxo_fn_type_t type,
                                 const char* data) {
    int retval = -1;
    char* name = NULL;
    const char* res_data = NULL;

    when_true_status(amxo_parser_no_resolve(pctx), exit, retval = 0);

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
    retval = amxo_parser_resolve(pctx, name, fn_name, type, res_data);
    if(retval == -1) {
        pctx->status = amxd_status_invalid_name;
        amxo_parser_msg(pctx, "No function resolver found with name \"%s\"", name);
    } else if(retval == 1) {
        pctx->status = amxd_status_function_not_found;
        amxo_parser_msg(pctx,
                        "No function implemention found for \"%s\" using \"%s\"",
                        fn_name,
                        name);
    } else if(retval == 0) {
        amxc_string_new(&pctx->resolved_fn_name, 0);
        amxc_string_set(pctx->resolved_fn_name, fn_name);
    }

exit:
    free(name);
    return retval;
}

int amxo_parser_call_entry_point(amxo_parser_t* pctx,
                                 const char* lib_name,
                                 const char* fn_name) {
    int retval = -1;
    amxc_string_t data;
    amxc_string_init(&data, 0);
    amxc_string_setf(&data, "%s", lib_name);

    when_true_status(amxo_parser_no_resolve(pctx), exit, retval = 0);

    pctx->resolved_fn = NULL;
    retval = amxo_parser_resolve(pctx, "import", fn_name, amxo_function_ep, amxc_string_get(&data, 0));
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
        amxc_string_delete(&pctx->resolved_fn_name);
    }

exit:
    amxc_string_clean(&data);
    return retval;
}

amxo_action_t amxo_parser_get_action_id(amxo_parser_t* pctx,
                                        const char* action_name) {
    static const char* names[] = {
        "any",
        "read",
        "write",
        "validate",
        "list",
        "describe",
        "add-inst",
        "del-inst",
        "destroy"
    };
    amxo_action_t action_id = amxo_action_invalid;
    amxc_var_t* ca = GET_ARG(&pctx->config, "_current_action");

    for(int i = 0; i <= action_max; i++) {
        if(strcmp(action_name, names[i]) == 0) {
            action_id = (amxo_action_t) i;
            break;
        }
    }

    if(action_id < 0) {
        pctx->status = amxd_status_invalid_action;
        amxo_parser_msg(pctx,
                        "Invalid action name \"%s\"",
                        action_name);
    }

    if(ca == NULL) {
        amxc_var_add_key(uint32_t, &pctx->config, "_current_action", action_id);
    } else {
        amxc_var_set(uint32_t, ca, action_id);
    }

    return action_id;
}

void amxo_parser_print(amxo_parser_t* pctx, const char* text) {
    amxc_string_t print_txt;

    amxc_string_init(&print_txt, 0);

    if(amxc_string_set_resolved(&print_txt, text, &pctx->config) > 0) {
        text = amxc_string_get(&print_txt, 0);
    }

    printf("%s (%s@%d)\n", text, pctx->file, pctx->line);

    amxc_string_clean(&print_txt);
}
