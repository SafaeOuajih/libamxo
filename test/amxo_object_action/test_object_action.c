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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <cmocka.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>

#include "test_object_action.h"

#include <amxc/amxc_macros.h>
static bool called = false;

static amxd_status_t failing_action(UNUSED amxd_object_t* const object,
                                    UNUSED amxd_param_t* const param,
                                    UNUSED amxd_action_t reason,
                                    UNUSED const amxc_var_t* const args,
                                    UNUSED amxc_var_t* const retval,
                                    UNUSED void* priv) {
    called = true;
    return amxd_status_invalid_value;
}

static amxd_status_t success_action(UNUSED amxd_object_t* const object,
                                    UNUSED amxd_param_t* const param,
                                    UNUSED amxd_action_t reason,
                                    UNUSED const amxc_var_t* const args,
                                    UNUSED amxc_var_t* const retval,
                                    UNUSED void* priv) {
    called = true;
    return amxd_status_ok;
}

static amxd_status_t data_action(UNUSED amxd_object_t* const object,
                                 UNUSED amxd_param_t* const param,
                                 UNUSED amxd_action_t reason,
                                 UNUSED const amxc_var_t* const args,
                                 UNUSED amxc_var_t* const retval,
                                 void* priv) {
    amxc_var_t* data = (amxc_var_t*) priv;

    assert_int_equal(amxc_var_type_of(data), AMXC_VAR_ID_HTABLE);
    assert_ptr_not_equal(amxc_var_get_path(data, "In", AMXC_VAR_FLAG_DEFAULT), NULL);
    assert_ptr_not_equal(amxc_var_get_path(data, "Out", AMXC_VAR_FLAG_DEFAULT), NULL);
    called = true;
    return amxd_status_ok;
}

void test_can_add_action_on_object(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "%define {\n"
        "    object MyObject {"
        "        on action validate call myvalidator;"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "myvalidator", AMXO_FUNC(success_action));

    called = false;
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_true(called);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_add_any_action_on_object(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "%define {\n"
        "    object MyObject {"
        "        on action any call myvalidator;"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "myvalidator", AMXO_FUNC(success_action));

    called = false;
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_true(called);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_proivide_data_to_object_action(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "%define {\n"
        "    object MyObject {"
        "        on action validate call myvalidator { In = 1, Out = 2 };"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "myvalidator", AMXO_FUNC(data_action));

    called = false;
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_true(called);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_failing_object_validation_makes_parser_fail(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "%define {\n"
        "    object MyObject {"
        "        on action validate call myvalidator;"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "myvalidator", AMXO_FUNC(failing_action));

    called = false;
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_invalid_value);
    assert_true(called);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}
