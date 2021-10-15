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
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"
#include "amxo_parser.tab.h"

static amxo_connection_t* amxo_connection_get_internal(amxc_llist_t* list,
                                                       int fd) {
    amxo_connection_t* con = NULL;

    amxc_llist_for_each(it, list) {
        con = amxc_llist_it_get_data(it, amxo_connection_t, it);
        if(con->fd == fd) {
            break;
        }
        con = NULL;
    }

    return con;
}

static int amxo_can_add_connection(amxo_parser_t* parser,
                                   int fd,
                                   amxo_con_type_t type) {
    int retval = -1;
    amxo_connection_t* con = NULL;

    if(type == AMXO_LISTEN) {
        if(parser->listeners == NULL) {
            amxc_llist_new(&parser->listeners);
        }
        when_null(parser->listeners, exit);
        con = amxo_connection_get_internal(parser->listeners, fd);
    } else {
        if(parser->connections == NULL) {
            amxc_llist_new(&parser->connections);
        }
        when_null(parser->connections, exit);
        con = amxo_connection_get_internal(parser->connections, fd);
    }

    retval = con == NULL ? 0 : -1;

exit:
    return retval;
}

static int amxo_connection_add_impl(amxo_parser_t* parser,
                                    amxo_connection_t* con,
                                    amxo_fd_read_t reader) {
    int retval = -1;
    amxc_var_t var_fd;

    amxc_var_init(&var_fd);
    amxc_var_set(fd_t, &var_fd, con->fd);

    con->reader = reader;
    if(con->type == AMXO_LISTEN) {
        amxc_llist_append(parser->listeners, &con->it);
        amxp_sigmngr_trigger_signal(NULL, "listen-added", &var_fd);
    } else {
        amxc_llist_append(parser->connections, &con->it);
        amxp_sigmngr_trigger_signal(NULL, "connection-added", &var_fd);
    }

    retval = 0;

    amxc_var_clean(&var_fd);
    return retval;
}

void amxo_parser_connection_free(amxc_llist_it_t* it) {
    amxo_connection_t* con = amxc_llist_it_get_data(it, amxo_connection_t, it);
    free(con->uri);
    free(con);
}

int amxo_connection_add(amxo_parser_t* parser,
                        int fd,
                        amxo_fd_read_t reader,
                        const char* uri,
                        amxo_con_type_t type,
                        void* priv) {
    int retval = -1;
    amxo_connection_t* con = NULL;

    when_null(parser, exit);
    when_null(reader, exit);
    when_true(fd < 0, exit);

    retval = amxo_can_add_connection(parser, fd, type);
    when_failed(retval, exit);

    retval = -1;
    con = (amxo_connection_t*) calloc(1, sizeof(amxo_connection_t));
    when_null(con, exit);

    con->uri = uri == NULL ? NULL : strdup(uri);
    con->fd = fd;
    con->priv = priv;
    con->type = type;

    retval = amxo_connection_add_impl(parser, con, reader);

exit:
    if(retval != 0) {
        free(con);
    }
    return retval;
}

int amxo_connection_wait_write(amxo_parser_t* parser,
                               int fd,
                               amxo_fd_cb_t can_write_cb) {
    int retval = -1;
    amxo_connection_t* con = NULL;
    amxc_var_t var_fd;
    const char* signal = "connection-wait-write";

    amxc_var_init(&var_fd);
    when_null(parser, exit);
    when_true(fd < 0, exit);
    when_null(can_write_cb, exit);

    con = amxo_connection_get_internal(parser->connections, fd);
    when_null(con, exit);

    amxc_var_set(fd_t, &var_fd, fd);
    amxp_sigmngr_trigger_signal(NULL, signal, &var_fd);
    con->can_write = can_write_cb;

    retval = 0;

exit:
    amxc_var_clean(&var_fd);
    return retval;
}

int amxo_connection_remove(amxo_parser_t* parser,
                           int fd) {
    int retval = -1;
    amxo_connection_t* con = NULL;
    amxc_var_t var_fd;
    const char* signal = "connection-deleted";

    amxc_var_init(&var_fd);
    when_null(parser, exit);

    con = amxo_connection_get_internal(parser->connections, fd);
    if(con == NULL) {
        signal = "listen-deleted";
        con = amxo_connection_get_internal(parser->listeners, fd);
    }
    when_null(con, exit);

    amxc_var_set(fd_t, &var_fd, fd);
    amxp_sigmngr_trigger_signal(NULL, signal, &var_fd);
    amxc_llist_it_clean(&con->it, amxo_parser_connection_free);

    if(amxc_llist_is_empty(parser->connections)) {
        amxc_llist_delete(&parser->connections, NULL);
    }

    if(amxc_llist_is_empty(parser->listeners)) {
        amxc_llist_delete(&parser->listeners, NULL);
    }

    retval = 0;

exit:
    amxc_var_clean(&var_fd);
    return retval;
}

amxo_connection_t* amxo_connection_get(amxo_parser_t* parser,
                                       int fd) {
    amxo_connection_t* con = NULL;
    when_null(parser, exit);

    con = amxo_connection_get_internal(parser->connections, fd);
    if(con != NULL) {
        goto exit;
    }
    con = amxo_connection_get_internal(parser->listeners, fd);

exit:
    return con;
}

amxo_connection_t* amxo_connection_get_first(amxo_parser_t* parser,
                                             amxo_con_type_t type) {
    amxo_connection_t* con = NULL;
    when_null(parser, exit);

    amxc_llist_for_each(it, parser->connections) {
        con = amxc_llist_it_get_data(it, amxo_connection_t, it);
        if(con->type == type) {
            break;
        }
        con = NULL;
    }

exit:
    return con;
}

amxo_connection_t* amxo_connection_get_next(amxo_parser_t* parser,
                                            amxo_connection_t* con,
                                            amxo_con_type_t type) {
    amxc_llist_it_t* it = NULL;
    when_null(parser, exit);
    when_null(con, exit);
    when_true(con->it.llist != parser->connections, exit);

    it = amxc_llist_it_get_next(&con->it);
    con = NULL;
    while(it) {
        con = amxc_llist_it_get_data(it, amxo_connection_t, it);
        if(con->type == type) {
            break;
        }
        con = NULL;
        it = amxc_llist_it_get_next(it);
    }

exit:
    return con;
}

int amxo_connection_set_el_data(amxo_parser_t* parser,
                                int fd,
                                void* el_data) {
    int retval = -1;
    amxo_connection_t* con = NULL;
    when_null(parser, exit);

    con = amxo_connection_get_internal(parser->connections, fd);
    if(con != NULL) {
        con = amxo_connection_get_internal(parser->listeners, fd);
    }

    when_null(con, exit);
    con->el_data = el_data;

    retval = 0;

exit:
    return retval;
}
