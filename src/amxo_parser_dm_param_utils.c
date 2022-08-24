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

static int64_t amxo_attr_2_param_attr(int64_t attributes) {
    int64_t param_attrs = 0;
    if(SET_BIT(attr_readonly) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_read_only);
    }
    if(SET_BIT(attr_persistent) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_persistent);
    }
    if(SET_BIT(attr_private) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_private);
    }
    if(SET_BIT(attr_protected) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_protected);
    }
    if(SET_BIT(attr_template) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_template);
    }
    if(SET_BIT(attr_instance) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_instance);
    }
    if(SET_BIT(attr_variable) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_variable);
    }
    if(SET_BIT(attr_key) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_key);
    }
    if(SET_BIT(attr_unique) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_unique);
    }
    if(SET_BIT(attr_mutable) & attributes) {
        param_attrs |= SET_BIT(amxd_pattr_mutable);
    }
    return param_attrs;
}

static void amxo_parser_set_event(amxo_parser_t* pctx,
                                  event_id_t event) {
    amxc_lstack_it_t* it = amxc_llist_get_last(&pctx->event_list);
    event_t* e = amxc_container_of(it, event_t, it);
    e->id = event;
}

static void amxo_parser_data_event(amxo_parser_t* pctx,
                                   amxd_param_t* param) {
    amxc_lstack_it_t* it = amxc_llist_get_last(&pctx->event_list);
    event_t* e = amxc_container_of(it, event_t, it);
    amxc_var_t* value = NULL;

    when_true(e->id != event_object_change, exit);
    if(amxc_var_type_of(&e->data) != AMXC_VAR_ID_HTABLE) {
        amxc_var_set_type(&e->data, AMXC_VAR_ID_HTABLE);
    }
    value = GET_ARG(&e->data, amxd_param_get_name(param));
    when_not_null(value, exit);

    amxc_var_set_key(&e->data, amxd_param_get_name(param), &param->value, AMXC_VAR_FLAG_COPY);

exit:
    return;
}

static amxd_param_t* amxo_parser_new_param(amxo_parser_t* pctx,
                                           const char* name,
                                           int64_t pattrs,
                                           uint32_t type) {
    amxd_param_t* param = NULL;

    pctx->status = amxd_param_new(&param, name, type);
    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx, "Failed to create parameter %s", name);
        goto exit;
    }
    amxd_param_set_attrs(param, pattrs, true);
    pctx->status = amxd_object_add_param(pctx->object, param);

exit:
    return param;
}

static int amxo_parser_set_param_value(amxo_parser_t* pctx,
                                       const char* parent_path,
                                       const char* name,
                                       amxd_param_t* param,
                                       amxc_var_t* value) {
    int retval = -1;
    if((value != NULL) && (param != NULL)) {
        pctx->status = amxd_param_set_value(param, value);
        if(pctx->status == amxd_status_invalid_value) {
            amxo_parser_msg(pctx,
                            "Invalid parameter value for parameter %s in object \"%s\"",
                            name,
                            parent_path);
            goto exit;
        }

        if(pctx->status == amxd_status_ok) {
            pctx->param = param;
            amxo_hooks_set_param(pctx, value);
            retval = 0;
        } else {
            retval = -1;
        }
    } else {
        pctx->param = param;
        if(param != NULL) {
            retval = 0;
            amxo_hooks_set_param(pctx, value);
        } else {
            retval = 1;
        }
    }

exit:
    return retval;
}

static bool amxo_parser_add_param(amxo_parser_t* pctx,
                                  const char* name,
                                  int64_t pattrs,
                                  uint32_t type) {
    bool retval = false;
    amxd_param_t* param = NULL;

    if(amxd_object_get_type(pctx->object) == amxd_object_instance) {
        pattrs |= SET_BIT(amxd_pattr_instance);
    }
    param = amxo_parser_new_param(pctx, name, pattrs, type);
    when_null(param, exit);
    amxo_hooks_add_param(pctx, name, pattrs, type);

    pctx->param = param;
    retval = true;

exit:
    return retval;
}

static bool amxo_parser_update_param(amxo_parser_t* pctx,
                                     amxd_param_t* param,
                                     const char* name,
                                     uint32_t type) {
    bool retval = false;

    if(!amxo_parser_check_config(pctx,
                                 "define-behavior.existing-parameter",
                                 "update")) {
        amxo_parser_msg(pctx, "Duplicate parameter %s", name);
        pctx->status = amxd_status_duplicate;
        goto exit;
    }
    if((type != AMXC_VAR_ID_LIST) &&
       ( type != AMXC_VAR_ID_HTABLE) &&
       ( type != AMXC_VAR_ID_FD)) {
        amxc_var_set_type(&param->value, type);
    } else {
        amxo_parser_msg(pctx, "Invalid parameter type for parameter %s", name);
        pctx->status = amxd_status_invalid_type;
        goto exit;
    }

    pctx->param = param;
    retval = true;

exit:
    return retval;
}

static amxd_param_t* amxo_parser_check_param(amxo_parser_t* pctx,
                                             const char* name,
                                             const char* parent_path,
                                             amxc_var_t* value,
                                             int* retval) {
    amxd_param_t* param = NULL;
    if(amxo_parser_check_config(pctx, "populate-behavior.unknown-parameter", "add")) {
        uint32_t type = amxc_var_is_null(value) ? AMXC_VAR_ID_CSTRING : amxc_var_type_of(value);
        int64_t pattrs = SET_BIT(amxd_pattr_persistent);
        if(amxd_object_get_type(pctx->object) == amxd_object_instance) {
            pattrs |= SET_BIT(amxd_pattr_instance);
        }
        if(amxd_object_get_type(pctx->object) == amxd_object_template) {
            pattrs |= SET_BIT(amxd_pattr_instance);
            pattrs |= SET_BIT(amxd_pattr_template);
        }
        param = amxo_parser_new_param(pctx, name, pattrs, type);
    } else if(amxo_parser_check_config(pctx,
                                       "populate-behavior.unknown-parameter",
                                       "warning")) {
        amxo_parser_msg(pctx,
                        "Parameter %s not found in object \"%s\"",
                        name,
                        parent_path);
        pctx->status = amxd_status_parameter_not_found;
        *retval = 0;
    } else {
        amxo_parser_msg(pctx,
                        "Parameter %s not found in object \"%s\"",
                        name,
                        parent_path);
        pctx->status = amxd_status_parameter_not_found;
    }

    return param;
}

bool amxo_parser_set_param_attrs(amxo_parser_t* pctx, uint64_t attr, bool enable) {
    int64_t pattrs = amxo_attr_2_param_attr(attr);
    amxd_param_set_attrs(pctx->param, pattrs, enable);
    return true;
}

bool amxo_parser_set_param_flags(amxo_parser_t* pctx) {
    const amxc_htable_t* ht_flags = NULL;

    when_null(pctx->data, exit);
    when_true(amxc_var_type_of(pctx->data) != AMXC_VAR_ID_HTABLE, exit);

    ht_flags = amxc_var_constcast(amxc_htable_t, pctx->data);
    amxc_htable_for_each(it, ht_flags) {
        const char* flag_name = amxc_htable_it_get_key(it);
        amxc_var_t* flag = amxc_var_from_htable_it(it);
        if(amxc_var_dyncast(bool, flag)) {
            amxd_param_set_flag(pctx->param, flag_name);
        } else {
            amxd_param_unset_flag(pctx->param, flag_name);
        }
    }

    amxc_var_delete(&pctx->data);

exit:
    return true;
}

bool amxo_parser_push_param(amxo_parser_t* pctx,
                            const char* name,
                            int64_t attr_bitmask,
                            uint32_t type) {
    amxd_param_t* param = NULL;
    int64_t pattrs = amxo_attr_2_param_attr(attr_bitmask);
    bool retval = false;
    amxc_string_t res_name;
    amxc_string_init(&res_name, 0);

    amxo_parser_set_event(pctx, event_none);

    if(amxc_string_set_resolved(&res_name, name, &pctx->config) > 0) {
        name = amxc_string_get(&res_name, 0);
    }

    pctx->status = amxd_status_ok;
    param = amxd_object_get_param_def(pctx->object, name);
    if(param == NULL) {
        retval = amxo_parser_add_param(pctx, name, pattrs, type);
    } else {
        retval = amxo_parser_update_param(pctx, param, name, type);
    }

    amxc_string_clean(&res_name);
    return retval;
}

int amxo_parser_set_param(amxo_parser_t* pctx,
                          const char* name,
                          amxc_var_t* value) {
    amxd_param_t* param = NULL;
    int retval = -1;
    char* parent_path = amxd_object_get_path(pctx->object, AMXD_OBJECT_NAMED);
    amxc_string_t res_name;
    amxc_string_init(&res_name, 0);

    if(amxc_string_set_resolved(&res_name, name, &pctx->config) > 0) {
        name = amxc_string_get(&res_name, 0);
    }

    pctx->status = amxd_status_ok;
    param = pctx->param == NULL ? amxd_object_get_param_def(pctx->object, name) : pctx->param;
    if(param == NULL) {
        param = amxo_parser_check_param(pctx, name, parent_path, value, &retval);
    }
    if(param != NULL) {
        amxo_parser_data_event(pctx, param);
        retval = amxo_parser_set_param_value(pctx, parent_path, name, param, value);
    }

    amxc_string_clean(&res_name);
    free(parent_path);
    return retval;
}

bool amxo_parser_pop_param(amxo_parser_t* pctx) {
    bool retval = false;
    amxc_var_t value;
    amxc_var_init(&value);

    if(pctx->param != NULL) {
        amxc_var_copy(&value, &pctx->param->value);
        pctx->status = amxd_param_validate(pctx->param, &value);
        if(pctx->status != amxd_status_ok) {
            amxo_parser_msg(pctx, "Parameter %s validation failed",
                            amxd_param_get_name(pctx->param));
            goto exit;
        }

        amxo_hooks_end_param(pctx);
        pctx->param = NULL;
    }
    retval = true;

exit:
    amxc_var_clean(&value);
    return retval;
}
