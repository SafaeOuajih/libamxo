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

#include "test_config_section.h"

#include <amxc/amxc_macros.h>
void test_parsing_array(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odls[] = {
        "%config { MyOption = [ 1, 2, 3 ]; }",
        "%config { MyOption = [ \"1\", \"2\", \"3\" ]; }",
        "%config { MyOption = [ word1, word2, word3 ]; }",
        "%config { MyOption = [ true, false, true ]; }",
        "%config { MyOption = [  ]; }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        amxc_var_t* option = NULL;
        const amxc_llist_t* list = NULL;
        assert_int_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
        option = amxo_parser_get_config(&parser, "MyOption");
        assert_ptr_not_equal(option, NULL);
        assert_int_equal(amxc_var_type_of(option), AMXC_VAR_ID_LIST);
        list = amxc_var_constcast(amxc_llist_t, option);
        assert_ptr_not_equal(list, NULL);
        switch(i) {
        case 0:
            assert_int_equal(amxc_llist_size(list), 3);
            amxc_llist_for_each(it, list) {
                amxc_var_t* item = amxc_var_from_llist_it(it);
                assert_int_equal(amxc_var_type_of(item), AMXC_VAR_ID_INT64);
            }
            break;
        case 1:
        case 2:
            assert_int_equal(amxc_llist_size(list), 3);
            amxc_llist_for_each(it, list) {
                amxc_var_t* item = amxc_var_from_llist_it(it);
                assert_int_equal(amxc_var_type_of(item), AMXC_VAR_ID_CSTRING);
            }
            break;
        case 3:
            assert_int_equal(amxc_llist_size(list), 3);
            amxc_llist_for_each(it, list) {
                amxc_var_t* item = amxc_var_from_llist_it(it);
                assert_int_equal(amxc_var_type_of(item), AMXC_VAR_ID_BOOL);
            }
            break;
        case 4:
            assert_true(amxc_llist_is_empty(list));
            break;
        }
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_parsing_key_value_pairs(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odls[] = {
        "%config { MyOption = { Key1 = 1, Key2 = 2, Key3 = 3 }; }",
        "%config { MyOption = { Key1 = \"1\", Key2 = \"2\", Key3 = \"3\" }; }",
        "%config { MyOption = { Key1 = word1, Key2 = word2, Key3 = word3 }; }",
        "%config { MyOption = { Key1 = true, Key2 = false, Key3 = true }; }",
        "%config { MyOption = {  }; }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        amxc_var_t* option = NULL;
        const amxc_htable_t* table = NULL;
        assert_int_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
        option = amxo_parser_get_config(&parser, "MyOption");
        assert_ptr_not_equal(option, NULL);
        assert_int_equal(amxc_var_type_of(option), AMXC_VAR_ID_HTABLE);
        table = amxc_var_constcast(amxc_htable_t, option);
        assert_ptr_not_equal(table, NULL);
        switch(i) {
        case 0:
            assert_int_equal(amxc_htable_size(table), 3);
            amxc_htable_for_each(it, table) {
                amxc_var_t* item = amxc_var_from_htable_it(it);
                assert_int_equal(amxc_var_type_of(item), AMXC_VAR_ID_INT64);
            }
            break;
        case 1:
        case 2:
            assert_int_equal(amxc_htable_size(table), 3);
            amxc_htable_for_each(it, table) {
                amxc_var_t* item = amxc_var_from_htable_it(it);
                assert_int_equal(amxc_var_type_of(item), AMXC_VAR_ID_CSTRING);
            }
            break;
        case 3:
            assert_int_equal(amxc_htable_size(table), 3);
            amxc_htable_for_each(it, table) {
                amxc_var_t* item = amxc_var_from_htable_it(it);
                assert_int_equal(amxc_var_type_of(item), AMXC_VAR_ID_BOOL);
            }
            break;
        case 4:
            assert_true(amxc_htable_is_empty(table));
            break;
        }
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_global_setting_are_made_available_in_main_odl(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t* setting = NULL;
    const char* odls[] = {
        "%config { Key1 = 1; Key2 = 2; Key3 = 3; } include \"global_config.odl\";",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odls[0], amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    amxc_var_dump(&parser.config, STDOUT_FILENO);

    setting = amxo_parser_get_config(&parser, "local_setting");
    assert_ptr_equal(setting, NULL);
    assert_true(amxc_var_is_null(setting));
    amxc_var_delete(&setting);

    setting = amxo_parser_get_config(&parser, "Key1");
    assert_ptr_not_equal(setting, NULL);
    assert_false(amxc_var_is_null(setting));
    amxc_var_dump(setting, STDOUT_FILENO);
    assert_int_equal(amxc_var_dyncast(uint32_t, setting), 1);

    setting = amxo_parser_get_config(&parser, "global_setting");
    assert_ptr_not_equal(setting, NULL);
    assert_false(amxc_var_is_null(setting));
    assert_string_equal(amxc_var_constcast(cstring_t, setting), "global test");

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_set_configuration_using_path(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t* setting = NULL;
    const char* odls[] = {
        "%config { MyOption.SubOption.Test = \"Hallo\"; MyOption.SubOption.Value = 1; }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odls[0], amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    amxc_var_dump(&parser.config, STDOUT_FILENO);

    setting = amxo_parser_get_config(&parser, "MyOption");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_HTABLE);

    setting = amxo_parser_get_config(&parser, "MyOption.SubOption");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_HTABLE);

    setting = amxo_parser_get_config(&parser, "MyOption.SubOption.Test");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_CSTRING);

    setting = amxo_parser_get_config(&parser, "MyOption.SubOption.Value");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_INT64);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_set_configuration_data_using_path(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t* setting = NULL;
    const char* odls[] = {
        "%config { MyOption = { SubOption.Test = \"Hallo\", SubOption.Value = 1 }; }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odls[0], amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    amxc_var_dump(&parser.config, STDOUT_FILENO);

    setting = amxo_parser_get_config(&parser, "MyOption");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_HTABLE);

    setting = amxo_parser_get_config(&parser, "MyOption.SubOption");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_HTABLE);

    setting = amxo_parser_get_config(&parser, "MyOption.SubOption.Test");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_CSTRING);

    setting = amxo_parser_get_config(&parser, "MyOption.SubOption.Value");
    assert_non_null(setting);
    assert_int_equal(amxc_var_type_of(setting), AMXC_VAR_ID_INT64);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}
