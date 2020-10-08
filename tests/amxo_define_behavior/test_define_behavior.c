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

#include "test_define_behavior.h"

#define UNUSED __attribute__((unused))

void test_duplicate_objects_default_behavior(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* main_odl = "%define { object MyObject; }";
    const char* second_odl = "%define { object MyObject; }";
    const char* odls[] = {
        "%define { object MyObject; object MyObject; }",
        "%define { object MyObject; } %define { object MyObject; }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_duplicate);
        amxd_dm_clean(&dm);
    }

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_int_not_equal(amxo_parser_parse_string(&parser, second_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_duplicate);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_objects_can_update(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* main_odl = "%define { object MyObject; }";
    const char* second_odl = "%config { define-behavior = { existing-object = \"update\" }; } %define { object MyObject; }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_int_equal(amxo_parser_parse_string(&parser, second_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_objects_can_add_parameter(UNUSED void** state) {
    amxd_dm_t dm;
    amxd_object_t* object = NULL;
    amxd_param_t* param = NULL;
    amxo_parser_t parser;
    const char* main_odl = "%define { object MyObject { string Text; } }";
    const char* second_odl =
        "%config { define-behavior = { existing-object = \"update\" }; }"
        "%define {"
        "    object MyObject {"
        "        string TestParam;"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_int_equal(amxo_parser_parse_string(&parser, second_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_findf(&dm, "MyObject");
    assert_ptr_not_equal(object, NULL);
    param = amxd_object_get_param_def(object, "TestParam");
    assert_ptr_not_equal(param, NULL);
    param = amxd_object_get_param_def(object, "Text");
    assert_ptr_not_equal(param, NULL);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_parameters_default_behavior(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "%define { object MyObject { string TestParam; uint32 TestParam; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_duplicate);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_parameters_can_update(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;
    amxd_param_t* param = NULL;
    const char* odl =
        "%config { define-behavior = { existing-parameter = \"update\" }; }"
        "%define { object MyObject { string TestParam; uint32 TestParam; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_findf(&dm, "MyObject");
    param = amxd_object_get_param_def(object, "TestParam");
    assert_ptr_not_equal(param, NULL);
    assert_int_equal(amxd_param_get_type(param), AMXC_VAR_ID_UINT32);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_parameters_change_to_invalid_type_fails(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "%config { define-behavior = { existing-parameter = \"update\" }; }"
        "%define { object MyObject { string TestParam; htable TestParam; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_invalid_type);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}
