/***********************************************************************************
Copyright (c) 2014 Ramnath Sai Sagar (ramnath.sagar@gmail.com)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <dlfcn.h>

#include <gptl.h>
#include <papi.h>

#include <infiniband/verbs.h>
#include <rdma/rdma_verbs.h>


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
static int (*real_mlx4_ib_arm_cq)(struct ibv_cq *, int/*enum ib_cq_notify_flags*/) = NULL;


static int (*real_ibv_destroy_ah)(struct ibv_ah *) = NULL;
static int (*real_ibv_destroy_qp)(struct ibv_qp *) = NULL;
static int (*real_ibv_dereg_mr)(struct ibv_mr *) = NULL;
static int (*real_ibv_destroy_cq)(struct ibv_cq *) = NULL;
static int (*real_ibv_dealloc_pd)(struct ibv_pd *) = NULL;

static void (*real_ibv_free_device_list)(struct ibv_device **) = NULL;
static int (*real_ibv_close_device)(struct ibv_context *) = NULL;


static int check = 0;

static void setoptions()
{
	int ret;
	ret = GPTLsetoption (GPTL_IPC, 1);
	ret = GPTLsetoption (PAPI_TOT_INS, 1);
	ret = GPTLsetoption (PAPI_L2_DCM, 1);
	ret = GPTLsetoption (GPTLoverhead, 1);

}

static void initialize_tracing()
{
	int ret;
	//printf("Initializing..\n");
	if (GPTLinitialize()) fprintf(stderr,"GPTLinitializae failed\n");
	//printf("Initialized\n");
}

static void finalize_tracing()
{
	int ret;
	//printf("Finalizing..\n");
	ret = GPTLpr(1);
	ret = GPTLfinalize();
	//printf("Finalized\n");
}

char *finddevice()
{
	char line[128];
	char *driver;
	FILE *file;
	long saveoffset;
	driver = malloc(7*sizeof(char));
	system("ibdev2netdev | grep mlx > tmp.file");
	
	file = fopen("tmp.file","r");
	
	if (file != NULL)
	{
		/*long*/ saveoffset = ftell(file);
		fseek(file, 0, SEEK_END);
	
		if(ftell(file) != 0)
			driver="mlx4";
		else
			driver="ocrdma";
	
		fclose(file);
	}
	else
		perror("File not present\n");
	
	system("rm -rf tmp.file");
	system("ibdev2netdev | grep ocrdma > tmp.file");
	
	file = fopen("tmp.file","r");

	if ( file != NULL)
	{
		saveoffset = ftell(file);
		fseek(file, 0, SEEK_END);

		if (ftell(file) != 0 && (!strcmp(driver,"mlx4")))
			driver="both";
		//else
			

	}
	system("rm -rf tmp.file");
	return driver;
}


void load_driver()
{
	
	void *dldriver;
	char *device = malloc(7*sizeof(char));
	char *so_name;
	device=finddevice();
	printf("The device found is %s\n",device);
	if((!strcmp(device,"ocrdma")) || (!strcmp(device,"both")))
	{
		so_name = "/usr/lib64/libocrdma.so";
		dldriver = dlopen(so_name, RTLD_NOW);
		//dldriver = dlopen("/usr/local/lib/libocrdma-rdmav2.so", RTLD_NOW);

		if (!dldriver) {
			fprintf(stderr, "Warning: couldn't load driver from /usr/lib64/%s\n",device);
			exit(0);
		}
		real_ocrdma_post_send = dlsym(dldriver,"ocrdma_post_send");
		real_ocrdma_post_recv = dlsym(dldriver,"ocrdma_post_recv");
		real_ocrdma_poll_cq   = dlsym(dldriver,"ocrdma_poll_cq");
		real_ocrdma_arm_cq    = dlsym(dldriver,"ocrdma_arm_cq");
		printf("address of send=%p recv=%p poll=%p arm_cq=%p\n",real_ocrdma_post_send,real_ocrdma_post_recv,real_ocrdma_poll_cq,real_ocrdma_arm_cq);

	}
	
	if((!strcmp(device,"mlx4")) || (!strcmp(device,"both")))
	{
		dldriver = dlopen("/usr/local/lib/libmlx4-rdmav2.so", RTLD_NOW);
		if (!dldriver) {
			fprintf(stderr, "Warning: Couldn't load driver /usr/local/lib/%s\n",device);
			exit(0);
		}

		real_mlx4_post_send = dlsym(dldriver,"mlx4_post_send");
		real_mlx4_post_recv = dlsym(dldriver,"mlx4_post_recv");
		real_mlx4_poll_cq   = dlsym(dldriver, "mlx4_poll_cq");
		real_mlx4_ib_arm_cq = dlsym(dldriver, "mlx4_ib_arm_cq");

	        printf("address of send=%p recv=%p poll=%p arm_cq=%p\n",real_mlx4_post_send,real_mlx4_post_recv,real_mlx4_poll_cq,real_mlx4_ib_arm_cq);
	}
}

void readibsymbol()
{
	void *dlhandle;
	dlhandle = dlopen("/usr/lib64/libibverbs.so"/*so_name*/, RTLD_NOW);

	if (!dlhandle) {
		fprintf(stderr, "Warning: couldn't load driver\n");
		exit(0);//goto out;
	}

	real_ibv_get_device_list = dlsym(dlhandle, "ibv_get_device_list");
	real_ibv_get_device_name = dlsym(dlhandle,"ibv_get_device_name");
	real_ibv_open_device = dlsym(dlhandle,"ibv_open_device");
	real_ibv_query_device = dlsym(dlhandle,"ibv_query_device");
	real_ibv_query_gid = dlsym(dlhandle,"ibv_query_gid");
	real_ibv_query_port = dlsym(dlhandle, "ibv_query_port");

	real_ibv_alloc_pd = dlsym(dlhandle,"ibv_alloc_pd");
	real_ibv_reg_mr = dlsym(dlhandle,"ibv_reg_mr");
	real_ibv_create_cq = dlsym(dlhandle,"ibv_create_cq");
	real_ibv_get_cq_event = dlsym(dlhandle,"ibv_get_cq_event");
	real_ibv_req_notity_cq = dlsym(dlhandle,"ibv_req_notity_cq");
	real_ibv_create_qp = dlsym(dlhandle, "ibv_create_qp");
	real_ibv_modify_qp = dlsym(dlhandle, "ibv_modify_qp");
	real_ibv_create_ah = dlsym(dlhandle,"ibv_create_ah");
	

	//real_ibv_destroy_ah = dlsym(dlhandle,"ibv_destroy_ah");
	real_ibv_destroy_qp = dlsym(dlhandle,"ibv_destroy_qp");
	real_ibv_dereg_mr = dlsym(dlhandle,"ibv_dereg_mr");
	real_ibv_destroy_cq = dlsym(dlhandle,"ibv_destroy_cq");
	real_ibv_dealloc_pd = dlsym(dlhandle,"ibv_dealloc_pd");

	real_ibv_free_device_list = dlsym(dlhandle, "ibv_free_device_list");
	real_ibv_close_device = dlsym(dlhandle, "ibv_close_device");

	load_driver();

	//printf("%p , %p and %p\n",real_ocrdma_poll_cq,real_ibv_create_qp,real_ibv_modify_qp);
}

struct ibv_device **ibv_get_device_list(int *num_devices)
{
	struct ibv_device **retval;
		
	if (check == 0) {
		readibsymbol();
		//setoptions();
		initialize_tracing();
		check = 1;
	}

	//printf("get_device_list\n");
	GPTLstart("ibv_get_device_list");

	retval = real_ibv_get_device_list(num_devices);

	//printf("1get_device_list\n");
	GPTLstop("ibv_get_device_list");

	return retval;
}

const char *ibv_get_device_name(struct ibv_device *device)
{
	const char* retval;

	//printf("get_device_name\n");
	GPTLstart("ibv_get_device_name");

	retval = real_ibv_get_device_name(device);

	GPTLstop("ibv_get_device_name");
	//printf("1get_device_name\n");

	return retval;

}

struct ibv_context *ibv_open_device(struct ibv_device *device)
{
	struct ibv_context* retval;
	
	//printf("ibv_open_device\n");
	GPTLstart("ibv_open_device");

	retval = real_ibv_open_device(device);

	GPTLstop("ibv_open_device");
	//printf("1ibv_open_device\n");

	return retval;

}

int ibv_query_device(struct ibv_context *context, struct ibv_device_attr *device_attr)
{
	int retval;

	//printf("ibv_query_device\n");
	GPTLstart("ibv_query_device");

	retval = real_ibv_query_device(context, device_attr);

	GPTLstop("ibv_query_device");
	//printf("1ibv_query_device\n");

return retval;

}

int ibv_query_gid(struct ibv_context *context, uint8_t port_num, int index, union ibv_gid *gid)
{
	int retval;

	//printf("ibv_query_gid\n");
	GPTLstart("ibv_query_gid");

	retval = real_ibv_query_gid(context, port_num, index, gid);

	GPTLstop("ibv_query_gid");
	//printf("1ibv_query_gid\n");

	return retval;

}

/*int ibv_query_port(struct ibv_context *context, uint8_t port_num, struct ibv_port_attr *port_attr)
{
	int retval;

	printf("ibv_query_port\n");
	GPTLstart("ibv_query_port");

	retval = real_ibv_query_port(context, port_num, port_attr);
	
	GPTLstart("ibv_query_port");
	printf("1ibv_query_port\n");
	
	return retval;
}*/


struct ibv_pd *ibv_alloc_pd(struct ibv_context *context)
{
	struct ibv_pd* retval;

	//printf("ibv_alloc_pd\n");
	GPTLstart("ibv_alloc_pd");

	retval = real_ibv_alloc_pd(context);

	GPTLstop("ibv_alloc_pd");
	//printf("1ibv_alloc_pd\n");

	return retval;

}

struct ibv_mr *ibv_reg_mr(struct ibv_pd *pd, void *addr, size_t length, int access)
{
	struct ibv_mr* retval;
	
	//printf("ibv_reg_mr\n");
	GPTLstart("ibv_reg_mr");

	retval = real_ibv_reg_mr(pd, addr, length, access);

	GPTLstop("ibv_reg_mr");
	//printf("1ibv_reg_mr\n");

	return retval;

}

struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe, void *cq_context, struct ibv_comp_channel *channel, int comp_vector)
{
	struct ibv_cq* retval;
	
	//printf("ibv_create_cq\n");
	GPTLstart("ibv_create_cq");

	retval = real_ibv_create_cq(context, cqe, cq_context, channel, comp_vector);

	GPTLstop("ibv_create_cq");
	//printf("ibv_create_cq\n");

	return retval;

}

int ibv_get_cq_event(struct ibv_comp_channel *channel, struct ibv_cq **cq, void **cq_context)
{
	int retval;

	//printf("ibv_get_cq_event\n");
	GPTLstart("ibv_get_cq_event");

	retval = real_ibv_get_cq_event(channel, cq, cq_context);

	GPTLstop("ibv_get_cq_event");
	//printf("ibv_get_cq_event\n");

	return retval;

}

/*int ibv_req_notify_cq(struct ibv_cq *cq, int solicited_only)
{
	int retval;

	printf("ibv_req_notify_cq\n");
	GPTLstart("ibv_req_notity_cq");

	retval = real_ibv_req_notity_cq(cq, solicited_only);

	GPTLstop("ibv_req_notity_cq");
	printf("1ibv_req_notify_cq\n");

	return retval;

}*/


/*void ibv_free_device_list(struct ibv_device **list)
{
	GPTLstart("ibv_free_device_list");
	printf("ibv_free_device_list\n");
	
	real_ibv_free_device_list(list);

	printf("1ibv_free_device_list\n");
	GPTLstop("ibv_free_device_list");

	if (check == 1) {
		finalize_tracing();
		check = 0;
	}
}*/

struct ibv_qp *ibv_create_qp(struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr)
{
	struct ibv_qp* retval;
	//printf("In create_qp\n");
	GPTLstart("ibv_create_qp");

	retval = real_ibv_create_qp(pd, qp_init_attr);

	GPTLstop("ibv_create_qp");
	//printf("leaving create_qp\n");
	return retval;

}

int ibv_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr, int attr_mask)
{
	int retval;
	
	//printf("In modify qp");
	GPTLstart("ibv_modify_qp");
	
	retval = real_ibv_modify_qp(qp, attr, attr_mask);

	GPTLstop("ibv_modify_qp");
	//printf("In modify qp");

	return retval;

}

struct ibv_ah *ibv_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr)
{
	struct ibv_ah* retval;
	//printf("ibv_create_ah\n");
	GPTLstart("ibv_create_ah");

	retval = real_ibv_create_ah(pd, attr);

	GPTLstop("ibv_create_ah");
	//printf("1ibv_create_ah\n");

	return retval;

}

int mlx4_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr)
{
	int retval;


	GPTLstart("mlx4_post_send");

	retval = real_mlx4_post_send(qp,wr,bad_wr);

	GPTLstop("mlx4_post_send");

	return retval;
}

int ocrdma_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr)
{
	int retval;

	GPTLstart("ocrdma_post_send");

	retval = real_ocrdma_post_send(qp, wr, bad_wr);

	GPTLstop("ocrdma_post_send");

	return retval;

}

int mlx4_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr)
{
	int retval;

	//printf("ibv_post_recv\n");
	GPTLstart("mlx4_post_recv");

	retval = real_mlx4_post_recv(qp, wr, bad_wr);

	GPTLstop("mlx4_post_recv");
	//printf("1ibv_post_recv\n");

return retval;

}

int ocrdma_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr)
{
	int retval;

	//printf("ibv_post_recv\n");
	GPTLstart("ocrdma_post_recv");

	retval = real_ocrdma_post_recv(qp, wr, bad_wr);

	GPTLstop("ocrdma_post_recv");
	//printf("1ibv_post_recv\n");

	return retval;

}

int mlx4_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	int retval;
	//printf("Inside poll_cq\n");
	GPTLstart("mlx4_poll_cq");

	retval = real_mlx4_poll_cq (cq, num_entries, wc);
	GPTLstop("mlx4_poll_cq");

	return retval;

}

int ocrdma_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	int retval;
	//printf("Inside poll_cq\n");
	GPTLstart("ocrdma_poll_cq");

	retval = real_ocrdma_poll_cq (cq, num_entries, wc);
	GPTLstop("ocrdma_poll_cq");

	return retval;

}

int ocrdma_arm_cq(struct ibv_cq *ibcq, int solicited)
{
	int retval;
	GPTLstart("ocrdma_arm_cq");

	retval = real_ocrdma_arm_cq(ibcq, solicited);

	GPTLstop("ocrdma_arm_cq");

	return retval;

}

int mlx4_ib_arm_cq(struct ib_cq *ibcq, int/*enum ib_cq_notify_flags*/ flags)
{
	int retval;
	
	GPTLstart("mlx4_ib_arm_cq");

	retval = real_mlx4_ib_arm_cq(ibcq, /*ib_cq_notify_flags*/ flags);

	GPTLstop("mlx4_ib_arm_cq");

	return retval;

}
int ibv_destroy_ah(struct ibv_ah *ah)
{
	int retval;

	//printf("ibv_destroy_ah\n");
	GPTLstart("ibv_destroy_ah");

	retval = real_ibv_destroy_ah(ah);

	GPTLstop("ibv_destroy_ah");
	//printf("ibv_destroy_ah\n");

	return retval;
}

int ibv_destroy_qp(struct ibv_qp *qp)
{
	int retval;

	//printf("ibv_destroy_qp\n");
	GPTLstart("ibv_destroy_qp");

	retval = real_ibv_destroy_qp(qp);

	GPTLstop("ibv_destroy_qp");
	//printf("1ibv_destroy_qp\n");

	return retval;

}

int ibv_dereg_mr(struct ibv_mr *mr)
{

	int retval;

	//printf("ibv_dereg_mr\n");
	GPTLstart("ibv_dereg_mr");

	retval = real_ibv_dereg_mr(mr);

	GPTLstop("ibv_dereg_mr");
	//printf("1ibv_dereg_mr\n");

	return retval;

}

int ibv_destroy_cq(struct ibv_cq *cq)
{
	int retval;

	//printf("ibv_destroy_cq\n");
	GPTLstart("ibv_destroy_cq");

	retval = real_ibv_destroy_cq(cq);

	GPTLstop("ibv_destroy_cq");
	//printf("1ibv_destroy_cq\n");

	return retval;

}

int ibv_dealloc_pd(struct ibv_pd *pd)
{
	int retval;

	//printf(" S72 : ibv_dealloc_pd START\n");
	GPTLstart("ibv_dealloc_pd");

	retval = real_ibv_dealloc_pd(pd);

	GPTLstop("ibv_dealloc_pd");


	/*if (check == 1) {
		finalize_tracing();
		check = 0;
	}*/


	//printf("S72: ibv_dealloc_pd STOP\n");
	return retval;

}


void ibv_free_device_list(struct ibv_device **list)
{
	GPTLstart("ibv_free_device_list");
	//printf("ibv_free_device_list\n");

	real_ibv_free_device_list(list);

	//printf("1ibv_free_device_list\n");
	GPTLstop("ibv_free_device_list");

/*if (check == 1) {
finalize_tracing();
check = 0;
}*/
}


int ibv_close_device(struct ibv_context *context)
{
	int retval;
	
	//printf(" S72 : ibv_close_device START\n");
	GPTLstart("ibv_close_device");


	retval = real_ibv_close_device(context);

	//printf("1ibv_close_device\n");
	GPTLstop("ibv_close_device");

	if (check == 1) {
		finalize_tracing();
		check = 0;
	}

	//printf(" S72 : ibv_close_device STOP\n");
	return retval;
}
	
