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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/resource.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_object_event.h>
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"
#include "amxo_parser.tab.h"

static ssize_t amxo_parser_string_reader(amxo_parser_t* parser,
                                         char* buf,
                                         size_t max_size) {
    ssize_t result = 0;
    result = amxc_rbuffer_read(&parser->rbuffer, (char*) buf, max_size);
    errno = 0;

    return result;
}

static int amxo_parser_parse_fd_internal(amxo_parser_t* parser,
                                         int fd,
                                         amxd_object_t* object) {
    int retval = -1;

    parser->fd = fd;
    parser->object = object;
    parser->reader = amxo_parser_fd_reader;
    parser->status = amxd_status_ok;

    amxo_parser_create_lex(parser);
    retval = yyparse(parser->scanner);
    amxo_parser_destroy_lex(parser);

    parser->fd = -1;

    if(retval == 0) {
        amxc_string_clean(&parser->msg);
    }

    return retval;
}

static void amxo_parser_entry_point_free(amxc_llist_it_t* it) {
    amxo_entry_t* entry = amxc_llist_it_get_data(it, amxo_entry_t, it);
    free(entry);
}

ssize_t PRIVATE amxo_parser_fd_reader(amxo_parser_t* parser,
                                      char* buf,
                                      size_t max_size) {
    ssize_t result = 0;

    errno = 0;

    if(parser->buffer_len > 0) {
        memcpy(buf, parser->buffer, parser->buffer_len);
    }

    result = read(parser->fd, buf + parser->buffer_len, max_size - parser->buffer_len);
    if((result == -1) && (errno != EAGAIN)) {
        printf("Read failed %d\n", errno);
        goto exit;
    }
    errno = 0;
    result += parser->buffer_len;
    parser->buffer_len = 0;
    if(result >= (ssize_t) max_size) {
        while(buf[result - 1] != '\n' && result > 0 && max_size - result <= 128) {
            result--;
        }
        if(max_size - result > 0) {
            memcpy(parser->buffer, buf + result, max_size - result);
            parser->buffer_len = max_size - result;
        }
    }

exit:
    return result;
}

static void amxo_parser_pop_event(event_t* e, amxd_dm_t* dm) {
    amxd_object_t* object = amxd_dm_findf(dm, "%s", e->path);

    when_null(object, exit);

    if(dm->sigmngr.enabled) {
        if(e->id == event_instance_add) {
            amxd_object_send_add_inst(object, false);
        } else if(e->id == event_object_change) {
            amxd_object_send_changed(object, &e->data, false);
        }
    }

exit:
    amxc_llist_it_take(&e->it);
    amxc_var_clean(&e->data);
    free(e->path);
    free(e);
}

static void amxo_parser_send_events(amxo_parser_t* parser, amxd_dm_t* dm) {
    amxc_llist_for_each(it, &parser->event_list) {
        event_t* e = amxc_container_of(it, event_t, it);
        amxo_parser_pop_event(e, dm);
    }
}

int PRIVATE amxo_parser_parse_file_impl(amxo_parser_t* parser,
                                        const char* file_path,
                                        amxd_object_t* object) {
    int retval = -1;
    int fd = -1;

    fd = open(file_path, O_RDONLY);
    if(fd == -1) {
        retval = errno;
        if(errno == ENOENT) {
            amxo_parser_msg(parser, "File not found %s", file_path);
        } else {
            amxo_parser_msg(parser, "File open error 0x%8.8X", errno);
        }
        goto exit;
    }
    parser->file = file_path;
    retval = amxo_parser_parse_fd_internal(parser, fd, object);
    parser->file = NULL;
    close(fd);

exit:
    return retval;
}

void PRIVATE amxo_parser_child_init(amxo_parser_t* parser) {
    when_null(parser, exit);

    parser->fd = -1;
    parser->object = NULL;
    parser->param = NULL;
    parser->func = NULL;
    parser->status = amxd_status_ok;
    parser->resolved_fn = NULL;
    parser->resolvers = NULL;
    parser->include_stack = NULL;
    parser->hooks = NULL;
    parser->entry_points = NULL;
    parser->connections = NULL;
    parser->listeners = NULL;
    parser->data = NULL;
    parser->post_includes = NULL;
    parser->parent = NULL;
    parser->resolved_fn_name = NULL;
    parser->buffer_len = 0;

    memset(parser->buffer, 0, 128);
    amxc_rbuffer_init(&parser->rbuffer, 0);
    amxc_string_init(&parser->msg, 0);
    amxc_astack_init(&parser->object_stack);
    amxc_llist_init(&parser->event_list);
    amxc_var_init(&parser->config);
    amxc_llist_init(&parser->global_config);
    amxc_htable_init(&parser->mibs, 5);
    amxc_llist_init(&parser->function_names);

    parser->file = "<unknown>";

exit:
    return;
}

int amxo_parser_init(amxo_parser_t* parser) {
    int retval = -1;
    amxc_var_t* inc_dirs = NULL;
    when_null(parser, exit);

    amxo_parser_child_init(parser);

    amxc_var_set_type(&parser->config, AMXC_VAR_ID_HTABLE);
    inc_dirs = amxc_var_add_key(amxc_llist_t, &parser->config, "include-dirs", NULL);
    amxc_var_add(cstring_t, inc_dirs, ".");

    amxo_parser_init_resolvers(parser);

    retval = 0;

exit:
    return retval;
}

void amxo_parser_clean(amxo_parser_t* parser) {
    when_null(parser, exit);

    parser->fd = -1;
    parser->object = NULL;
    parser->param = NULL;
    parser->func = NULL;
    parser->status = amxd_status_ok;
    parser->resolved_fn = NULL;

    amxc_rbuffer_clean(&parser->rbuffer);
    amxc_string_clean(&parser->msg);
    amxc_astack_clean(&parser->object_stack, NULL);
    amxc_llist_clean(&parser->event_list, amxo_parser_free_event);
    amxc_llist_clean(&parser->global_config, amxc_string_list_it_free);

    amxo_parser_clean_resolvers(parser);
    if(parser->resolvers != NULL) {
        amxc_htable_delete(&parser->resolvers, NULL);
    }

    amxc_llist_delete(&parser->hooks, NULL);
    amxc_var_clean(&parser->config);
    amxc_var_delete(&parser->include_stack);
    amxc_llist_delete(&parser->entry_points, amxo_parser_entry_point_free);
    amxc_llist_delete(&parser->connections, amxo_parser_connection_free);
    amxc_llist_delete(&parser->listeners, amxo_parser_connection_free);
    amxc_llist_clean(&parser->function_names, amxc_string_list_it_free);
    amxc_htable_clean(&parser->mibs, amxo_parser_del_mib_info);
    amxc_var_delete(&parser->post_includes);

exit:
    return;
}

int amxo_parser_new(amxo_parser_t** parser) {
    int retval = -1;
    when_null(parser, exit);

    *parser = (amxo_parser_t*) calloc(1, sizeof(amxo_parser_t));
    when_null((*parser), exit);

    retval = amxo_parser_init(*parser);

exit:
    if((retval != 0) && (*parser != NULL)) {
        amxo_parser_clean(*parser);
        free(*parser);
        *parser = NULL;
    }
    return retval;
}

void amxo_parser_delete(amxo_parser_t** parser) {
    when_null(parser, exit);
    when_null(*parser, exit);

    amxo_parser_clean(*parser);
    free(*parser);
    *parser = NULL;

exit:
    return;
}

int amxo_parser_parse_fd(amxo_parser_t* parser,
                         int fd,
                         amxd_object_t* object) {
    int retval = -1;
    struct rlimit nofile = { 0, 0 };
    when_null(parser, exit);
    when_null(object, exit);

    when_failed(getrlimit(RLIMIT_NOFILE, &nofile), exit);

    when_true(fd < 0 || (rlim_t) llabs(fd) > nofile.rlim_max, exit);
    when_failed(fcntl((int) llabs(fd), F_GETFD), exit);

    amxo_hooks_start(parser);
    retval = amxo_parser_parse_fd_internal(parser, fd, object);
    amxc_llist_clean(&parser->global_config, amxc_string_list_it_free);
    amxo_resolver_import_clean(parser, NULL);

    amxo_hooks_end(parser);

    amxo_parser_send_events(parser, amxd_object_get_dm(object));

exit:
    return retval;
}

int amxo_parser_parse_file(amxo_parser_t* parser,
                           const char* file_path,
                           amxd_object_t* object) {
    int retval = -1;
    char* current_wd = getcwd(NULL, 0);
    char* real_path = NULL;
    char* dir_name = NULL;
    amxc_string_t res_file_path;
    amxc_string_init(&res_file_path, 0);

    when_null(parser, exit);
    when_str_empty(file_path, exit);
    when_null(object, exit);

    if(amxc_string_set_resolved(&res_file_path, file_path, &parser->config) > 0) {
        file_path = amxc_string_get(&res_file_path, 0);
    }

    real_path = realpath(file_path, NULL);
    if(real_path != NULL) {
        dir_name = dirname(real_path);
        when_true(chdir(dir_name) == -1, exit);
        dir_name[strlen(dir_name)] = '/';
    }

    parser->file = (real_path == NULL) ? file_path : real_path;
    amxo_hooks_start(parser);
    retval = amxo_parser_parse_file_impl(parser,
                                         real_path == NULL ? file_path : real_path,
                                         object);
    amxc_llist_clean(&parser->global_config, amxc_string_list_it_free);
    amxo_resolver_import_clean(parser, NULL);
    amxo_hooks_end(parser);

    amxo_parser_send_events(parser, amxd_object_get_dm(object));

    if(real_path != NULL) {
        when_true(chdir(current_wd) == -1, exit);
    }

exit:
    amxc_string_clean(&res_file_path);
    free(current_wd);
    free(real_path);
    return retval;
}

int amxo_parser_parse_string(amxo_parser_t* parser,
                             const char* text,
                             amxd_object_t* object) {
    int retval = -1;
    when_null(parser, exit);
    when_null(object, exit);
    when_str_empty(text, exit);

    amxc_rbuffer_write(&parser->rbuffer, text, strlen(text));
    parser->object = object;
    parser->reader = amxo_parser_string_reader;
    parser->status = amxd_status_ok;

    amxo_hooks_start(parser);
    amxo_parser_create_lex(parser);
    retval = yyparse(parser->scanner);
    amxo_parser_destroy_lex(parser);
    amxo_hooks_end(parser);

    amxo_parser_send_events(parser, amxd_object_get_dm(object));

    amxc_rbuffer_clean(&parser->rbuffer);
    parser->object = NULL;

exit:
    return retval;
}

amxc_var_t* amxo_parser_claim_config(amxo_parser_t* parser,
                                     const char* path) {
    amxc_var_t* retval = NULL;
    when_null(parser, exit);
    when_str_empty(path, exit);

    retval = amxo_parser_get_config(parser, path);
    if(retval == NULL) {
        // add a NULL variant
        amxc_var_t dummy;
        amxc_var_init(&dummy);
        amxo_parser_set_config(parser, path, &dummy);
        retval = amxo_parser_get_config(parser, path);
        amxc_var_clean(&dummy);
    }

exit:
    return retval;
}

amxc_var_t* amxo_parser_get_config(amxo_parser_t* parser,
                                   const char* path) {
    amxc_var_t* retval = NULL;
    when_null(parser, exit);
    when_str_empty(path, exit);

    retval = amxc_var_get_path(&parser->config, path, AMXC_VAR_FLAG_DEFAULT);

exit:
    return retval;
}

int amxo_parser_set_config(amxo_parser_t* parser,
                           const char* path,
                           amxc_var_t* value) {
    int retval = 0;
    when_null(parser, exit);
    when_null(value, exit);
    when_str_empty(path, exit);

    retval = amxc_var_set_path(&parser->config,
                               path,
                               value,
                               AMXC_VAR_FLAG_UPDATE | AMXC_VAR_FLAG_COPY | AMXC_VAR_FLAG_AUTO_ADD);

exit:
    return retval;
}

int amxo_parser_add_entry_point(amxo_parser_t* parser,
                                amxo_entry_point_t fn) {
    int retval = -1;
    amxo_entry_t* ep = NULL;
    when_null(parser, exit);
    when_null(fn, exit);

    if(parser->entry_points == NULL) {
        retval = amxc_llist_new(&parser->entry_points);
        when_null(parser->entry_points, exit);
    }

    amxc_llist_for_each(it, parser->entry_points) {
        ep = amxc_llist_it_get_data(it, amxo_entry_t, it);
        if(ep->entry_point == fn) {
            retval = 0;
            goto exit;
        }
    }

    ep = (amxo_entry_t*) calloc(1, sizeof(amxo_entry_t));
    when_null(ep, exit);

    ep->entry_point = fn;
    amxc_llist_append(parser->entry_points, &ep->it);
    retval = 0;

exit:
    if(retval != 0) {
        free(ep);
    }
    return retval;
}

int amxo_parser_invoke_entry_points(amxo_parser_t* parser,
                                    amxd_dm_t* dm,
                                    int reason) {
    int retval = -1;
    int fail_count = 0;
    bool dm_eventing_enabled = true;
    when_null(parser, exit);
    when_null(dm, exit);

    if(parser->entry_points != NULL) {
        amxc_llist_for_each(it, parser->entry_points) {
            amxo_entry_t* ep = amxc_llist_it_get_data(it, amxo_entry_t, it);
            retval = ep->entry_point(reason, dm, parser);
            if(retval != 0) {
                fail_count++;
            }
        }
    }

    retval = fail_count;

    when_true(fail_count > 0, exit);
    when_true(parser->post_includes == NULL, exit);

    while(amxp_signal_read() == 0) {
    }

    dm_eventing_enabled = GET_BOOL(&parser->config, "dm-eventing-enabled");
    amxp_sigmngr_enable(&dm->sigmngr, dm_eventing_enabled);
    amxc_var_for_each(var, parser->post_includes) {
        const char* file = amxc_var_constcast(cstring_t, var);
        if(amxo_parser_parse_file(parser, file, parser->object) != 0) {
            fail_count++;
        }
        amxc_var_delete(&var);
    }
    amxp_sigmngr_enable(&dm->sigmngr, true);

    retval = fail_count;

exit:
    return retval;
}

int amxo_parser_rinvoke_entry_points(amxo_parser_t* parser,
                                     amxd_dm_t* dm,
                                     int reason) {
    int retval = -1;
    int fail_count = 0;
    when_null(parser, exit);
    when_null(dm, exit);

    if(parser->entry_points != NULL) {
        amxc_llist_for_each_reverse(it, parser->entry_points) {
            amxo_entry_t* ep = amxc_llist_it_get_data(it, amxo_entry_t, it);
            retval = ep->entry_point(reason, dm, parser);
            if(retval != 0) {
                fail_count++;
            }
        }
    }

    retval = fail_count;

exit:
    return retval;
}