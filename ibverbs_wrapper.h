/***********************************************************************************
 *
 * Copyright (c) 2014 Ramnath Sai Sagar (ramnath.sagar@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <dirent.h>

#include <dlfcn.h>

#include <gptl.h>

#ifdef HAVE_PAPI
    #include <papi.h>
#endif

#include <infiniband/verbs.h>
//#include <rdma/rdma_verbs.h>

static struct ibv_device** (*real_ibv_get_device_list)(int *) = NULL;
static const char* (*real_ibv_get_device_name)(struct ibv_device *) = NULL;
static struct ibv_context* (*real_ibv_open_device)(struct ibv_device *) = NULL;
static int (*real_ibv_query_device)(struct ibv_context *, struct ibv_device_attr *) = NULL;
static int (*real_ibv_query_gid)(struct ibv_context *, uint8_t, int, union ibv_gid *) = NULL;
static int (*real_ibv_query_port)(struct ibv_context *, uint8_t , struct ibv_port_attr *) = NULL;

static struct ibv_pd* (*real_ibv_alloc_pd)(struct ibv_context *) = NULL;
static struct ibv_mr* (*real_ibv_reg_mr)(struct ibv_pd *, void *, size_t , int) = NULL;
static struct ibv_cq* (*real_ibv_create_cq)(struct ibv_context *, int, void *, struct ibv_comp_channel *, int) = NULL;
static int (*real_ibv_get_cq_event)(struct ibv_comp_channel *, struct ibv_cq **, void **) = NULL;
static int (*real_ibv_req_notity_cq)(struct ibv_cq *, int) = NULL;
static struct ibv_qp* (*real_ibv_create_qp)(struct ibv_pd *, struct ibv_qp_init_attr *) = NULL;
static int (*real_ibv_modify_qp)(struct ibv_qp *, struct ibv_qp_attr *, int) = NULL;
static struct ibv_ah* (*real_ibv_create_ah)(struct ibv_pd *, struct ibv_ah_attr *) = NULL;


static int (*real_ocrdma_post_send)(struct ibv_qp *, struct ibv_send_wr *, struct ibv_send_wr **) = NULL;
static int (*real_ocrdma_post_recv)(struct ibv_qp *, struct ibv_recv_wr *, struct ibv_recv_wr **) = NULL;
static int (*real_ocrdma_poll_cq)(struct ibv_cq *, int, struct ibv_wc *) = NULL;
static int (*real_ocrdma_arm_cq)(struct ibv_cq *, int) = NULL;

static int (*real_mlx4_post_send)(struct ibv_qp *, struct ibv_send_wr *, struct ibv_send_wr **) = NULL;
static int (*real_mlx4_post_recv)(struct ibv_qp *, struct ibv_recv_wr *, struct ibv_recv_wr **) = NULL;
static int (*real_mlx4_poll_cq)(struct ibv_cq *, int , struct ibv_wc *) = NULL;
static int (*real_mlx4_arm_cq)(struct ibv_cq *, int/*enum ib_cq_notify_flags*/) = NULL;


static int (*real_ibv_destroy_ah)(struct ibv_ah *) = NULL;
static int (*real_ibv_destroy_qp)(struct ibv_qp *) = NULL;
static int (*real_ibv_dereg_mr)(struct ibv_mr *) = NULL;
static int (*real_ibv_destroy_cq)(struct ibv_cq *) = NULL;
static int (*real_ibv_dealloc_pd)(struct ibv_pd *) = NULL;

static void (*real_ibv_free_device_list)(struct ibv_device **) = NULL;
static int (*real_ibv_close_device)(struct ibv_context *) = NULL;

