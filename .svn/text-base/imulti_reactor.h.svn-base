#pragma once

#ifndef _WIN32

#define SOCKET int

#define FD_READ_BIT      0
#define FD_READ          (1 << FD_READ_BIT)

#define FD_WRITE_BIT     1
#define FD_WRITE         (1 << FD_WRITE_BIT)

#define FD_ACCEPT_BIT    3
#define FD_ACCEPT        (1 << FD_ACCEPT_BIT)

#define FD_CONNECT_BIT   4
#define FD_CONNECT       (1 << FD_CONNECT_BIT)

#define FD_CLOSE_BIT     5
#define FD_CLOSE         (1 << FD_CLOSE_BIT)

#endif


namespace tcp_fsm
{

struct tcp_conn_struct;
struct send_req_struct;

typedef tcp_conn_struct* tcp_conn_t;
typedef send_req_struct* send_req_t;

#ifdef _WIN32
typedef std::map<SOCKET,int> interrupt;
#else
struct interrupt_info
{
	int	code;

	tcp_conn_t conn;
	send_req_t sreq;

	interrupt_info()
	{
		code = 0;
		conn = 0;
		sreq = 0;
	}
};
typedef std::map<SOCKET,interrupt_info> interrupt;
#endif

struct imulti_reactor {
	virtual ~imulti_reactor(){}

	virtual void add_event(SOCKET socket, int type )=0;
	virtual void del_event(SOCKET socket)=0;

	virtual void add_thread(pthread_t)=0;					// add one more thread to be not blocked,
															// other threads (has not been added) will be blocked

	virtual void add_conn(tcp_conn_t )=0;					// del_conn performs by close socket

	virtual bool get_interrupt(interrupt&)=0;
	virtual void stop(SOCKET socket = 0)=0;
};


#ifdef _WIN32
imulti_reactor* make_iocp_reactor();
imulti_reactor* make_wsa_reactor();
#else
imulti_reactor* make_multi_reactor();
#endif

}
