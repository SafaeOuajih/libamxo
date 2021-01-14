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

#include "test_valid_odl.h"

#define UNUSED __attribute__((unused))

static void check_options_are_available(amxo_parser_t* parser) {
    amxc_var_t* option = amxo_parser_get_config(parser, "test_option_number");
    assert_ptr_not_equal(option, NULL);
    assert_int_equal(amxc_var_type_of(option), AMXC_VAR_ID_INT64);

    option = amxo_parser_get_config(parser, "test_option_string");
    assert_ptr_not_equal(option, NULL);
    assert_int_equal(amxc_var_type_of(option), AMXC_VAR_ID_CSTRING);

    option = amxo_parser_get_config(parser, "test_option_bool");
    assert_ptr_not_equal(option, NULL);
    assert_int_equal(amxc_var_type_of(option), AMXC_VAR_ID_BOOL);
}

static void check_objects_exist(amxd_dm_t* dm) {
    amxd_object_t* root = amxd_dm_get_root(dm);
    static const char* paths[] = {
        "TestObjectRoot",
        "TestObjectRoot.TestObjectSingelton",
        "TestObjectRoot.TestObjSingletonAttr",
        "TestObjectRoot.TestObjectTemplate",
        "TestObjectRoot.TestObjTemplateAttr",
        "TestObjectRoot.TestObjectParamTypes",
        "TestObjectRoot.TestObjectFunctions",
        "TestObjectRoot.TestSingeltonParamAttr",
        "TestObjectRoot.TestTemplateParamAttr.99",
        "TestObjectRoot.TestTemplateParamAttr.100",
        "TestObjectRoot.TestSingletonFuncAttr",
        "TestObjectRoot.TestTemplateFuncAttr",
        "TestObjectRoot.TestObjectTemplateWithChild",
        "TestObjectRoot.TestObjectTemplateWithChild.TemplateChildObject",
        "TestObjectRoot.TestObjectTemplateWithChild.Name1",
        "TestObjectRoot.TestObjectTemplateWithChild.Name1.TemplateChildObject",
        "TestObjectRoot.TestObjectTemplateWithChild.Name2",
        "TestObjectRoot.TestObjectTemplateWithChild.Name2.MibObject",
        "TestObjectRoot.TestObjectTemplateWithChild.Name2.TemplateChildObject",
        "TestObjectRoot.ExtendWithTestMib",
        "TestObjectRoot.ExtendWithTestMib.MibObject",
        NULL
    };

    for(int i = 0; paths[i] != NULL; i++) {
        printf("Checking %s exists\n", paths[i]);
        assert_ptr_not_equal(amxd_object_findf(root, paths[i]), NULL);
    }
}

static void check_parameters_exist(amxd_dm_t* dm) {
    amxd_object_t* root = amxd_dm_get_root(dm);
    amxd_object_t* object = NULL;
    amxd_param_t* param = NULL;
    static const char* types_params[] = {
        "Param1",
        "Param2",
        "Param3",
        "Param4",
        "Param5",
        "Param6",
        "Param7",
        "Param8",
        "Param9",
        "Param10",
        "Param11",
        "Param12",
        "Param13",
        NULL
    };

    object = amxd_object_findf(root, "TestObjectRoot.TestObjectParamTypes");
    assert_int_equal(amxd_object_get_param_count(object, amxd_dm_access_protected), 13);
    for(int i = 0; types_params[i] != NULL; i++) {
        assert_ptr_not_equal(amxd_object_get_param_def(object, types_params[i]), NULL);
    }

    object = amxd_object_findf(root, "TestObjectRoot.TestSingeltonParamAttr");
    assert_int_equal(amxd_object_get_param_count(object, amxd_dm_access_private), 1);
    param = amxd_object_get_param_def(object, "Param1");
    assert_ptr_not_equal(param, NULL);
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_private));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_read_only));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_variable));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_instance));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_template));

    object = amxd_object_findf(root, "TestObjectRoot.TestTemplateParamAttr");
    assert_int_equal(amxd_object_get_param_count(object, amxd_dm_access_private), 3);
    param = amxd_object_get_param_def(object, "Param1");
    assert_ptr_not_equal(param, NULL);
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_private));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_read_only));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_variable));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_instance));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_template));
    param = amxd_object_get_param_def(object, "Param2");
    assert_ptr_not_equal(param, NULL);
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_private));
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_read_only));
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_variable));
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_instance));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_template));
    param = amxd_object_get_param_def(object, "Param3");
    assert_ptr_not_equal(param, NULL);
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_private));
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_read_only));
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_variable));
    assert_true(amxd_param_is_attr_set(param, amxd_pattr_instance));
    assert_false(amxd_param_is_attr_set(param, amxd_pattr_template));

    object = amxd_object_findf(root, "TestObjectRoot");
    assert_int_equal(amxd_object_get_param_count(object, amxd_dm_access_private), 2);
    param = amxd_object_get_param_def(object, "TestObjectTemplateWithChildCounter");
    assert_ptr_not_equal(param, NULL);
    object = amxd_object_findf(root, "TestObjectRoot.TestObjectTemplateWithChild.TemplateChildObject");
    assert_int_equal(amxd_object_get_param_count(object, amxd_dm_access_private), 1);
    param = amxd_object_get_param_def(object, "Param1");
    assert_ptr_not_equal(param, NULL);
    object = amxd_object_findf(root, "TestObjectRoot.TestObjectTemplateWithChild.1.TemplateChildObject");
    assert_int_equal(amxd_object_get_param_count(object, amxd_dm_access_private), 1);
    param = amxd_object_get_param_def(object, "Param1");
    assert_ptr_not_equal(param, NULL);
}

static void check_functions_exist(amxd_dm_t* dm) {
    amxd_object_t* root = amxd_dm_get_root(dm);
    amxd_object_t* object = NULL;
    amxd_function_t* func = NULL;
    static const char* obj_funcs[] = {
        "func1",
        "func2",
        "func3",
        "func4",
        "func5",
        "func6",
        "func7",
        "func8",
        "func9",
        "func10",
        "func11",
        "func12",
        "func13",
        "func14",
        "func15",
        "func16",
        NULL
    };

    object = amxd_object_findf(root, "TestObjectRoot.TestObjectFunctions");
    assert_int_equal(amxd_object_get_function_count(object, amxd_dm_access_private), 24);
    for(int i = 0; obj_funcs[i] != NULL; i++) {
        assert_ptr_not_equal(amxd_object_get_function(object, obj_funcs[i]), NULL);
    }

    object = amxd_object_findf(root, "TestObjectRoot.TestSingletonFuncAttr");
    assert_int_equal(amxd_object_get_function_count(object, amxd_dm_access_private), 10);
    func = amxd_object_get_function(object, "TestFunc1");
    assert_true(amxd_function_is_attr_set(func, amxd_fattr_template));
    assert_true(amxd_function_is_attr_set(func, amxd_fattr_instance));
    assert_true(amxd_function_is_attr_set(func, amxd_fattr_private));

    func = amxd_object_get_function(object, "TestFunc2");
    assert_false(amxd_function_is_attr_set(func, amxd_fattr_template));
    assert_false(amxd_function_is_attr_set(func, amxd_fattr_instance));
    assert_false(amxd_function_is_attr_set(func, amxd_fattr_private));
}

static void check_mib_extions_work(amxd_dm_t* dm) {
    amxd_object_t* object = amxd_dm_findf(dm, "TestObjectRoot.ExtendWithTestMib");

    assert_ptr_not_equal(object, NULL);
    assert_true(amxd_object_has_mib(object, "TestMib"));
    assert_ptr_not_equal(amxd_object_get_param_def(object, "mibparam"), NULL);
    assert_ptr_not_equal(amxd_object_get_function(object, "mibfunc"), NULL);
}

void test_can_parse_odl_file(UNUSED void** state) {
    amxd_dm_t dm;
    amxc_var_t* lib_dirs = NULL;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    lib_dirs = amxo_parser_get_config(&parser, "import-dirs");
    amxc_var_add(cstring_t, lib_dirs, "../test_plugin/");
    amxc_var_dump(&parser.config, STDOUT_FILENO);

    assert_int_equal(amxo_parser_parse_file(&parser, "test_valid.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    check_options_are_available(&parser);
    check_objects_exist(&dm);
    check_parameters_exist(&dm);
    check_functions_exist(&dm);
    check_mib_extions_work(&dm);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_can_parse_empty_file(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_file(&parser, "empty.odl", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}


void test_can_parse_fd(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t* lib_dirs = NULL;
    int fd = open("test_valid.odl", O_RDONLY);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    lib_dirs = amxo_parser_get_config(&parser, "import-dirs");
    amxc_var_add(cstring_t, lib_dirs, "../test_plugin/");
    amxc_var_dump(&parser.config, STDOUT_FILENO);

    assert_int_equal(amxo_parser_parse_fd(&parser, fd, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    close(fd);

    check_options_are_available(&parser);
    check_objects_exist(&dm);
    check_parameters_exist(&dm);
    check_functions_exist(&dm);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
    amxo_resolver_import_close_all();
}

void test_can_parse_string(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, "%config {} %define{} %populate { } %config { }", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
    assert_int_equal(amxo_parser_parse_string(&parser, " ", amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_duplicate_func_name(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odl = "%define { object Test { void F1(); void F2(); void F1(); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}
