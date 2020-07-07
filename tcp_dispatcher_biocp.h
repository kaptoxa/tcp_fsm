// tcp dispatcher by completion port
#pragma once

#include <pthread.h>
#include "itcp_dispatcher.h"
#include "connection.h"
#include "multi_reactor.h"
#include "internal_messages.h"



namespace tcp_fsm
{

const unsigned int POOLSIZE = 1;

struct tcp_conn_struct;
struct send_req_struct;

typedef tcp_conn_struct* tcp_conn_t;
typedef send_req_struct* send_req_t;

typedef boost::function<bool(tcp_conn_t, callbacks_t&)> accepted_cb;

//struct itcp_dispatcher;
class tcp_dispatcher_biocp;


class tcp_dispatcher_biocp:
	public itcp_dispatcher
{
	boost::scoped_ptr<std::ofstream> log;

	pthread_t hthread;
	SOCKET listening_socket;

	pthread_t lthread;

	boost::scoped_ptr<iocp_reactor> iocp_r;
	boost::scoped_ptr<imulti_reactor> wsa_r;
	
	accepted_cb accepted;
	sem_t stop_accepting_sem;


	boost::scoped_ptr<internal_messages> i_messages;


	// to release the operations which has not been completed
	typedef std::vector< tcp_conn_t > conns_t;
	typedef std::vector< recv_req_t > recvs_t;
	typedef std::vector< send_req_t > sends_t;
	conns_t conns;		// waiting for connect or close event
	recvs_t recvs;
	sends_t sends;

	void free_completed_conn(tcp_conn_t,const char*);
	void free_completed_send(send_req_t,const char*);
	void free_completed_recv(recv_req_t,const char*);
	void free_not_completed_objects();



	void create_connection(tcp_conn_t);
	void close_connection(tcp_conn_t,bool iocp=false);	// bool means "what is the reason of closing? iocp or not?"
	tcp_conn_t accept_connection();
	void connection_established(tcp_conn_t);
	bool activate_receiving(tcp_conn_t);

	void send_request(send_req_t);
	void recv_complete(recv_req_struct*);
	void send_complete(send_req_struct*);
	void call_false_cb(send_req_t);
public:

	void listening_thread();
	void thread_proc();

	tcp_dispatcher_biocp(uint16_t listen_port,const accepted_cb& );
	~tcp_dispatcher_biocp();

	tcp_conn_t make_conn() throw();
	virtual void connect(tcp_conn_t conn, uint32_t ip, uint16_t port, const callbacks_t&) throw();

	virtual tcp_conn_t connect(uint32_t ip, uint16_t port, const callbacks_t&) throw();
	virtual void close_conn(tcp_conn_t) throw();
	virtual void close_conn_wait(tcp_conn_t) throw();

	virtual void set_bandwidth(uint32_t bytes_per_second  ) throw(){}

	virtual void addref(tcp_conn_t) throw();
	virtual void release(tcp_conn_t) throw();

	virtual send_req_t send(tcp_conn_t,
		const void* buf,
		int size,
		bool oob, //out-of-band
		const boost::function<void(bool)>& 
		) throw();

	virtual bool send(tcp_conn_t,
		const void* buf,
		int size,
		bool oob
		) throw();

	virtual void break_send(send_req_t) throw();
	virtual void release(send_req_t) throw();

	virtual void stop_accepting() throw();
	virtual void get_ip_port(tcp_conn_t, uint32_t& ip, uint16_t& port) throw();
};

itcp_dispatcher* make_tcp_dispatcher( uint16_t port, const accepted_cb& accepted );

}
