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
#include <amxp/amxp_slot.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_parameter.h>
#include <amxd/amxd_transaction.h>
#include <amxo/amxo.h>
#include <amxo/amxo_hooks.h>

#include "test_events.h"

#define UNUSED __attribute__((unused))

uint32_t event_counter = 0;

static void _print_event(const char * const sig_name,
                         UNUSED const amxc_var_t * const data,
                         UNUSED void * const priv) {

    printf("Event received %s\n", sig_name);
    event_counter++;
}

void test_event_subscription(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_trans_t transaction;

    const char *odl = "%define {"
        "    object Test[] { string text = \"Hallo\"; }"
        "}"
        "%populate {"
        "    on event regexp(\".*\") call print_event;"
        "}";
    const char *odl_2 = "%populate {"
        "     on event regexp(\"dm:instance-.*\") of \"Test\" call print_event;"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "print_event", AMXO_FUNC(_print_event));
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 1);

    amxp_slot_disconnect_all(_print_event);
    assert_int_equal(amxo_parser_parse_string(&parser, odl_2, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxd_trans_init(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_add_inst(&transaction, 0, NULL);
    amxd_trans_select_pathf(&transaction, "..");
    amxd_trans_add_inst(&transaction, 0, NULL);
    amxd_trans_select_pathf(&transaction, "..");
    amxd_trans_add_inst(&transaction, 0, NULL);
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);
    amxd_trans_clean(&transaction);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 3);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_event_subscription_filter(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_trans_t transaction;

    const char *odl =
        "%define {\n"
        "    object Test { string text = \"Hallo\"; }\n"
        "}\n"
        "%populate {\n"
        "    on event regexp(\".*\") call print_event \n"
        "        filter 'object == \"Test\" && parameters.text.from == \"Hallo\"';\n"
        "}\n";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "print_event", AMXO_FUNC(_print_event));
    printf("%s\n", odl);
    fflush(stdout);
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 0);

    amxd_trans_init(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Test Text");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Test Text2");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Hallo");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "YAT");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);
    amxd_trans_clean(&transaction);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 2);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_deprecated_event_subscription_write_with_object(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_trans_t transaction;

    const char *odl =
        "%define {"
        "    object Test {"
        "        write with print_event;"
        "        string text = \"Hallo\";"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "print_event", AMXO_FUNC(_print_event));
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 1);

    amxd_trans_init(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Test Text");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Test Text2");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Hallo");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "YAT");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);
    amxd_trans_clean(&transaction);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 4);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_deprecated_event_subscription_write_with_param(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxd_trans_t transaction;

    const char *odl =
        "%define {"
        "    object Test {"
        "        string text {"
        "            write with print_event;"
        "            default \"Hallo\";"
        "        }"
        "        uint32 Other = 123;"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    amxo_resolver_ftab_add(&parser, "print_event", AMXO_FUNC(_print_event));
    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 0);

    amxd_trans_init(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Test Text");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Test Text2");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "text", "Hallo");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);

    amxd_trans_clean(&transaction);
    amxd_trans_select_pathf(&transaction, "Test");
    amxd_trans_set_value(cstring_t, &transaction, "Other", "0xFF");
    assert_int_equal(amxd_trans_apply(&transaction, &dm), 0);
    amxd_trans_clean(&transaction);

    event_counter = 0;
    while(amxp_signal_read() == 0) {
    }
    assert_int_equal(event_counter, 3);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_subscription_warns_if_function_not_resolved(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    const char *odl = "%define {"
        "    object Test[] { string text = \"Hallo\"; }"
        "}"
        "%populate {"
        "    on event regexp(\".*\") call print_event;"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_deprecated_subscription_warns_if_function_not_resolved(UNUSED void **state) {
    amxd_dm_t dm;
    amxo_parser_t parser;

    const char *odl =
        "%define {"
        "    object Test {"
        "        string text {"
        "            write with print_event;"
        "            default \"Hallo\";"
        "        }"
        "        uint32 Other = 123;"
        "    }"
        "}";

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    assert_int_equal(amxo_parser_parse_string(&parser, odl, amxd_dm_get_root(&dm)), 0);
    assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}