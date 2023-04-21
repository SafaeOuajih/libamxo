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
#include <amxp/amxp.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_object_expression.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>
#include <amxo/amxo_mibs.h>

#include "test_mibs.h"

#include <amxc/amxc_macros.h>
void test_can_scan_mib_dir(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib_valid"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_scan_mib_dir_fails_when_invalid_expr(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib_dir.odl"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_scan_mib_dir_skips_mib_when_already_known(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 4);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 4);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_scan_mib_dir_fails_with_invalid_arguments(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_scan_mib_dir(NULL, "./mibs"), 0);
    assert_int_not_equal(amxo_parser_scan_mib_dir(&parser, "./not_existing_dir"), 0);
    assert_int_not_equal(amxo_parser_scan_mib_dir(&parser, NULL), 0);
    assert_int_not_equal(amxo_parser_scan_mib_dir(&parser, ""), 0);
    assert_int_not_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib1.odl"), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_scan_mib_dirs(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t dirs;
    amxc_var_t* mib_dir = NULL;

    amxc_var_init(&dirs);
    amxc_var_set_type(&dirs, AMXC_VAR_ID_LIST);
    amxc_var_add(cstring_t, &dirs, "./mibs");
    amxc_var_add(cstring_t, &dirs, "./mibs/test_mib_valid");
    amxc_var_add(cstring_t, &dirs, "./${mib-dir}/test_mib_valid");

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    mib_dir = amxo_parser_claim_config(&parser, "mib-dir");
    amxc_var_set(cstring_t, mib_dir, "mibs");

    assert_int_equal(amxo_parser_scan_mib_dirs(&parser, &dirs), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    amxc_var_clean(&dirs);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_scan_mib_dirs_using_config(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t* dirs = NULL;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    dirs = amxo_parser_claim_config(&parser, "mib-dirs");
    amxc_var_set_type(dirs, AMXC_VAR_ID_LIST);
    amxc_var_add(cstring_t, dirs, "./mibs");
    amxc_var_add(cstring_t, dirs, "./mibs/test_mib_valid");

    assert_int_equal(amxo_parser_scan_mib_dirs(&parser, NULL), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_scan_mib_dirs_fails_with_invalid_arguments(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t dirs;

    amxc_var_init(&dirs);
    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_scan_mib_dirs(NULL, NULL), 0);
    assert_int_not_equal(amxo_parser_scan_mib_dirs(&parser, NULL), 0);
    assert_int_not_equal(amxo_parser_scan_mib_dirs(&parser, &dirs), 0);

    amxc_var_set_type(&dirs, AMXC_VAR_ID_LIST);
    amxc_var_add(cstring_t, &dirs, "");
    assert_int_not_equal(amxo_parser_scan_mib_dirs(&parser, &dirs), 0);
    amxc_var_clean(&dirs);
    amxc_var_set_type(&dirs, AMXC_VAR_ID_LIST);
    amxc_var_add(uint32_t, &dirs, 123);
    assert_int_not_equal(amxo_parser_scan_mib_dirs(&parser, &dirs), 0);

    amxc_var_clean(&dirs);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_apply_mib_to_object(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;
    const char* odl = "%define { object Test { string MyParam; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 4);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_int_equal(amxo_parser_apply_mib(&parser, object, "test_mib1"), 0);
    assert_non_null(amxd_object_get_param_def(object, "Mib1Text"));

    assert_true(amxd_object_has_mib(object, "test_mib1"));
    assert_int_equal(amxd_object_remove_mib(object, "test_mib1"), 0);
    assert_null(amxd_object_get_param_def(object, "Mib1Text"));

    assert_int_equal(amxo_parser_apply_mib(&parser, object, "test_mib1"), 0);
    assert_non_null(amxd_object_get_param_def(object, "Mib1Text"));

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_apply_unknown_mib_fails(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;
    const char* odl = "%define { object Test { string MyParam; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 4);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_int_not_equal(amxo_parser_apply_mib(&parser, object, "unknown_mib1"), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_apply_fails_with_invalid_arguments(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;
    amxd_object_t* not_in_dm = NULL;
    const char* odl = "%define { object Test { string MyParam; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxd_object_new(&not_in_dm, amxd_object_singleton, "Test");

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 4);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_int_not_equal(amxo_parser_apply_mib(NULL, object, "test_mib1"), 0);
    assert_int_not_equal(amxo_parser_apply_mib(&parser, NULL, "test_mib1"), 0);
    assert_int_not_equal(amxo_parser_apply_mib(&parser, object, NULL), 0);
    assert_int_not_equal(amxo_parser_apply_mib(&parser, object, ""), 0);
    assert_int_not_equal(amxo_parser_apply_mib(&parser, not_in_dm, "tesT_mib1"), 0);

    amxd_object_delete(&not_in_dm);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_apply_mibs_to_object(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;

    const char* odl = "%define { object Test { string MyParam = 'ADD'; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib_valid"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_int_equal(amxo_parser_apply_mibs(&parser, object, amxd_object_matches_expr), 2);
    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_true(amxd_object_has_mib(object, "test_mib3"));
    assert_true(amxd_object_has_mib(object, "test_mib6"));

    amxd_object_set_value(cstring_t, object, "MyParam", "HELLO");
    assert_int_equal(amxo_parser_apply_mibs(&parser, object, amxd_object_matches_expr), 2);
    assert_false(amxd_object_has_mib(object, "test_mib3"));
    assert_false(amxd_object_has_mib(object, "test_mib6"));
    assert_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_null(amxd_object_get_param_def(object, "Mib6Text"));

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_add_mibs_to_object(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;

    const char* odl = "%define { object Test { string MyParam = 'ADD'; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib_valid"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_int_equal(amxo_parser_add_mibs(&parser, object, amxd_object_matches_expr), 2);
    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_true(amxd_object_has_mib(object, "test_mib3"));
    assert_true(amxd_object_has_mib(object, "test_mib6"));

    amxd_object_set_value(cstring_t, object, "MyParam", "HELLO");
    assert_int_equal(amxo_parser_add_mibs(&parser, object, amxd_object_matches_expr), 0);
    assert_non_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_true(amxd_object_has_mib(object, "test_mib3"));
    assert_true(amxd_object_has_mib(object, "test_mib6"));

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_remove_mibs_from_object(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;

    const char* odl = "%define { object Test { string MyParam = 'ADD'; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib_valid"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_int_equal(amxo_parser_add_mibs(&parser, object, amxd_object_matches_expr), 2);
    assert_null(amxd_object_get_param_def(object, "Mib1Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_non_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_true(amxd_object_has_mib(object, "test_mib3"));
    assert_true(amxd_object_has_mib(object, "test_mib6"));

    amxd_object_set_value(cstring_t, object, "MyParam", "HELLO");
    assert_int_equal(amxo_parser_remove_mibs(&parser, object, amxd_object_matches_expr), 2);
    assert_null(amxd_object_get_param_def(object, "Mib3Text"));
    assert_null(amxd_object_get_param_def(object, "Mib6Text"));
    assert_false(amxd_object_has_mib(object, "test_mib3"));
    assert_false(amxd_object_has_mib(object, "test_mib6"));

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_add_mibs_from_include(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t* object = NULL;

    const char* odl = "%define { object Test { string MyParam = 'ADD'; } } "
        "include \"add_mid.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs"), 0);
    assert_int_equal(amxo_parser_scan_mib_dir(&parser, "./mibs/test_mib_valid"), 0);
    assert_int_equal(amxc_htable_size(&parser.mibs), 5);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    object = amxd_dm_get_object(&dm, "Test");
    assert_non_null(object);

    assert_true(amxd_object_has_mib(object, "test_mib1"));
    assert_non_null(amxd_object_get_param_def(object, "Mib1Text"));

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}
