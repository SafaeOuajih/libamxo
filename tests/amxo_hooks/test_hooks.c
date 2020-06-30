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

#include "test_hooks.h"

#define UNUSED __attribute__((unused))

static amxc_aqueue_t expected_order;

static void test_hook_start(UNUSED amxo_parser_t *parser) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "start");
}

static void test_hook_end(UNUSED amxo_parser_t *parser) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "end");
}

static void test_hook_start_include(UNUSED amxo_parser_t *parser,
                                    UNUSED const char *file) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "include start");
}

static void test_hook_end_include(UNUSED amxo_parser_t *parser,
                                  UNUSED const char *file) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "include end");
}

static void test_hook_start_section(UNUSED amxo_parser_t *parser,
                                    UNUSED int section_id) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "section start");
}

static void test_hook_end_section(UNUSED amxo_parser_t *parser,
                                  UNUSED int section_id) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "section end");
}

static void test_hook_set_config(UNUSED amxo_parser_t *parser,
                                 UNUSED const char *option,
                                 UNUSED amxc_var_t *value) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "set config");
}

static void test_hook_create_object(UNUSED amxo_parser_t *parser,
                                    UNUSED amxd_object_t *parent,
                                    UNUSED const char *name,
                                    UNUSED int64_t attr_bitmask,
                                    UNUSED amxd_object_type_t type) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "create object");
}

static void test_hook_add_instance(UNUSED amxo_parser_t *parser,
                                   UNUSED amxd_object_t *parent,
                                   UNUSED uint32_t index,
                                   UNUSED const char *name) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "instance add");
}

static void test_hook_select_object(UNUSED amxo_parser_t *parser,
                                    UNUSED amxd_object_t *parent,
                                    UNUSED const char *path) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "select object");
}

static void test_hook_end_object(UNUSED amxo_parser_t *parser,
                                 UNUSED amxd_object_t *object) {
    const char *check = amxc_aqueue_remove(&expected_order);
    assert_ptr_not_equal(check, NULL);
    printf("%s\n", check);
    assert_string_equal(check, "end object");
}

static void test_hook_add_param(UNUSED amxo_parser_t *parser,
                                UNUSED amxd_object_t *object,
                                UNUSED const char *name,
                                UNUSED int64_t attr_bitmask,
                                UNUSED uint32_t type) {
}

static void test_hook_set_param(UNUSED amxo_parser_t *parser,
                                UNUSED amxd_object_t *object,
                                UNUSED amxd_param_t *param,
                                UNUSED amxc_var_t *value) {
}

static void test_hook_end_param(UNUSED amxo_parser_t *parser,
                                UNUSED amxd_object_t *object,
                                UNUSED amxd_param_t *param) {
}

static void test_hook_add_func(UNUSED amxo_parser_t *parser,
                               UNUSED amxd_object_t *object,
                               UNUSED const char *name,
                               UNUSED int64_t attr_bitmask,
                               UNUSED uint32_t type) {
}

static void test_hook_end_func(UNUSED amxo_parser_t *parser,
                               UNUSED amxd_object_t *object,
                               UNUSED amxd_function_t *function) {
}

static void test_hook_add_func_arg(UNUSED amxo_parser_t *parser,
                                   UNUSED amxd_object_t *object,
                                   UNUSED amxd_function_t *func,
                                   UNUSED const char *name,
                                   UNUSED int64_t attr_bitmask,
                                   UNUSED uint32_t type,
                                   UNUSED amxc_var_t *def_value) {
}

static amxo_hooks_t test_hooks = {
    .start = test_hook_start,
    .end = test_hook_end,
    .start_include = test_hook_start_include,
    .end_include = test_hook_end_include,
    .set_config = test_hook_set_config,
    .start_section = test_hook_start_section,
    .end_section = test_hook_end_section,
    .create_object = test_hook_create_object,
    .add_instance = test_hook_add_instance,
    .select_object = test_hook_select_object,
    .end_object = test_hook_end_object,
    .add_param = test_hook_add_param,
    .set_param = test_hook_set_param,
    .end_param = test_hook_end_param,
    .add_func = test_hook_add_func,
    .end_func = test_hook_end_func,
    .add_func_arg = test_hook_add_func_arg
};

static amxo_hooks_t test_empty_hooks = { };

void test_hooks_are_called(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxc_aqueue_init(&expected_order);
    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxc_aqueue_add(&expected_order, "start");
    amxc_aqueue_add(&expected_order, "section start");
    amxc_aqueue_add(&expected_order, "set config");
    amxc_aqueue_add(&expected_order, "set config");
    amxc_aqueue_add(&expected_order, "set config");
    amxc_aqueue_add(&expected_order, "section end");
    amxc_aqueue_add(&expected_order, "include start");
    amxc_aqueue_add(&expected_order, "section start");
    amxc_aqueue_add(&expected_order, "section end");
    amxc_aqueue_add(&expected_order, "include end");
    amxc_aqueue_add(&expected_order, "section start");
    amxc_aqueue_add(&expected_order, "create object");  // TestObjectRoot
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjectSingelton
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjSingletonAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjectTemplate
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjTemplateAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjectParamTypes
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjectFunctions
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestSingeltonParamAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestTemplateParamAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestSingletonFuncAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestTemplateFuncAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "create object");  //    TestObjectTemplateWithChild
    amxc_aqueue_add(&expected_order, "create object");  //        TemplateChildObject
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "end object");     //    TestObjectTemplateWithChild
    amxc_aqueue_add(&expected_order, "create object");  //    TestChangeAttr
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "end object");     // TestObjectRoot
    amxc_aqueue_add(&expected_order, "section end");
    amxc_aqueue_add(&expected_order, "section start");
    amxc_aqueue_add(&expected_order, "select object");
    amxc_aqueue_add(&expected_order, "instance add");
    amxc_aqueue_add(&expected_order, "select object");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "select object");
    amxc_aqueue_add(&expected_order, "instance add");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "instance add");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "select object");
    amxc_aqueue_add(&expected_order, "end object");
    amxc_aqueue_add(&expected_order, "section end");
    amxc_aqueue_add(&expected_order, "end");

    amxo_parser_set_hooks(&parser, &test_hooks);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxc_aqueue_size(&expected_order), 0);

    amxd_dm_clean(&dm);

    amxo_parser_unset_hooks(&parser, &test_hooks);
    amxc_aqueue_add(&expected_order, "start");
    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxc_aqueue_size(&expected_order), 1);

    amxc_aqueue_clean(&expected_order, NULL);
    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_hooks_empty_hooks(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_parser_set_hooks(&parser, &test_empty_hooks);
    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid.odl", amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_hooks_add_remove_hooks(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxo_parser_t parser2;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);
    amxo_parser_init(&parser2);

    assert_int_equal(amxo_parser_set_hooks(&parser, &test_hooks), 0);
    assert_int_equal(amxo_parser_set_hooks(&parser, &test_empty_hooks), 0);
    assert_int_equal(amxo_parser_unset_hooks(&parser, &test_hooks), 0);
    assert_int_equal(amxo_parser_unset_hooks(&parser, &test_empty_hooks), 0);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid.odl", amxd_dm_get_root(&dm)), 0);

    assert_int_not_equal(amxo_parser_set_hooks(NULL, &test_hooks), 0);
    assert_int_not_equal(amxo_parser_set_hooks(&parser, NULL), 0);
    assert_int_not_equal(amxo_parser_unset_hooks(NULL, &test_hooks), 0);
    assert_int_not_equal(amxo_parser_unset_hooks(&parser, NULL), 0);

    assert_int_equal(amxo_parser_set_hooks(&parser, &test_hooks), 0);
    assert_int_not_equal(amxo_parser_set_hooks(&parser, &test_hooks), 0);
    assert_int_not_equal(amxo_parser_unset_hooks(&parser2, &test_hooks), 0);

    amxo_parser_clean(&parser);
    amxo_parser_clean(&parser2);
    amxd_dm_clean(&dm);
}