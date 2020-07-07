#pragma once

#include <pthread.h>
#include "refcounted_obj.h"
#include "itcp_dispatcher.h"
//#include "imulti_reactor.h"

#include "overlappedplus.h"






namespace tcp_fsm
{


struct tcp_conn_struct;
class cp_reactor
	//:	public imulti_reactor
{
	boost::scoped_ptr<std::ofstream> log;
	std::vector<pthread_t> threads;
	
	HANDLE hcport;

public:
	cp_reactor();		
	~cp_reactor();

	void add_thread(pthread_t);			// add one more thread to be not blocked
	void add_socket(SOCKET);
	void add_conn(tcp_conn_struct*);

	int get_transfer(interrupt&, IOVERLAPPED*&);// send_req_struct*&, recv_req_struct*&);
	void stop(SOCKET socket = 0, int events = 0);
	void what_happened();
};

}