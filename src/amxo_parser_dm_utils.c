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
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>
#include <amxo/amxo_mibs.h>

#include "amxo_parser_priv.h"
#include "amxo_parser.tab.h"
#include "amxo_parser_hooks_priv.h"

static amxd_action_t object_actions[] = {
    action_any,
    action_object_read,      // action_read,
    action_object_write,     // action_write
    action_object_validate,  // action_validate
    action_object_list,      // action_list
    action_object_describe,  // action_describe
    action_object_add_inst,  // action_add_inst
    action_object_del_inst,  // action_del_inst
    action_object_destroy    // action_destroy
};

static amxd_action_t param_actions[] = {
    action_any,
    action_param_read,      // action_read,
    action_param_write,     // action_write
    action_param_validate,  // action_validate
    action_invalid,         // action_list
    action_param_describe,  // action_describe
    action_invalid,         // action_add_inst
    action_invalid,         // action_del_inst
    action_param_destroy    // action_destroy
};

static void amxo_parser_push_event(amxo_parser_t* pctx,
                                   event_id_t event) {
    event_t* e = (event_t*) calloc(1, sizeof(event_t));
    e->id = event;
    e->path = amxd_object_get_path(pctx->object, AMXD_OBJECT_TERMINATE);
    amxc_var_init(&e->data);

    amxc_llist_append(&pctx->event_list, &e->it);
}

static amxd_status_t amxo_cleanup_data(amxd_object_t* const object,
                                       amxd_param_t* const param,
                                       amxd_action_t reason,
                                       UNUSED const amxc_var_t* const args,
                                       UNUSED amxc_var_t* const retval,
                                       void* priv) {
    amxd_status_t status = amxd_status_ok;
    amxc_var_t* data = (amxc_var_t*) priv;

    if((reason != action_object_destroy) &&
       ( reason != action_param_destroy)) {
        status = amxd_status_function_not_implemented;
        goto exit;
    }

    // action private data must not be removed when the action is used
    // on derivced objects.
    // only remove action data when the action is owned by the object or
    // parameter on which the action is called.
    if(reason == action_object_destroy) {
        if(amxd_object_has_action_cb(object, reason, amxo_cleanup_data)) {
            amxc_var_delete(&data);
            amxd_object_set_action_cb_data(object, reason, amxo_cleanup_data, NULL);
        }
    } else {
        if(amxd_param_has_action_cb(param, reason, amxo_cleanup_data)) {
            amxc_var_delete(&data);
            amxd_param_set_action_cb_data(param, reason, amxo_cleanup_data, NULL);
        }
    }

exit:
    return status;
}

static amxd_status_t amxo_parser_set_param_action(amxd_param_t* param,
                                                  amxd_action_t action,
                                                  amxd_action_fn_t fn,
                                                  amxc_var_t* data) {
    amxd_status_t status = amxd_status_ok;

    status = amxd_param_add_action_cb(param, action, fn, data);
    if((data != NULL) && (status == amxd_status_ok)) {
        status = amxd_param_add_action_cb(param,
                                          action_param_destroy,
                                          amxo_cleanup_data,
                                          data);
        if(status != amxd_status_ok) {
            amxd_param_remove_action_cb(param, action, fn);
        }
    }

    return status;
}

static amxd_status_t amxo_parser_set_object_action(amxd_object_t* object,
                                                   amxd_action_t action,
                                                   amxd_action_fn_t fn,
                                                   amxc_var_t* data) {
    amxd_status_t status = amxd_status_ok;

    status = amxd_object_add_action_cb(object, action, fn, data);
    if((data != NULL) && (status == amxd_status_ok)) {
        status = amxd_object_add_action_cb(object,
                                           action_object_destroy,
                                           amxo_cleanup_data,
                                           data);
        if(status != amxd_status_ok) {
            amxd_object_remove_action_cb(object, action, fn);
        }
    }

    return status;
}

static int64_t amxo_attr_2_object_attr(int64_t attributes) {
    int64_t obj_attrs = 0;
    if(SET_BIT(attr_readonly) & attributes) {
        obj_attrs |= SET_BIT(amxd_oattr_read_only);
    }
    if(SET_BIT(attr_persistent) & attributes) {
        obj_attrs |= SET_BIT(amxd_oattr_persistent);
    }
    if(SET_BIT(attr_private) & attributes) {
        obj_attrs |= SET_BIT(amxd_oattr_private);
    }
    if(SET_BIT(attr_protected) & attributes) {
        obj_attrs |= SET_BIT(amxd_oattr_protected);
    }
    return obj_attrs;
}

static amxd_object_t* amxo_parser_new_object(amxo_parser_t* pctx,
                                             amxd_dm_t* dm,
                                             const char* name,
                                             int64_t oattrs,
                                             amxd_object_type_t type) {
    amxd_object_t* object = NULL;
    const char* type_name = type == amxd_object_mib ? "mib" : "object";

    pctx->status = amxd_object_new(&object, type, name);
    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx, "Failed to create %s %s", type_name, name);
        goto exit;
    }
    if(type == amxd_object_mib) {
        pctx->status = amxd_dm_store_mib(dm, object);
    } else {
        amxd_object_set_attrs(object, oattrs, true);
        pctx->status = amxd_object_add_object(pctx->object, object);
    }

exit:
    return object;
}

static int amxo_parser_can_update_object(amxo_parser_t* pctx,
                                         const char* name,
                                         amxd_object_type_t type) {
    int retval = 0;
    const char* type_name = type == amxd_object_mib ? "mib" : "object";
    if(amxo_parser_check_config(pctx,
                                "define-behavior.existing-object",
                                "update")) {
        pctx->status = amxd_status_ok;
        goto exit;
    }
    amxo_parser_msg(pctx, "Duplicate %s %s", type_name, name);
    pctx->status = amxd_status_duplicate;
    retval = -1;

exit:
    return retval;
}

static amxd_object_t* amxo_parser_can_update_instance(amxo_parser_t* pctx,
                                                      uint32_t index,
                                                      const char* name) {
    amxd_object_t* object = NULL;
    if(!amxo_parser_check_config(pctx,
                                 "populate-behavior.duplicate-instance",
                                 "update")) {
        amxo_parser_msg(pctx, "Duplicate instance");
        goto exit;
    }
    if(pctx->data != NULL) {
        amxp_expr_t* expr = NULL;
        amxd_object_new_key_expr(pctx->object, &expr, pctx->data);
        object = amxd_object_find_instance(pctx->object, expr);
        amxp_expr_delete(&expr);
    }
    if(object == NULL) {
        object = amxd_object_get_instance(pctx->object, name, index);
    }

exit:
    return object;
}

static int amxo_parser_connect(amxo_parser_t* pctx,
                               amxp_signal_mngr_t* const sig_mngr,
                               const char* const signal_name,
                               bool signal_is_regexp,
                               const char* const expression,
                               amxp_slot_fn_t fn) {
    int retval = 0;

    if(signal_is_regexp) {
        retval = amxp_slot_connect_filtered(sig_mngr,
                                            signal_name,
                                            expression,
                                            fn,
                                            pctx->resolved_fn_name);
    } else {
        retval = amxp_slot_connect(sig_mngr,
                                   signal_name,
                                   expression,
                                   fn,
                                   pctx->resolved_fn_name);
    }
    if(retval != 0) {
        retval = -1;
        pctx->status = amxd_status_invalid_value;
        if(expression != NULL) {
            amxo_parser_msg(pctx, "Subscribe failed : %s", expression);
        } else {
            amxo_parser_msg(pctx, "Subscribe failed : %s", signal_name);
        }
        amxc_string_delete(&pctx->resolved_fn_name);
    } else {
        amxc_llist_append(&pctx->function_names, &pctx->resolved_fn_name->it);
        pctx->resolved_fn_name = NULL;
    }

    pctx->resolved_fn_name = NULL;
    return retval;
}

static amxd_object_t* amxd_parser_add_instance_msg(amxo_parser_t* pctx,
                                                   uint32_t index,
                                                   const char* name,
                                                   amxd_object_t* object) {
    switch(pctx->status) {
    case amxd_status_ok:
        break;
    case amxd_status_invalid_name:
        amxo_parser_msg(pctx, "Failed to create instance %s - invalid name", name);
        break;
    case amxd_status_invalid_type:
        amxo_parser_msg(pctx,
                        "Failed to create instance %s - parent is not a template object",
                        name);
        break;
    case amxd_status_duplicate:
        object = amxo_parser_can_update_instance(pctx, index, name);
        if(object != NULL) {
            pctx->status = amxd_status_ok;
        }
        break;
    case amxd_status_missing_key:
        amxo_parser_msg(pctx, "Failed to create instance %s - missing key(s)", name);
        break;
    default:
        amxo_parser_msg(pctx, "Failed to create instance %s", name);
        break;
    }

    return object;
}

bool amxo_parser_check_config(amxo_parser_t* pctx,
                              const char* path,
                              const char* check) {
    amxc_var_t* option = amxc_var_get_path(&pctx->config,
                                           path,
                                           AMXC_VAR_FLAG_DEFAULT);
    const char* value = NULL;
    value = amxc_var_constcast(cstring_t, option);

    if(value == NULL) {
        value = "error";
    }

    return strcmp(value, check) == 0;
}

void amxo_parser_free_event(amxc_llist_it_t* it) {
    event_t* e = amxc_container_of(it, event_t, it);
    amxc_var_clean(&e->data);
    free(e);
}

bool amxo_parser_check_attr(amxo_parser_t* pctx,
                            int64_t attributes,
                            int64_t bitmask) {
    bool retval = false;
    int attr_mask = ~bitmask;

    int check_attr = attributes | attr_mask;
    pctx->status = (check_attr != attr_mask) ? amxd_status_ok : amxd_status_invalid_attr;
    retval = (check_attr != attr_mask);
    if(!retval) {
        amxo_parser_msg(pctx, "Invalid attributes given");
    }
    return retval;
}

bool amxo_parser_set_object_attrs(amxo_parser_t* pctx, uint64_t attr, bool enable) {
    int64_t oattrs = amxo_attr_2_object_attr(attr);
    amxd_object_set_attrs(pctx->object, oattrs, enable);
    return true;
}

int amxo_parser_create_object(amxo_parser_t* pctx,
                              const char* name,
                              int64_t attr_bitmask,
                              amxd_object_type_t type) {
    amxd_object_t* object = NULL;
    amxc_string_t res_name;
    amxc_string_init(&res_name, 0);

    int64_t oattrs = amxo_attr_2_object_attr(attr_bitmask);
    int retval = -1;
    amxd_dm_t* dm = amxd_object_get_dm(pctx->object);

    if(amxc_string_set_resolved(&res_name, name, &pctx->config) > 0) {
        name = amxc_string_get(&res_name, 0);
    }

    pctx->status = amxd_status_ok;
    if(type == amxd_object_mib) {
        object = amxd_dm_get_mib(dm, name);
    } else {
        object = amxd_object_findf(pctx->object, "%s", name);
    }
    if(object == NULL) {
        object = amxo_parser_new_object(pctx, dm, name, oattrs, type);
        when_null(object, exit);
    } else {
        retval = amxo_parser_can_update_object(pctx, name, type);
        when_true(retval < 0, exit);
        amxd_object_set_attrs(object, oattrs, true);
    }

    amxo_hooks_create_object(pctx, name, oattrs, type);

    amxc_astack_push(&pctx->object_stack, pctx->object);
    pctx->object = object;
    amxo_parser_push_event(pctx, event_none);
    retval = 0;

exit:
    amxc_string_clean(&res_name);
    return retval;
}

bool amxo_parser_add_instance(amxo_parser_t* pctx,
                              const char* parent,
                              uint32_t index,
                              const char* name) {
    amxd_object_t* object = NULL;
    amxd_object_t* parent_obj = pctx->object;
    bool retval = false;
    amxc_string_t res_name;
    amxc_string_init(&res_name, 0);

    if(amxc_string_set_resolved(&res_name, name, &pctx->config) > 0) {
        name = amxc_string_get(&res_name, 0);
    }

    if(parent != NULL) {
        parent_obj = amxd_object_findf(parent_obj, "%s", parent);
        when_null(parent_obj, exit);
    }
    pctx->status = amxd_object_add_instance(&object,
                                            parent_obj,
                                            name,
                                            index,
                                            pctx->data);
    object = amxd_parser_add_instance_msg(pctx, index, name, object);
    when_null(object, exit);
    when_failed(pctx->status, exit);
    amxo_hooks_add_instance(pctx, amxd_object_get_index(object),
                            amxd_object_get_name(object, AMXD_OBJECT_NAMED));
    amxc_astack_push(&pctx->object_stack, pctx->object);
    pctx->object = object;
    amxo_parser_push_event(pctx, event_instance_add);
    retval = true;

exit:
    amxc_string_clean(&res_name);
    amxc_var_delete(&pctx->data);
    return retval;
}

bool amxo_parser_push_object(amxo_parser_t* pctx,
                             const char* path) {
    amxd_object_t* object = NULL;
    bool retval = false;
    amxc_string_t res_path;
    amxc_string_init(&res_path, 0);

    if(amxc_string_set_resolved(&res_path, path, &pctx->config) > 0) {
        path = amxc_string_get(&res_path, 0);
    }

    pctx->status = amxd_status_ok;
    object = amxd_object_findf(pctx->object, "%s", path);
    if(object == NULL) {
        char* parent_path = amxd_object_get_path(pctx->object, AMXD_OBJECT_NAMED);
        amxo_parser_msg(pctx,
                        "Object %s not found (start searching from \"%s\")",
                        path,
                        parent_path == NULL ? "root" : parent_path);
        free(parent_path);
        pctx->status = amxd_status_object_not_found;
        goto exit;
    }

    amxo_hooks_select_object(pctx, path);

    amxc_astack_push(&pctx->object_stack, pctx->object);
    pctx->object = object;
    amxo_parser_push_event(pctx, event_object_change);
    retval = true;

exit:
    amxc_string_clean(&res_path);
    return retval;
}

bool amxo_parser_pop_object(amxo_parser_t* pctx) {
    bool retval = false;
    amxd_object_type_t type = amxd_object_get_type(pctx->object);
    const char* type_name = (type == amxd_object_mib) ? "mib" : "object";
    amxc_var_t args;

    pctx->status = amxd_object_validate(pctx->object, 0);

    amxc_var_init(&args);
    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);

    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx, "%s %s validation failed",
                        type_name,
                        amxd_object_get_name(pctx->object, AMXD_OBJECT_NAMED));
        goto exit;
    }

    amxd_dm_invoke_action(pctx->object, NULL, action_object_write, &args, NULL);
    amxo_hooks_end_object(pctx);

    pctx->object = (amxd_object_t*) amxc_astack_pop(&pctx->object_stack);

    retval = true;

exit:
    amxc_var_clean(&args);
    return retval;
}

bool amxo_parser_set_counter(amxo_parser_t* pctx,
                             const char* param_name) {
    bool retval = false;
    amxc_string_t res_param_name;
    amxc_string_init(&res_param_name, 0);

    if(amxc_string_set_resolved(&res_param_name, param_name, &pctx->config) > 0) {
        param_name = amxc_string_get(&res_param_name, 0);
    }

    pctx->status = amxd_object_set_counter(pctx->object, param_name);
    if(pctx->status != amxd_status_ok) {
        char* path = amxd_object_get_path(pctx->object, AMXD_OBJECT_NAMED);
        amxo_parser_msg(pctx,
                        "Failed to to set instance counter %s on %s",
                        param_name,
                        path);
        pctx->status = amxd_status_duplicate;
        free(path);
        goto exit;
    }

    amxo_hooks_set_counter(pctx, param_name);

    retval = true;

exit:
    amxc_string_clean(&res_param_name);
    return retval;
}

int amxo_parser_subscribe_path(amxo_parser_t* pctx,
                               const char* event,
                               bool event_is_regexp,
                               const char* path,
                               bool path_is_regexp) {
    int retval = 1;
    amxc_string_t res_path;
    amxc_string_t expression;
    const char* expr = NULL;
    amxd_dm_t* dm = amxd_object_get_dm(pctx->object);
    amxp_slot_fn_t fn = (amxp_slot_fn_t) pctx->resolved_fn;

    amxc_string_init(&expression, 0);
    amxc_string_init(&res_path, 0);

    when_true_status(amxo_parser_no_resolve(pctx), exit, retval = 0);
    when_null(dm, exit);

    if(pctx->resolved_fn == NULL) {
        amxo_parser_msg(pctx,
                        "No event subscription created - no function was resolved");
        pctx->status = amxd_status_ok;
        amxc_var_delete(&pctx->data);
        goto exit;
    }

    if(amxc_string_set_resolved(&res_path, path, &pctx->config) > 0) {
        path = amxc_string_get(&res_path, 0);
    }
    if(path_is_regexp) {
        amxc_string_appendf(&expression, "object matches \"%s\"", path);
    } else {
        amxc_string_appendf(&expression, "object starts with \"%s\"", path);
    }
    expr = amxc_string_get(&expression, 0);
    retval = amxo_parser_connect(pctx, &dm->sigmngr, event, event_is_regexp, expr, fn);

exit:
    amxc_string_clean(&expression);
    amxc_string_clean(&res_path);

    return retval;
}

int amxo_parser_subscribe(amxo_parser_t* pctx,
                          const char* event,
                          bool event_is_regexp,
                          const char* full_expr) {
    int retval = 1;
    amxd_dm_t* dm = amxd_object_get_dm(pctx->object);
    amxp_slot_fn_t fn = (amxp_slot_fn_t) pctx->resolved_fn;

    when_true_status(amxo_parser_no_resolve(pctx), exit, retval = 0);
    when_null(dm, exit);

    if(pctx->resolved_fn == NULL) {
        amxo_parser_msg(pctx,
                        "No event subscription created - no function was resolved");
        pctx->status = amxd_status_ok;
        amxc_var_delete(&pctx->data);
        goto exit;
    }

    if(full_expr != NULL) {
        amxc_string_t res_expr;
        amxc_string_init(&res_expr, 0);
        if(amxc_string_set_resolved(&res_expr, full_expr, &pctx->config) > 0) {
            full_expr = amxc_string_get(&res_expr, 0);
        }
        retval = amxo_parser_connect(pctx, &dm->sigmngr, event, event_is_regexp, full_expr, fn);
        amxc_string_clean(&res_expr);
    } else {
        retval = amxo_parser_connect(pctx, &dm->sigmngr, event, event_is_regexp, NULL, fn);
    }

exit:
    return retval;
}

int amxo_parser_set_action(amxo_parser_t* pctx,
                           amxo_action_t action) {

    int retval = -1;
    amxc_var_t* ca = GET_ARG(&pctx->config, "_current_action");
    pctx->status = amxd_status_ok;

    if(amxo_parser_no_resolve(pctx)) {
        retval = 0;
        amxc_var_delete(&pctx->data);
        goto exit;
    }

    if(pctx->resolved_fn == NULL) {
        retval = 1;
        pctx->status = amxd_status_file_not_found;
        amxo_parser_msg(pctx, "Action %d not set - no function resolved", action);
        goto exit;
    }

    if(pctx->param != NULL) {
        if(param_actions[action] == action_invalid) {
            pctx->status = amxd_status_invalid_action;
            amxo_parser_msg(pctx, "Invalid parameter action (action id = %d)", action);
            goto exit;
        }
        pctx->status = amxo_parser_set_param_action(pctx->param,
                                                    param_actions[action],
                                                    (amxd_action_fn_t) pctx->resolved_fn,
                                                    pctx->data);
    } else {
        pctx->status = amxo_parser_set_object_action(pctx->object,
                                                     object_actions[action],
                                                     (amxd_action_fn_t) pctx->resolved_fn,
                                                     pctx->data);
    }

    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx, "Failed to set action (action id = %d)", action);
    } else {
        retval = 0;
    }

exit:
    amxc_var_delete(&ca);
    amxc_string_delete(&pctx->resolved_fn_name);
    if(retval != 0) {
        amxc_var_delete(&pctx->data);
    } else {
        pctx->data = NULL;
    }
    return retval;
}

bool amxo_parser_add_mib(amxo_parser_t* pctx,
                         const char* mib_name) {
    bool retval = false;
    amxd_dm_t* dm = amxd_object_get_dm(pctx->object);
    amxd_object_t* mib = NULL;
    amxc_string_t res_mib_name;
    amxc_string_init(&res_mib_name, 0);

    if(amxc_string_set_resolved(&res_mib_name, mib_name, &pctx->config) > 0) {
        mib_name = amxc_string_get(&res_mib_name, 0);
    }

    when_null(dm, exit);

    if(amxd_object_has_mib(pctx->object, mib_name)) {
        retval = true;
        goto exit;
    }

    mib = amxd_dm_get_mib(dm, mib_name);
    if(mib == NULL) {
        const char* file = amxo_parser_get_mib_file(pctx, mib_name);
        if(file != NULL) {
            amxo_parser_include(pctx, file);
            mib = amxd_dm_get_mib(dm, mib_name);
        }
        if(mib == NULL) {
            amxo_parser_msg(pctx, "MIB %s is not found", mib_name);
            pctx->status = amxd_status_object_not_found;
            goto exit;
        }
    }

    pctx->status = amxd_object_add_mib(pctx->object, mib_name);
    if(pctx->status != amxd_status_ok) {
        amxo_parser_msg(pctx,
                        "Failed to add MIB %s on object %s",
                        mib_name,
                        amxd_object_get_name(pctx->object, AMXD_OBJECT_NAMED));
        goto exit;
    }

    retval = true;

    amxo_hooks_add_mib(pctx, mib_name);

exit:
    amxc_string_clean(&res_mib_name);
    return retval;
}
