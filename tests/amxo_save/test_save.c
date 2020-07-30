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
#include <amxo/amxo_save.h>

#include "test_save.h"

#define UNUSED __attribute__((unused))

void test_save_config_array(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_var_t *the_array = NULL;

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    the_array = amxc_var_add_key(amxc_llist_t, &config_options, "MyArray", NULL);
    amxc_var_add(uint32_t, the_array, 100);
    amxc_var_add(int32_t, the_array, -100);
    amxc_var_add(bool, the_array, true);
    amxc_var_add(bool, the_array, false);
    amxc_var_add(cstring_t, the_array, "Text");
    amxc_var_add(csv_string_t, the_array, "Text,text");
    amxc_var_add(ssv_string_t, the_array, "Text text");
    amxc_var_add(uint64_t, the_array, 100);
    amxc_var_add(int64_t, the_array, -100);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    assert_ptr_equal(amxo_parser_get_config(&parser, "MyArray"), NULL);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_config.odl", amxd_dm_get_root(&dm)), 0);
    amxc_var_dump(&parser.config, STDOUT_FILENO);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "MyArray"), NULL);

    unlink("test_config.odl");

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxc_var_clean(&config_options);
}

void test_config_arrays_can_only_contain_primitives(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_var_t *the_array = NULL;

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    the_array = amxc_var_add_key(amxc_llist_t, &config_options, "MyArray", NULL);
    amxc_var_add(amxc_htable_t, the_array, NULL);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxc_var_clean(&config_options);
}

void test_save_config_key_value_pairs(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_var_t *the_table = NULL;

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    the_table = amxc_var_add_key(amxc_htable_t, &config_options, "populate_behavior", NULL);
    amxc_var_add_key(cstring_t, the_table, "unknown_parameter", "add");
    amxc_var_add_key(cstring_t, the_table, "duplicate_instance", "update");

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    assert_ptr_equal(amxo_parser_get_config(&parser, "populate_behavior"), NULL);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_config.odl", amxd_dm_get_root(&dm)), 0);
    amxc_var_dump(&parser.config, STDOUT_FILENO);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "populate_behavior"), NULL);

    unlink("test_config.odl");

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxc_var_clean(&config_options);
}

void test_config_tables_can_only_contain_primitives(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_var_t *the_table = NULL;

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    the_table = amxc_var_add_key(amxc_htable_t, &config_options, "populate_behavior", NULL);
    amxc_var_add_key(amxc_htable_t, the_table, "some_key", NULL);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxc_var_clean(&config_options);
}

void test_config_tables_keys_must_be_conform_naming_convention(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_var_t *the_table = NULL;

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    the_table = amxc_var_add_key(amxc_htable_t, &config_options, "populate_behavior", NULL);
    amxc_var_add_key(cstring_t, the_table, "%some_key", "value");

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxc_var_clean(&config_options);
}

void test_save_config_values(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_ts_t now;

    amxc_ts_now(&now);

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(bool, &config_options, "enable_auto_detect", true);
    amxc_var_add_key(uint32_t, &config_options, "iteratations", 10);
    amxc_var_add_key(amxc_ts_t, &config_options, "time", &now);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    assert_ptr_equal(amxo_parser_get_config(&parser, "enable_auto_detect"), NULL);
    assert_ptr_equal(amxo_parser_get_config(&parser, "iteratations"), NULL);
    assert_ptr_equal(amxo_parser_get_config(&parser, "time"), NULL);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_config.odl", amxd_dm_get_root(&dm)), 0);
    amxc_var_dump(&parser.config, STDOUT_FILENO);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "enable_auto_detect"), NULL);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "iteratations"), NULL);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "time"), NULL);

    unlink("test_config.odl");

    amxc_var_clean(&config_options);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_save_config_fails_when_using_invalid_key_names(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_ts_t now;

    amxc_ts_now(&now);

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(bool, &config_options, "%enable_auto_detect", true);
    amxc_var_add_key(uint32_t, &config_options, "1234AAQ", 10);
    amxc_var_add_key(amxc_ts_t, &config_options, "Test=Value", &now);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    unlink("test_config.odl");

    amxc_var_clean(&config_options);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_save_can_append_config_values(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t config_options;
    amxc_ts_t now;

    amxc_ts_now(&now);

    amxc_var_init(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(bool, &config_options, "enable_auto_detect", true);
    amxc_var_add_key(uint32_t, &config_options, "iteratations", 10);
    amxc_var_add_key(amxc_ts_t, &config_options, "time", &now);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, false), 0);

    amxc_var_clean(&config_options);
    amxc_var_set_type(&config_options, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(bool, &config_options, "fixed_dir", true);
    amxc_var_add_key(uint32_t, &config_options, "max_objects", 10);

    assert_int_equal(amxo_parser_save_config(&parser, "test_config.odl", &config_options, true), 0);

    assert_ptr_equal(amxo_parser_get_config(&parser, "enable_auto_detect"), NULL);
    assert_ptr_equal(amxo_parser_get_config(&parser, "iteratations"), NULL);
    assert_ptr_equal(amxo_parser_get_config(&parser, "time"), NULL);
    assert_ptr_equal(amxo_parser_get_config(&parser, "fixed_dir"), NULL);
    assert_ptr_equal(amxo_parser_get_config(&parser, "max_objects"), NULL);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_config.odl", amxd_dm_get_root(&dm)), 0);
    amxc_var_dump(&parser.config, STDOUT_FILENO);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "enable_auto_detect"), NULL);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "iteratations"), NULL);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "time"), NULL);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "fixed_dir"), NULL);
    assert_ptr_not_equal(amxo_parser_get_config(&parser, "max_objects"), NULL);

    unlink("test_config.odl");

    amxc_var_clean(&config_options);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_save_object(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t *object = NULL;
    amxd_object_t *instance = NULL;
    amxc_var_t values;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);
    amxc_var_init(&values);
    amxc_var_set_type(&values, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(cstring_t, &values, "Text", "Key3");

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    object = amxd_dm_findf(&dm, "MyRootObject.ChildObject.TemplateObject");
    assert_ptr_not_equal(object, NULL);
    assert_int_equal(amxd_object_add_instance(&instance, object, NULL, 3, &values), 0);

    assert_int_equal(amxo_parser_save_object(&parser, "test_save.odl", object, false), 0);
    amxd_dm_clean(&dm);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    object = amxd_dm_findf(&dm, "MyRootObject.ChildObject.TemplateObject.3");
    assert_ptr_not_equal(object, NULL);

    unlink("test_save.odl");
    amxc_var_clean(&values);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_save_from_root(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_save_object(&parser, "test_save.odl", amxd_dm_get_root(&dm), false), 0);
    amxd_dm_clean(&dm);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);

    unlink("test_save.odl");
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_can_save_instance(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t *object = NULL;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    object = amxd_dm_findf(&dm, "MyRootObject.ChildObject.TemplateObject.1");
    assert_int_equal(amxo_parser_save_object(&parser, "test_save.odl", object, false), 0);
    amxd_dm_clean(&dm);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    object = amxd_dm_findf(&dm, "MyRootObject.ChildObject.TemplateObject.1");
    assert_ptr_not_equal(object, NULL);
    object = amxd_dm_findf(&dm, "MyRootObject.ChildObject.TemplateObject.2");
    assert_ptr_equal(object, NULL);

    unlink("test_save.odl");
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_save_fails_if_file_can_not_be_opened(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t *object = NULL;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_not_equal(amxo_parser_save_object(&parser, "/fake_dir/test_save.odl", object, false), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_save_fails_if_append_to_none_existing_file(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_object_t *object = NULL;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_not_equal(amxo_parser_save_object(&parser, "none_existing.odl", object, true), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_save_takes_rw_data_path_into_account(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t *rw_data_path = NULL;
    int fd = -1;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    rw_data_path = amxo_parser_claim_config(&parser, "rw_data_path");
    amxc_var_set(cstring_t, rw_data_path, "/tmp");
    assert_int_equal(amxo_parser_parse_file(&parser, "test_main.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_save_object(&parser, "${rw_data_path}/test_save.odl", amxd_dm_get_root(&dm), false), 0);

    fd = open("/tmp/test_save.odl", O_RDONLY);
    assert_int_not_equal(fd, -1);
    close(fd);
    unlink("/tmp/test_save.odl");

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}