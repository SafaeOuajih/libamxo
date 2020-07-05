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

#include "test_invalid_odl.h"

#define UNUSED __attribute__((unused))

static amxd_status_t test_dummy_action(UNUSED amxd_object_t * const object,
                                       UNUSED amxd_param_t * const param,
                                       UNUSED amxd_action_t reason,
                                       UNUSED const amxc_var_t * const args,
                                       UNUSED amxc_var_t * const retval,
                                       UNUSED void *priv) {
    return amxd_status_ok;
}

static void _print_event(const char * const sig_name,
                         UNUSED const amxc_var_t * const data,
                         UNUSED void * const priv) {

    printf("Event received %s\n", sig_name);
}

void test_invalid_object_attrs(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { %in object Test; }",
        "%define { %out object Test; }",
        "%define { %mandatory object Test; }",
        "%define { %strict object Test; }",
        "%define { %volatile object Test; }",
        "%define { %template object Test; }",
        "%define { %instance object Test; }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_param_attrs(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { object Test { %in string Param; } }",
        "%define { object Test { %out string Param; } }",
        "%define { object Test { %mandatory string Param; } }",
        "%define { object Test { %strict string Param; } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_func_attrs(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { object Test { %read-only void Func(); } }",
        "%define { object Test { %persistent void Func(); } }",
        "%define { object Test { %in void Func(); } }",
        "%define { object Test { %out void Func(); } }",
        "%define { object Test { %mandatory void Func(); } }",
        "%define { object Test { %strict void Func(); } }",
        "%define { object Test { %volatile void Func(); } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_add_inst_on_singelton(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl =
        "%define { object Test; }" \
        "%populate { object Test { instance add(0,\"\"); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_duplicate_inst_index(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl =
        "%define { object Test[]; }" \
        "%populate { object Test { instance add(1,\"\"); instance add(1,\"\"); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_duplicate_inst_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl =
        "%define { object Test[]; }" \
        "%populate { object Test { instance add(0,\"AB\"); instance add(0,\"AB\"); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_inst_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl =
        "%define { object Test[]; }" \
        "%populate { object Test { instance add(0,\"%Text$\"); instance add(0,\"AB\"); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}


void test_duplicate_obj_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test1; object Test2; object Test1; }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_obj_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object \"23&Test1\";  }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_duplicate_param_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { string P1; string P2; string P1; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_duplicate_param_name_with_counter(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { string P1; object TT[] { counted with P1; } } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_param_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { string \"12$P1\"; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_func_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { void \"F$1\"(); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_duplicate_func_arg_name(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { void F1(bool a1, bool a2, bool a1); } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_select_none_existing_obj(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test; } %populate { object NoneExisting; }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_select_none_existing_param(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { string P1;} } %populate { object Test { parameter P2 = \"text\"; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_param_value(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl = "%define { object Test { bool P1;} } %populate { object Test { parameter P1 = \"text\"; } }";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_param_value_validate(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odl =
        "%define {"
        "    object Test {"
        "        uint32 P1 {"
        "            default 50;"
        "            on action validate call check_maximum 10;"
        "        }"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}


void test_invalid_resolvers(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { object Test { void test()<!!>;} }",
        "%define { object Test { void test()<!:!>;} }",
        "%define { object Test { void test()<!:echo!>;} }",
        "%define { object Test { void test()<!invalid:echo!>;} }",
        "%define { object Test { void test()<!more!>;} }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_not_existing_entry_point(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        " import  \"../test_plugin/test_plugin.so\" as test_plugin; %define { entry-point test_plugin.not_existing;  }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_parameter_actions(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { object Test { string Text { on action \"list\" call dummy; } } }",
        "%define { object Test { string Text { on action add-inst call dummy; } } }",
        "%define { object Test { string Text { on action del-inst call dummy; } } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "dummy", AMXO_FUNC(test_dummy_action));

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_not_resolved_action(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { object Test { on action \"list\" call not_existing; } }",
        "%define { object Test { string Text { on action read call no_existing; } } }",
        "%define { object Test { on action write call not_existing; } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_add_not_existing_mib(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { object Test { extend with mib FakeMib; } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_add_mib_with_duplicates(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char *odls[] = {
        "%define { mib TestMib { string Text; } object Test { string Text; extend with mib TestMib; } }",
        "%define { mib TestMib { string Text; } object Test { extend with mib TestMib; string Text; } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_regexp_in_filter(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    const char *odl =
        "%define {\n"
        "    object Test { string text = \"Hallo\"; }\n"
        "}\n"
        "%populate {\n"
        "    on event \".*\" call print_event \n"
        "        filter {\n"
        "            \'object matches \"[(adsads[\"';\n"
        "        }\n"
        "}\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "print_event", AMXO_FUNC(_print_event));
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_key_params(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    const char *odl_1 =
        "%define {\n"
        "    object TestRoot {\n"
        "        object Test[] {\n"
        "            %key string text;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "%populate {\n"
        "   object TestRoot.Test {\n"
        "       instance add();\n"
        "   }\n"
        "}\n";

    const char *odl_2 =
        "%define {\n"
        "    object TestRoot {\n"
        "        object Test[] {\n"
        "            %key string text;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "%populate {\n"
        "   object TestRoot.Test {\n"
        "       instance add() {\n"
        "           parameter text = \"KeyValue\";\n"
        "       }\n"
        "   }\n"
        "}\n";

    const char *odl_3 =
        "%define {\n"
        "    object TestRoot {\n"
        "        object Test[] {\n"
        "            %key string text;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "%populate {\n"
        "   object TestRoot.Test {\n"
        "       instance add(text = \"key1\");\n"
        "       instance add(text = \"key1\");\n"
        "   }\n"
        "}\n";

    const char *odl_4 =
        "%define {\n"
        "    object TestRoot {\n"
        "        object Test[] {\n"
        "            %key string text = \"Hallo\";\n"
        "        }\n"
        "    }\n"
        "}\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    printf("%s\n", odl_1);
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl_1, amxd_dm_get_root(&dm)), 0);
    amxd_dm_clean(&dm);

    printf("%s\n", odl_2);
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl_2, amxd_dm_get_root(&dm)), 0);
    amxd_dm_clean(&dm);

    printf("%s\n", odl_3);
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl_3, amxd_dm_get_root(&dm)), 0);
    amxd_dm_clean(&dm);

    printf("%s\n", odl_4);
    assert_int_not_equal(amxo_parser_parse_string(&parser, odl_4, amxd_dm_get_root(&dm)), 0);
    amxd_dm_clean(&dm);

    amxo_parser_clean(&parser);
}
