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

#include "test_include.h"

#include <amxc/amxc_macros.h>
void test_can_include_empty_file(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "include \"empty.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_include_between_sections(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl =
        "include \"empty.odl\";" \
        "%config { }" \
        "include \"empty.odl\";" \
        "%define { }" \
        "include \"empty.odl\";" \
        "%populate { }" \
        "include \"empty.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_none_existing_include_file(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "include \"does_not_exists.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_file_not_found);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_none_existing_optional_include_file(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "#include \"does_not_exists.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_recursive_include_detection(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "include \"test1.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_unknown_error);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_recursive_dir_include_detection(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "include \"./recursive_dir/\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_unknown_error);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_include_absolute_path(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    char* abs_path = realpath("empty.odl", NULL);
    char odl[8192];

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    sprintf(odl, "include \"%s\";", abs_path);
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    sprintf(odl, "include \"/tmp/fake.odl\";");
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_file_not_found);

    free(abs_path);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_post_include(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    char* abs_path = realpath("empty.odl", NULL);
    char odl[8192];

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    sprintf(odl, "&include \"%s\";", abs_path);
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    assert_true(dm.sigmngr.enabled);

    sprintf(odl, "&include \"/tmp/fake.odl\";");
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_file_not_found);

    assert_int_not_equal(amxo_parser_invoke_entry_points(&parser, &dm, 0), 0);

    assert_true(dm.sigmngr.enabled);

    free(abs_path);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_include_directory(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "include \"main.odl\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    assert_non_null(amxd_dm_findf(&dm, "MainObject.InstanceObject.default"));
    assert_non_null(amxd_dm_findf(&dm, "MainObject.InstanceObject.1"));
    assert_non_null(amxd_dm_findf(&dm, "MainObject.InstanceObject.2"));
    assert_non_null(amxd_dm_findf(&dm, "MainObject.InstanceObject.3"));

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_empty_directory(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "include \"./empty_dir/\";";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}
