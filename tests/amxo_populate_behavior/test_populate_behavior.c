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
#include <amxd/amxd_transaction.h>
#include <amxo/amxo.h>


#include "test_populate_behavior.h"

#define UNUSED __attribute__((unused))

void test_none_existing_param_default_behavior(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* main_odl =
        "%define { object MyObject { string Text; } }\n"
        "%populate { object MyObject { parameter OtherParam = 10; } } ";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_parameter_not_found);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_none_existing_param_can_add(UNUSED void** state) {
    amxd_object_t* object = NULL;
    amxd_param_t* param = NULL;
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* main_odl =
        "%config { populate-behavior = { unknown-parameter = \"add\" }; }\n"
        "%define { object MyObject { string Text; } }\n"
        "%populate { object MyObject { parameter OtherParam = 10; } }\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_findf(&dm, "MyObject");
    param = amxd_object_get_param_def(object, "OtherParam");
    assert_ptr_not_equal(param, NULL);
    assert_int_equal(amxd_param_get_type(param), AMXC_VAR_ID_INT64);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_none_existing_param_warning(UNUSED void** state) {
    amxd_object_t* object = NULL;
    amxd_param_t* param = NULL;
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* main_odl =
        "%config { populate-behavior = { unknown-parameter = \"warning\" }; }\n"
        "%define { object MyObject { string Text; } }\n"
        "%populate { object MyObject { parameter OtherParam = 10; } }\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    object = amxd_dm_findf(&dm, "MyObject");
    param = amxd_object_get_param_def(object, "OtherParam");
    assert_ptr_equal(param, NULL);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_instance_default_behavior(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* main_odl =
        "%define {"
        "    object MyObject {"
        "        object MyTemplate[] {"
        "            string Text;"
        "        }"
        "    }"
        "}"
        "%populate {"
        "    object MyObject.MyTemplate {"
        "        instance add(1);"
        "        instance add(2);"
        "        instance add(1);"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_duplicate);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_instance_can_update(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;
    char* value = NULL;
    const char* main_odl =
        "%config { populate-behavior = { duplicate-instance = \"update\" }; }\n"
        "%define {\n"
        "    object MyObject {\n"
        "        object MyTemplate[] {\n"
        "            string Text;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "%populate {\n"
        "    object MyObject.MyTemplate {\n"
        "        instance add(1) { parameter Text = \"Test\"; }\n"
        "        instance add(2);\n"
        "        instance add(1) {\n"
        "            parameter Text = \"HALLO\";\n"
        "        }\n"
        "    }\n"
        "}\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    object = amxd_dm_findf(&dm, "MyObject.MyTemplate.1");
    value = amxd_object_get_value(cstring_t, object, "Text", NULL);
    assert_string_equal(value, "HALLO");

    free(value);
    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}

void test_duplicate_instance_with_keys_can_update(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;
    uint32_t value = 0;
    const char* main_odl =
        "%config { populate-behavior = { duplicate-instance = \"update\" }; }\n"
        "%define {\n"
        "    object MyObject {\n"
        "        object MyTemplate[] {\n"
        "            %key string Text;\n"
        "            uint32 Number;"
        "        }\n"
        "    }\n"
        "}\n"
        "%populate {\n"
        "    object MyObject.MyTemplate {\n"
        "        instance add(Text = \"Value1\") { parameter Number = 666; }\n"
        "        instance add(Text = \"Value2\");\n"
        "        instance add(Text = \"Value1\") {\n"
        "            parameter Number = 1234;\n"
        "        }\n"
        "    }\n"
        "}\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    object = amxd_dm_findf(&dm, "MyObject.MyTemplate.1");
    value = amxd_object_get_value(uint32_t, object, "Number", NULL);
    assert_int_equal(value, 1234);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}


void test_transactions_with_two_keys(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* templ = NULL;
    amxd_object_t* object = NULL;
    amxd_status_t status;
    const char* main_odl =
        "%define {\n"
        "    object MyObjectKeyTest {\n"
        "        object Node[] {\n"
        "            %key uint32 Key1;\n"
        "            %key uint32 Key2;\n"
        "        }\n"
        "    }\n"
        "}\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, main_odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    templ = amxd_dm_findf(&dm, "MyObjectKeyTest.Node");
    assert_ptr_not_equal(templ, NULL);

    amxd_trans_t trans;
    amxd_trans_init(&trans);
    amxd_trans_select_object(&trans, templ);
    amxd_trans_add_inst(&trans, 1, "N1");
    amxd_trans_set_value(uint32_t, &trans, "Key1", 0);
    amxd_trans_set_value(uint32_t, &trans, "Key2", 0);
    status = amxd_trans_apply(&trans, &dm);
    assert_int_equal(status, amxd_status_ok);
    amxd_trans_clean(&trans);

    object = amxd_object_findf(templ, "[Key1 == 0].");
    assert_ptr_not_equal(object, NULL);

    amxd_trans_init(&trans);
    amxd_trans_select_object(&trans, templ);
    amxd_trans_add_inst(&trans, 2, "N2");
    amxd_trans_set_value(uint32_t, &trans, "Key1", 1);
    amxd_trans_set_value(uint32_t, &trans, "Key2", 0);
    status = amxd_trans_apply(&trans, &dm);
    assert_int_equal(status, amxd_status_ok);
    amxd_trans_clean(&trans);

    object = amxd_object_findf(templ, "[Key1 == 1].");
    assert_ptr_not_equal(object, NULL);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
}
