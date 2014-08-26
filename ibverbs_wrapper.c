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

static void  getsymbols(char *so_name) {
	
	void *dldriver;

	dldriver = dlopen(so_name, RTLD_NOW);

	if (!dldriver) fprintf ( stderr, "Warning: Couldn't load driver from %s\n", so_name);

	if ( strstr ( so_name  , "mlx") ) {
		real_mlx4_post_send = dlsym(dldriver,"mlx4_post_send");
		real_mlx4_post_send = dlsym(dldriver,"mlx4_post_send");
		real_mlx4_post_recv = dlsym(dldriver,"mlx4_post_recv");
		real_mlx4_arm_cq = dlsym(dldriver, "mlx4_ib_arm_cq");
		
		fprintf(stdout, "address of send=%p recv=%p poll=%p arm_cq=%p\n",real_mlx4_post_send,real_mlx4_post_recv,real_mlx4_poll_cq,real_mlx4_arm_cq);
	}
	else if ( strstr ( so_name, "ocrdma") ) {
		real_ocrdma_post_send = dlsym(dldriver,"ocrdma_post_send");
		real_ocrdma_post_send = dlsym(dldriver,"ocrdma_post_send");
		real_ocrdma_post_recv = dlsym(dldriver,"ocrdma_post_recv");
		real_ocrdma_poll_cq   = dlsym(dldriver,"ocrdma_poll_cq");

		fprintf(stdout, "address of send=%p recv=%p poll=%p arm_cq=%p\n",real_ocrdma_post_send,real_ocrdma_post_recv,real_ocrdma_poll_cq,real_ocrdma_arm_cq);
	}
	else {
		//Do nothing..
	}
}


static char *getpath(char *buffer) {
	
	char *iterator;

	char *path = (char *)malloc(4096*sizeof(char));
	int i = 0;

	//First we need to jump to the begining of the first path found from LD_LIBRARY_PATH
	iterator = strstr(buffer, ": ");

	//What if there is no shared library found
	if ( iterator == NULL ) {
		fprintf(stdout,"Warning: Shared library not found.. Are you sure its in LD_LIBRARY_PATH ?");
		return NULL;
	}

	//Iterator now points to : /path/to/shared/library
	//						 ^
	//We ned to jump two positions to reach /path...
	//										^
	iterator += 2;

	//Lets move character by character until we reach end of the first path found
	while ( iterator[i] != ' ' && iterator[i] != '\n' ) {
		path[i] = iterator[i];
		i++;
	}
	//We need to make sure that this path string is delimited
	path[i]='\0';

	return path;

}

static void load_driver(char *dname) {
	
	FILE *cmd_output;
	char buffer[1024];

	//If its a Emulex based RoCE solution
	if ( strstr ( dname, "ocrdma" ) ) 
		cmd_output = popen("whereis libocrdma", "r");

	//If its a Mellanox based RoCE / Infiniband solution
	else if ( strstr ( dname, "mlx" ) ) 
		cmd_output = popen("whereis libmlx4-rdmav2" , "r");
	
	//If its a card that we havent taken care of yet
        //Send me an email if you are intersted in writing a wrapper to a particular vendor's verb implementation
	else {
		printf("Warning: Didnt find any recognizable RDMA devices\n");
		return;
	}
	fgets(buffer, 1024, cmd_output);
	getsymbols( getpath(buffer) );
}


static void read_device_specific_symbols() {

	char infini_path[] = "/sys/class/infiniband/";
	struct dirent *curDir;
	int n = 0;

	DIR *infinibandDir = opendir(infini_path);
	if ( infinibandDir == NULL ) {
		printf("Are you sure RDMA modules are loaded? Will abort now..\n");
		exit(0);
	}

	while ( (curDir = readdir(infinibandDir)) != NULL ) {
		if ( strcmp(curDir -> d_name , ".")  && strcmp(curDir->d_name , ".." ) ) {
			load_driver(curDir->d_name);	
			n++;
		}
	}
	if ( n == 0 ) {
		printf("Looks like there is no RoCE / Infiniband / iWARP capable devices..\n");
		printf("Are you sure device driver modules are loaded?");
		printf("No use for me.. Will abort now!");
		exit(0);
	}
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

	GPTLstart("mlx4_post_recv");

	retval = real_mlx4_post_recv(qp, wr, bad_wr);

	GPTLstop("mlx4_post_recv");

return retval;

}

int ocrdma_post_recv(struct ibv_qp *qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr)
{
	int retval;

	GPTLstart("ocrdma_post_recv");

	retval = real_ocrdma_post_recv(qp, wr, bad_wr);

	GPTLstop("ocrdma_post_recv");

	return retval;

}

int mlx4_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	int retval;
	
	GPTLstart("mlx4_poll_cq");

	retval = real_mlx4_poll_cq (cq, num_entries, wc);
	
	GPTLstop("mlx4_poll_cq");

	return retval;

}

int ocrdma_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	int retval;
	
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

int mlx4_arm_cq(struct ibv_cq *ibcq, int/*enum ib_cq_notify_flags*/ flags)
{
	int retval;
	
	GPTLstart("mlx4_arm_cq");

	retval = real_mlx4_arm_cq(ibcq, /*ib_cq_notify_flags*/ flags);

	GPTLstop("mlx4_ib_arm_cq");

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
	
