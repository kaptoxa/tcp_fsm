#pragma once

#include <pthread.h>
#include "refcounted_obj.h"
#include "itcp_dispatcher.h"
#include "multi_reactor.h"
#include "overlappedplus.h"


namespace tcp_fsm
{

enum conn_state
{
	cs_wait4connect,
	cs_connected,
	cs_closed,
}; 

struct tcp_conn_struct:
	refcounted_obj
{
	SOCKET sock;
	sockaddr_in addr;

	callbacks_t cbs;
	tcp_fsm::recv_req_struct* recv; //should be pointer!!!!

	uint32_t* guess;
	sem_t close_sem;

	FILE* dump;
	uint8_t checkbyte;

	conn_state state;

	tcp_conn_struct(){}
	tcp_conn_struct(SOCKET socket, callbacks_t _cbs )
	{
#ifndef NDEBUG
		printf("tcp_conn_t() %p\n", this);
#endif
		dumping_on = true;
		guess = new uint32_t(0);

		sem_init( &close_sem, 0, 0);

		sock = socket;
		cbs = _cbs;
		recv = 0;

		checkbyte = 255;

		state = cs_wait4connect;
	}
	~tcp_conn_struct()
	{
		delete guess;
		sem_destroy( &close_sem );
#ifndef NDEBUG
		printf("~tcp_conn_t() %p\n", this);
#endif
	}
};

}
