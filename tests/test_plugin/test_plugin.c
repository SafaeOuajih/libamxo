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

#include <stdlib.h>
#include <stdio.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxp/amxp_slot.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_action.h>
#include <amxd/amxd_transaction.h>

#include <amxo/amxo.h>

#define UNUSED __attribute__((unused))

static amxo_parser_t *local_parser = NULL;

int _test_entry_point(int reason,
                      amxd_dm_t *dm,
                      amxo_parser_t *parser);

int _test_failing_entry_point(UNUSED int reason,
                              UNUSED amxd_dm_t *dm,
                              UNUSED amxo_parser_t *parser);

amxd_status_t _test_func(amxd_object_t *object,
                         amxd_function_t *func,
                         amxc_var_t *args,
                         amxc_var_t *ret);
amxd_status_t _TestObject_TestFunc1(amxd_object_t *object,
                                    amxd_function_t *func,
                                    amxc_var_t *args,
                                    amxc_var_t *ret);
amxd_status_t _TestObject_TestFunc2(amxd_object_t *object,
                                    amxd_function_t *func,
                                    amxc_var_t *args,
                                    amxc_var_t *ret);
amxd_status_t _TestObject_TestFunc4(amxd_object_t *object,
                                    amxd_function_t *func,
                                    amxc_var_t *args,
                                    amxc_var_t *ret);

amxd_status_t __TestObject_TestFunc1(amxd_object_t *object,
                                     amxd_function_t *func,
                                     amxc_var_t *args,
                                     amxc_var_t *ret);
amxd_status_t TestObject_TestFunc2(amxd_object_t *object,
                                   amxd_function_t *func,
                                   amxc_var_t *args,
                                   amxc_var_t *ret);
amxd_status_t __TestFunc3(amxd_object_t *object,
                          amxd_function_t *func,
                          amxc_var_t *args,
                          amxc_var_t *ret);
amxd_status_t TestFunc4(amxd_object_t *object,
                        amxd_function_t *func,
                        amxc_var_t *args,
                        amxc_var_t *ret);

amxd_status_t _test_dummy_action(amxd_object_t * const object,
                                 amxd_param_t * const param,
                                 amxd_action_t reason,
                                 const amxc_var_t * const args,
                                 amxc_var_t * const retval,
                                 void *priv);

amxd_status_t _test_func(amxd_object_t *object,
                         amxd_function_t *func,
                         amxc_var_t *args,
                         amxc_var_t *ret) {
    char *path = amxd_object_get_path(object, AMXD_OBJECT_NAMED);
    printf("Function call: \n");
    printf("Object   - %s\n", path);
    printf("Function - %s\n", amxd_function_get_name(func));
    amxc_var_copy(ret, args);
    if(args != NULL) {
        fflush(stdout);
        amxc_var_dump(args, STDOUT_FILENO);
    }
    free(path);
    return 0;
}

amxd_status_t _TestObject_TestFunc1(amxd_object_t *object,
                                    amxd_function_t *func,
                                    amxc_var_t *args,
                                    amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}

amxd_status_t _TestObject_TestFunc2(amxd_object_t *object,
                                    amxd_function_t *func,
                                    amxc_var_t *args,
                                    amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}

amxd_status_t _TestObject_TestFunc4(amxd_object_t *object,
                                    amxd_function_t *func,
                                    amxc_var_t *args,
                                    amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}


// PCB Style symbols
//////////////////////////////////////////////////////////////
amxd_status_t __TestObject_TestFunc1(amxd_object_t *object,
                                     amxd_function_t *func,
                                     amxc_var_t *args,
                                     amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}

amxd_status_t TestObject_TestFunc2(amxd_object_t *object,
                                   amxd_function_t *func,
                                   amxc_var_t *args,
                                   amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}

amxd_status_t __TestFunc3(amxd_object_t *object,
                          amxd_function_t *func,
                          amxc_var_t *args,
                          amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}

amxd_status_t TestFunc4(amxd_object_t *object,
                        amxd_function_t *func,
                        amxc_var_t *args,
                        amxc_var_t *ret) {
    return _test_func(object, func, args, ret);
}
//////////////////////////////////////////////////////////////

int _test_entry_point(UNUSED int reason,
                      UNUSED amxd_dm_t *dm,
                      UNUSED amxo_parser_t *parser) {

    amxc_var_t *counter = amxo_parser_get_config(parser, "counter");
    amxc_var_set(uint32_t, counter, amxc_var_constcast(uint32_t, counter) + 1);

    if(reason == 0) {
        local_parser = parser;
    } else {
        local_parser = NULL;
    }
    return 0;
}

int _test_failing_entry_point(UNUSED int reason,
                              UNUSED amxd_dm_t *dm,
                              UNUSED amxo_parser_t *parser) {

    amxc_var_t *counter = amxo_parser_get_config(parser, "counter");
    amxc_var_set(uint32_t, counter, amxc_var_constcast(uint32_t, counter) + 1);

    return -1;
}

amxd_status_t _test_dummy_action(amxd_object_t * const object,
                                 amxd_param_t * const param,
                                 amxd_action_t reason,
                                 const amxc_var_t * const args,
                                 amxc_var_t * const retval,
                                 void *priv) {
    amxd_status_t status = amxd_status_ok;
    if(reason == action_param_read) {
        status = amxd_action_param_read(object, param, reason, args, retval, priv);
    }
    return status;
}