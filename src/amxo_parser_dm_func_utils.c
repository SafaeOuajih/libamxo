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
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxp/amxp_slot.h>
#include <amxp/amxp_expression.h>
#include <amxd/amxd_common.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_object_expression.h>
#include <amxd/amxd_object_event.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"
#include "amxo_parser.tab.h"
#include "amxo_parser_hooks_priv.h"

static int64_t amxo_attr_2_func_attr(int64_t attributes) {
    int64_t func_attrs = 0;
    if(SET_BIT(attr_private) & attributes) {
        func_attrs |= SET_BIT(amxd_fattr_private);
    }
    if(SET_BIT(attr_protected) & attributes) {
        func_attrs |= SET_BIT(amxd_fattr_protected);
    }
    if(SET_BIT(attr_template) & attributes) {
        func_attrs |= SET_BIT(amxd_fattr_template);
    }
    if(SET_BIT(attr_instance) & attributes) {
        func_attrs |= SET_BIT(amxd_fattr_instance);
    }
    return func_attrs;
}

static int64_t amxo_attr_2_arg_attr(int64_t attributes) {
    int64_t arg_attrs = 0;
    if(SET_BIT(attr_in) & attributes) {
        arg_attrs |= SET_BIT(amxd_aattr_in);
    }
    if(SET_BIT(attr_out) & attributes) {
        arg_attrs |= SET_BIT(amxd_aattr_out);
    }
    if(SET_BIT(attr_mandatory) & attributes) {
        arg_attrs |= SET_BIT(amxd_aattr_mandatory);
    }
    if(SET_BIT(attr_strict) & attributes) {
        arg_attrs |= SET_BIT(amxd_aattr_strict);
    }
    return arg_attrs;
}

int amxo_parser_push_func(amxo_parser_t* pctx,
                          const char* name,
                          int64_t attr_bitmask,
                          uint32_t type) {
    amxd_function_t* func = NULL;
    amxd_function_t* orig_func = NULL;
    int64_t fattrs = amxo_attr_2_func_attr(attr_bitmask);
    int retval = -1;
    amxc_string_t res_name;
    amxc_string_init(&res_name, 0);

    if(amxc_string_set_resolved(&res_name, name, &pctx->config) > 0) {
        name = amxc_string_get(&res_name, 0);
    }

    orig_func = amxd_object_get_function(pctx->object, name);

    pctx->status = amxd_function_new(&func, name, type, NULL);
    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx, "Failed to create function %s", name);
        goto exit;
    }

    if(amxd_object_get_type(pctx->object) == amxd_object_instance) {
        fattrs |= SET_BIT(amxd_fattr_instance);
    }

    amxd_function_set_attrs(func, fattrs, true);

    if(orig_func != NULL) {
        if(amxd_function_get_owner(orig_func) == pctx->object) {
            amxd_function_delete(&orig_func);
        }
        amxo_parser_msg(pctx, "Overriding function %s", name);
        pctx->status = amxd_object_add_function(pctx->object, func);
        retval = 1;
    } else {
        pctx->status = amxd_object_add_function(pctx->object, func);
        retval = 0;
    }

    amxo_hooks_add_func(pctx, name, fattrs, type);

    pctx->func = func;

exit:
    amxc_string_clean(&res_name);
    return retval;
}

bool amxo_parser_set_function_flags(amxo_parser_t* pctx, amxc_var_t* flags) {
    const amxc_htable_t* ht_flags = NULL;

    when_null(flags, exit);
    when_true(amxc_var_type_of(flags) != AMXC_VAR_ID_HTABLE, exit);

    ht_flags = amxc_var_constcast(amxc_htable_t, flags);
    amxc_htable_for_each(it, ht_flags) {
        const char* flag_name = amxc_htable_it_get_key(it);
        amxc_var_t* flag = amxc_var_from_htable_it(it);
        if(amxc_var_dyncast(bool, flag)) {
            amxd_function_set_flag(pctx->func, flag_name);
        } else {
            amxd_function_unset_flag(pctx->func, flag_name);
        }
    }

    amxc_var_delete(&flags);

exit:
    return true;
}

void amxo_parser_pop_func(amxo_parser_t* pctx) {
    amxo_hooks_end_func(pctx);
    amxd_object_fn_t fn = (amxd_object_fn_t) pctx->resolved_fn;
    amxd_function_set_impl(pctx->func, fn);
    amxc_string_delete(&pctx->resolved_fn_name);
    pctx->func = NULL;
    pctx->resolved_fn = NULL;
}

bool amxo_parser_add_arg(amxo_parser_t* pctx,
                         const char* name,
                         int64_t attr_bitmask,
                         uint32_t type,
                         amxc_var_t* def_value) {
    bool retval = false;
    int64_t aattrs = amxo_attr_2_arg_attr(attr_bitmask);
    if(!IS_BIT_SET(aattrs, amxd_aattr_in) &&
       !IS_BIT_SET(aattrs, amxd_aattr_out)) {
        aattrs |= 1 << amxd_aattr_in;
    }

    amxo_hooks_add_func_arg(pctx, name, aattrs, type, def_value);

    pctx->status = amxd_function_new_arg(pctx->func, name, type, def_value);
    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx, "Failed to create/add function argument %s", name);
        goto exit;
    }

    amxd_function_arg_set_attrs(pctx->func, name, aattrs, true);
    retval = true;

exit:
    return retval;
}
