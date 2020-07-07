#pragma once

#include <queue>
#include <set>
#include "itcp_dispatcher.h"
#include "imulti_reactor.h"
#include "refcounted_obj.h"
#include <pthread.h>

#include "overlappedplus.h"






namespace tcp_fsm
{




#ifdef _WIN32



struct tcp_conn_struct;
class iocp_reactor//:	public imulti_reactor
{
	std::vector<pthread_t> threads;
	
	HANDLE hcport;

public:
	iocp_reactor();		
	~iocp_reactor();

	void add_thread(pthread_t);			// add one more thread to be not blocked
	void add_conn(tcp_conn_struct*);

	int get_transfer(interrupt&, tcp_conn_t&, send_req_struct*&, recv_req_struct*&);
	void stop(SOCKET socket = 0, int events = 0);
	void what_happened();
};





class wsa_reactor:
	public imulti_reactor
{
	SOCKET isocket;
	WSANETWORKEVENTS wsEvents;

	HANDLE hExitEvent;
	HANDLE hInterruptEvent;

	pthread_mutex_t	wait_events_mutex;
	typedef std::map<WSAEVENT,SOCKET> wait_events_t;
	boost::shared_ptr<wait_events_t> wait_events;

public:

	wsa_reactor();
	~wsa_reactor();

	void add_event(SOCKET socket, int type );
	void del_event(SOCKET socket);

	void add_thread(pthread_t);
	void add_conn(tcp_conn_struct*);

	bool get_interrupt(interrupt&);
	void stop(SOCKET socket = 0);
	bool what_happened();				// if there were no errors then return true
};




#endif
//#if defined(HAVE_EPOLL_CREATE)
//#elif HAVE_EPOLL_CREATE

struct tcp_conn_struct;

class linux_reactor:
	public imulti_reactor
{
	boost::scoped_ptr<std::ofstream> log;
	std::vector<pthread_t> threads;

	int epfd;
	int pipe_fds[2];
	
public:

	linux_reactor();
	~linux_reactor();

	void add_event(int socket, int type );
	void del_event(int socket );

	void add_thread(pthread_t);
	void add_conn(tcp_conn_struct*);

	bool get_interrupt(interrupt&);
	void stop(int socket = 0);
	
	void	what_happened(int);
};







//#endif
#ifdef _FreeBSD

class fbsd_reactor:
	public imulti_reactor
{
	boost::scoped_ptr<std::ofstream> log;

	int kqfd;
	int pipe_fds[2];
	int isocket;

	typedef std::set<int> desc_set_t;
	desc_set_t desc_set;

public:

	fbsd_reactor();
	~fbsd_reactor();

	void	add_event(int socket, int type );

	bool	get_interrupt();
	void	interrupt(int socket = 0);
	bool	internal_interrupt() { return 0==isocket; }

	int		who() { return isocket; }
	void	what_happened() {}
	int		which() { return 0; }

};








#else

class socket_reactor:
	public imulti_reactor
{
	boost::scoped_ptr<std::ofstream> log;

	int pipe_fds[2];
	int isocket;

	typedef std::set<int> desc_set_t;
	desc_set_t r_desc_set;
	desc_set_t w_desc_set;

public:

	socket_reactor();
	~socket_reactor();

	void	add_event(int socket, int type );

	bool	get_interrupt();
	void	interrupt(int socket = 0);
	bool	internal_interrupt() { return 0==isocket; }

	int		who() { return isocket; }
	void	what_happened() {}
	int		which() { return 0; }

};

#endif

}
