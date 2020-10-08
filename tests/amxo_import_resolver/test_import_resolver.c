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
#include <amxo/amxo_hooks.h>

#include "test_import_resolver.h"

#define UNUSED __attribute__((unused))

static void check_can_invoke_functions(amxd_dm_t* dm) {
    amxc_var_t args;
    amxc_var_t ret;

    amxd_object_t* root = amxd_dm_get_root(dm);
    amxd_object_t* object = amxd_object_get_child(root, "TestObject");

    amxc_var_init(&args);
    amxc_var_init(&ret);
    amxc_var_set_type(&args, AMXC_VAR_ID_HTABLE);

    assert_ptr_not_equal(object, NULL);

    assert_int_equal(amxd_object_invoke_function(object, "TestFunc1", &args, &ret), amxd_status_ok);
    assert_int_equal(amxd_object_invoke_function(object, "TestFunc2", &args, &ret), amxd_status_ok);
    assert_int_equal(amxd_object_invoke_function(object, "TestFunc3", &args, &ret), amxd_status_ok);
    assert_int_equal(amxd_object_invoke_function(object, "TestFunc4", &args, &ret), amxd_status_ok);

    amxc_var_clean(&args);
    amxc_var_clean(&ret);
}

void test_import_resolver_resolves_pcb_style(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid1.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    check_can_invoke_functions(&dm);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
    amxo_resolver_import_close_all();
}

void test_import_resolver_resolves(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid2.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    check_can_invoke_functions(&dm);

    amxd_dm_clean(&dm);
    amxo_parser_clean(&parser);
    amxo_resolver_import_close_all();
}

void test_import_resolver_invalid_data(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odls[] = {
        "%define { object Test { void testa()<!import::_test_func!>;} }",
        "%define { object Test { void testb()<!import:test:_test_func:toomuch!>;} }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_resolver_import_open(&parser, "../test_plugin/test_plugin.so", "test", 0), 0);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_file_not_found);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_import_resolver_multiple_import(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odls[] = {
        "import \"../test_plugin/test_plugin.so\" as test; %define { object TestObject { void TestFunc1(); } }",
        "import \"../test_plugin/test_plugin.so\" as test2; %define { object TestObject { void TestFunc1();  } }",
        "import \"../test_plugin/test_plugin.so\" as test3; %define { object TestObject { void TestFunc1();  } }",
        "import \"../test_plugin/test_plugin.so\" as test; %define { object TestObject { void TestFunc1();  } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_resolver_import_open(&parser, "../test_plugin/test_plugin.so", "test", 0), 0);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
        amxd_dm_clean(&dm);
        amxo_parser_clean(&parser);
        amxo_parser_init(&parser);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_can_call_entry_point(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t counter;

    const char* odl = "%define { entry-point test.test_entry_point; }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxc_var_init(&counter);
    amxc_var_set(uint32_t, &counter, 0);
    amxo_parser_set_config(&parser, "counter", &counter);

    assert_int_equal(amxo_resolver_import_open(&parser, "../test_plugin/test_plugin.so", "test", 0), 0);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_int_equal(amxo_parser_invoke_entry_points(&parser, &dm, AMXO_START), 0);
    assert_int_equal(amxo_parser_invoke_entry_points(&parser, &dm, AMXO_STOP), 0);

    assert_int_equal(amxc_var_constcast(uint32_t, amxo_parser_get_config(&parser, "counter")), 2);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_entry_point_invocation_continues_after_failing_entry_point(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t counter;

    const char* odl = "%define { entry-point test.test_failing_entry_point; entry-point test.test_entry_point; }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxc_var_init(&counter);
    amxc_var_set(uint32_t, &counter, 0);
    amxo_parser_set_config(&parser, "counter", &counter);

    assert_int_equal(amxo_resolver_import_open(&parser, "../test_plugin/test_plugin.so", "test", 0), 0);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_int_equal(amxo_parser_invoke_entry_points(&parser, &dm, AMXO_START), 1);
    assert_int_equal(amxo_parser_invoke_entry_points(&parser, &dm, AMXO_STOP), 1);

    assert_int_equal(amxc_var_constcast(uint32_t, amxo_parser_get_config(&parser, "counter")), 4);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_parsing_fails_when_entry_point_can_not_be_resolved(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    const char* odl = "%define { entry-point test.not_existing; }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_file_not_found);

    assert_int_equal(amxo_resolver_import_open(&parser, "../test_plugin/test_plugin.so", "test", 0), 0);
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_function_not_found);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_entry_point_invoke_does_not_crash_with_invalid_args(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_invoke_entry_points(NULL, &dm, AMXO_START), 0);
    assert_int_not_equal(amxo_parser_invoke_entry_points(&parser, NULL, AMXO_START), 0);
    assert_int_equal(amxo_parser_invoke_entry_points(&parser, &dm, 666), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_open_non_existing_file(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "import \"NONE-EXISTING.so\" as fake;";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);
    assert_int_not_equal(amxo_resolver_import_open(&parser, "NONE-EXISTING.so", "test", 0), 0);
    assert_int_not_equal(amxo_resolver_import_open(NULL, "NONE-EXISTING.so", "test", 0), 0);
    assert_int_not_equal(amxo_resolver_import_open(&parser, "", "test", 0), 0);
    assert_int_not_equal(amxo_resolver_import_open(&parser, NULL, "test", 0), 0);
    assert_int_not_equal(amxo_resolver_import_open(&parser, "NONE-EXISTING.so", NULL, 0), 0);
    assert_int_not_equal(amxo_resolver_import_open(&parser, "NONE-EXISTING.so", "", 0), 0);
    assert_int_not_equal(amxo_resolver_import_open(&parser, "../test_plugin/test_plugin.so", "", 0), 0);
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_file_not_found);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}