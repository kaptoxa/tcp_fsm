#pragma once

#include <pthread.h>
#include "itcp_dispatcher.h"
#include "connection.h"
#include "multi_reactor.h"
#include "internal_messages.h"
#include "bwstop.h"


namespace tcp_fsm
{

class tcp_dispatcher:
	public itcp_dispatcher
{
	boost::scoped_ptr<std::ofstream> log, elog;

	pthread_t hthread;
	int listening_socket;


	boost::scoped_ptr<tcp_conn_struct> ec;
	boost::scoped_ptr<imulti_reactor> mr;

	accepted_cb accepted;

	boost::scoped_ptr<internal_messages> i_messages;


	typedef std::vector<send_req_t> sendings_t;
	typedef std::map<tcp_conn_t,sendings_t*> outgoings_t;
	outgoings_t outgoings;
	boost::scoped_ptr<bwstop> bws;



	void create_connection(tcp_conn_t);
	void close_connection(tcp_conn_t);
	tcp_conn_t accept_connection();
	void connection_established(tcp_conn_t);
	void activate_receiving(tcp_conn_t);

	int send_request(send_req_t);
	void recv_complete(recv_req_struct*);
	void send_complete(send_req_struct*);
	void call_false_cb(send_req_t);

	int do_recv(tcp_conn_struct*);
	bool do_send(send_req_struct*,uint32_t* guess = 0);

	void TRACE(const char* msg,...);

public:

	void thread_proc();

	tcp_dispatcher(uint16_t listen_port,const accepted_cb&);
	~tcp_dispatcher() throw();

	tcp_conn_t make_conn() throw();
	virtual void connect(tcp_conn_t conn, uint32_t ip, uint16_t port, const callbacks_t&) throw();

	virtual tcp_conn_t connect(uint32_t ip, uint16_t port, const callbacks_t&) throw();
	virtual void close_conn(tcp_conn_t) throw();
	virtual void close_conn_wait(tcp_conn_t) throw();

	virtual void set_bandwidth(uint32_t bytes_per_second) throw();

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
	virtual void get_ip_port(tcp_conn_t, uint32_t& ip, uint16_t& port) throw() ;
};

}
