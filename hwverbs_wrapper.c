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
#include "hwverbs_wrapper.h"

/************************************************************************************

NOTE: When adding new "hwverbs", changes will affect in getsymbols and load driver only

*************************************************************************************/

/*
*
* Function getsymbols
*
*/

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

/*
*
* Function getpath parses the buffer which holds the path list of different copies/versions of .so
* that LD_LIBRARY_PATH found. We pick the first one, as when an RDMA - app runs, this is what would be 
* called
*
*/

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

/*
*
* Function load driver finds out device used and loads corresponding .so file associated
*/

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


/*
*
* Function read_device_specific_symbols is used to read symbols for device implementation of data path verbs
* Here we only read the device symbols whose driver modules have been loaded.
* One way to find this is to look for all the sub-folder under /sys/class/infiniband 
*
*/

void read_device_specific_symbols() {

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

/************************************************************************************

					OCRDMA

*************************************************************************************/

int ocrdma_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr)
{
	int retval;

	GPTLstart("ocrdma_post_send");

	retval = real_ocrdma_post_send(qp, wr, bad_wr);

	GPTLstop("ocrdma_post_send");

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

/************************************************************************************

					MLX

*************************************************************************************/

int mlx4_post_send(struct ibv_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr)
{
	int retval;


	GPTLstart("mlx4_post_send");

	retval = real_mlx4_post_send(qp,wr,bad_wr);

	GPTLstop("mlx4_post_send");

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


int mlx4_poll_cq(struct ibv_cq *cq, int num_entries, struct ibv_wc *wc)
{
	int retval;
	
	GPTLstart("mlx4_poll_cq");

	retval = real_mlx4_poll_cq (cq, num_entries, wc);
	
	GPTLstop("mlx4_poll_cq");

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

/************************************************************************************

					OTHERS

*************************************************************************************/

