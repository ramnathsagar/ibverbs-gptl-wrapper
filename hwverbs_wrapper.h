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

static void  getsymbols(char *so_name);
static char *getpath(char *buffer);
static void load_driver(char *dname);
void read_device_specific_symbols();

//OCRDMA
static int (*real_ocrdma_post_send)(struct ibv_qp *, struct ibv_send_wr *, struct ibv_send_wr **) = NULL;
static int (*real_ocrdma_post_recv)(struct ibv_qp *, struct ibv_recv_wr *, struct ibv_recv_wr **) = NULL;
static int (*real_ocrdma_poll_cq)(struct ibv_cq *, int, struct ibv_wc *) = NULL;
static int (*real_ocrdma_arm_cq)(struct ibv_cq *, int) = NULL;

//MLX
static int (*real_mlx4_post_send)(struct ibv_qp *, struct ibv_send_wr *, struct ibv_send_wr **) = NULL;
static int (*real_mlx4_post_recv)(struct ibv_qp *, struct ibv_recv_wr *, struct ibv_recv_wr **) = NULL;
static int (*real_mlx4_poll_cq)(struct ibv_cq *, int , struct ibv_wc *) = NULL;
static int (*real_mlx4_arm_cq)(struct ibv_cq *, int/*enum ib_cq_notify_flags*/) = NULL;

//.......


