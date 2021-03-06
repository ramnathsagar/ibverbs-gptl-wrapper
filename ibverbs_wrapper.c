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
#include "ibverbs_wrapper.h"
#include "hwverbs_wrapper.h"

static int check = 0;

static void setoptions()
{
	int ret;
#ifdef HAVE_PAPI
	ret = GPTLsetoption (GPTL_IPC, 1);
	ret = GPTLsetoption (PAPI_TOT_INS, 1);
	ret = GPTLsetoption (PAPI_L2_DCM, 1);
#endif

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


static void readibsymbol()
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
	

	real_ibv_destroy_ah = dlsym(dlhandle,"ibv_destroy_ah");
	real_ibv_destroy_qp = dlsym(dlhandle,"ibv_destroy_qp");
	real_ibv_dereg_mr = dlsym(dlhandle,"ibv_dereg_mr");
	real_ibv_destroy_cq = dlsym(dlhandle,"ibv_destroy_cq");
	real_ibv_dealloc_pd = dlsym(dlhandle,"ibv_dealloc_pd");

	real_ibv_free_device_list = dlsym(dlhandle, "ibv_free_device_list");
	real_ibv_close_device = dlsym(dlhandle, "ibv_close_device");

	//printf("%p , %p and %p\n",real_ibv_get_cq_event,real_ibv_create_qp,real_ibv_modify_qp);
}

struct ibv_device **ibv_get_device_list(int *num_devices)
{
	struct ibv_device **retval;
		
	if (check == 0) {
		readibsymbol();
		read_device_specific_symbols();
		setoptions();
		initialize_tracing();
		check = 1;
	}

	GPTLstart("ibv_get_device_list");

	retval = real_ibv_get_device_list(num_devices);

	GPTLstop("ibv_get_device_list");

	return retval;
}

const char *ibv_get_device_name(struct ibv_device *device)
{
	const char* retval;

	GPTLstart("ibv_get_device_name");

	retval = real_ibv_get_device_name(device);

	GPTLstop("ibv_get_device_name");

	return retval;

}

struct ibv_context *ibv_open_device(struct ibv_device *device)
{
	struct ibv_context* retval;
	
	GPTLstart("ibv_open_device");

	retval = real_ibv_open_device(device);

	GPTLstop("ibv_open_device");

	return retval;

}

int ibv_query_device(struct ibv_context *context, struct ibv_device_attr *device_attr)
{
	int retval;

	GPTLstart("ibv_query_device");

	retval = real_ibv_query_device(context, device_attr);

	GPTLstop("ibv_query_device");

	return retval;

}

int ibv_query_gid(struct ibv_context *context, uint8_t port_num, int index, union ibv_gid *gid)
{
	int retval;

	GPTLstart("ibv_query_gid");

	retval = real_ibv_query_gid(context, port_num, index, gid);

	GPTLstop("ibv_query_gid");

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

	GPTLstart("ibv_alloc_pd");

	retval = real_ibv_alloc_pd(context);

	GPTLstop("ibv_alloc_pd");

	return retval;

}

struct ibv_mr *ibv_reg_mr(struct ibv_pd *pd, void *addr, size_t length, int access)
{
	struct ibv_mr* retval;
	
	GPTLstart("ibv_reg_mr");

	retval = real_ibv_reg_mr(pd, addr, length, access);

	GPTLstop("ibv_reg_mr");

	return retval;

}

struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe, void *cq_context, struct ibv_comp_channel *channel, int comp_vector)
{
	struct ibv_cq* retval;
	
	GPTLstart("ibv_create_cq");

	retval = real_ibv_create_cq(context, cqe, cq_context, channel, comp_vector);

	GPTLstop("ibv_create_cq");

	return retval;

}

int ibv_get_cq_event(struct ibv_comp_channel *channel, struct ibv_cq **cq, void **cq_context)
{
	int retval;

	GPTLstart("ibv_get_cq_event");

	retval = real_ibv_get_cq_event(channel, cq, cq_context);

	GPTLstop("ibv_get_cq_event");

	return retval;

}

struct ibv_qp *ibv_create_qp(struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr)
{
	struct ibv_qp* retval;
	
	GPTLstart("ibv_create_qp");

	retval = real_ibv_create_qp(pd, qp_init_attr);

	GPTLstop("ibv_create_qp");
	
	return retval;

}

int ibv_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr, int attr_mask)
{
	int retval;
	
	GPTLstart("ibv_modify_qp");
	
	retval = real_ibv_modify_qp(qp, attr, attr_mask);

	GPTLstop("ibv_modify_qp");

	return retval;

}

struct ibv_ah *ibv_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr)
{
	struct ibv_ah* retval;
	GPTLstart("ibv_create_ah");

	retval = real_ibv_create_ah(pd, attr);

	GPTLstop("ibv_create_ah");

	return retval;

}


int ibv_destroy_ah(struct ibv_ah *ah)
{
	int retval;

	GPTLstart("ibv_destroy_ah");

	retval = real_ibv_destroy_ah(ah);

	GPTLstop("ibv_destroy_ah");

	return retval;
}

int ibv_destroy_qp(struct ibv_qp *qp)
{
	int retval;

	GPTLstart("ibv_destroy_qp");

	retval = real_ibv_destroy_qp(qp);

	GPTLstop("ibv_destroy_qp");

	return retval;

}

int ibv_dereg_mr(struct ibv_mr *mr)
{

	int retval;

	GPTLstart("ibv_dereg_mr");

	retval = real_ibv_dereg_mr(mr);

	GPTLstop("ibv_dereg_mr");

	return retval;

}

int ibv_destroy_cq(struct ibv_cq *cq)
{
	int retval;

	GPTLstart("ibv_destroy_cq");

	retval = real_ibv_destroy_cq(cq);

	GPTLstop("ibv_destroy_cq");

	return retval;

}

int ibv_dealloc_pd(struct ibv_pd *pd)
{
	int retval;

	GPTLstart("ibv_dealloc_pd");

	retval = real_ibv_dealloc_pd(pd);

	GPTLstop("ibv_dealloc_pd");

	return retval;

}


void ibv_free_device_list(struct ibv_device **list)
{
	GPTLstart("ibv_free_device_list");

	real_ibv_free_device_list(list);

	GPTLstop("ibv_free_device_list");

}


int ibv_close_device(struct ibv_context *context)
{
	int retval;
	
	GPTLstart("ibv_close_device");

	retval = real_ibv_close_device(context);

	GPTLstop("ibv_close_device");

	if (check == 1) {
		finalize_tracing();
		check = 0;
	}

	return retval;
}
	
