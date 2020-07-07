#pragma once

#include <stdint.h>
#include <boost/function.hpp>

namespace tcp_fsm
{

struct tcp_conn_struct;
struct send_req_struct;
struct recv_req_struct;

typedef tcp_conn_struct* tcp_conn_t;
typedef send_req_struct* send_req_t;
typedef recv_req_struct* recv_req_t;

// trivial scenarios:

// regular connecting, sending/receiving, external termination
// connect() -> connected(!null) -> [read() ... send()] -> closed() -> release()

// regular connecting, sending/receiving, internal termination
// connect() -> connected(!null) -> [read() ... send()] -> close_conn() -> closed() -> release()

// connecting, failure
// connect() -> connected(null) -> release()

// accepting, sending/receiving, termination
// accepted_cb() -> [read() ... send()] -> closed() -> release()

// 

struct callbacks_t
{
	// read() callback is called every time there is some incoming data to process
	// subscribers are requested to process the message as fast as possible not to cause any lag to other sockets
	boost::function<void(tcp_conn_t, const void*, int size)> read;

	// closed() is called exactly one time if connected(!null) or accepted_cb() has been called before
	// no more callbacks will be called after closed()
	// closed() is never called if connected(null) occurred
	// closed() means the well-established connection is closed (either graceful or not)
	// if connection has not been established then closed() is not called
	boost::function<void(tcp_conn_t)> closed;

	// connected() is called exactly one time if connect() has been called
	// connected() is never called if connection was accpeted (accepted_cb() was called)
	// tcp_conn_t parameter provides information weather connect succeeded or not
	// !null means success, null means failure
	boost::function<void(tcp_conn_t)> connected;
};

struct itcp_dispatcher
{
	virtual ~itcp_dispatcher() throw() {}

	// creates a new unconnected tcp_conn_t
	// should be released (external reference is set to 1)
	// thread-safe
	virtual tcp_conn_t make_conn() throw() = 0;

	// performs a connect on tcp_conn_t, obtained previously by calling make_conn()
	// not thread-safe, there should be exactly one connect() call for each make_conn() 
	virtual void connect(tcp_conn_t conn, uint32_t ip, uint16_t port, const callbacks_t&) throw() = 0;

	// creates a new tcp_conn_t and performs a connect
	// should be released (external reference is set to 1)
	// thread-safe
	virtual tcp_conn_t connect(uint32_t ip, uint16_t port, const callbacks_t&) throw() = 0;

	// closes connection if not closed before
	// can be called multiple times
	// thread-safe
	virtual void close_conn(tcp_conn_t) throw() = 0; //terminates a connection (can be called multiple times)

	// closes connection if not closed before. guarantees that closed() callback is called before the function returns
	// can be called multiple times
	// not thread-safe for the same tcp_conn_t
	virtual void close_conn_wait(tcp_conn_t) throw() = 0;

	// sets limitation of outgoing bandwidth
	// not thread-safe
	virtual void set_bandwidth(uint32_t bytes_per_second) throw() = 0;

	// references tcp_conn_t
	// thread-safe
	virtual void addref(tcp_conn_t) throw() = 0;

	// dereferences tcp_conn_t
	// thread-safe
	virtual void release(tcp_conn_t) throw() = 0;

	// sends data. data of multiple simultaneous calls is not interleaved
	// sent_cb() will be called in any case. sent_cb() will be called prior to closed() callback
	// closed() callback is called after all sent_cb() are called
	// send_req_t should be released (external reference is set to 1)
	// thread-safe
	virtual send_req_t send(tcp_conn_t,
		const void* buf,
		int size,
		bool oob, //out-of-band
		const boost::function<void(bool)>& sent_cb // bool means if sending was canceled by break_send() call
		) throw() = 0;

	// sends data. data of multiple simultaneous calls is not interleaved
	// thread-safe
	virtual bool send(tcp_conn_t,
		const void* buf,
		int size,
		bool oob
		) throw() = 0;

	// breaks send request, forces immediate execution of sent_cb() callback
	// function returns only after sent_cb() is called
	// can be called multiple times
	// not thread-safe for the same send_req_t
	virtual void break_send(send_req_t) throw() = 0;

	// releases send_req_t
	// not thread-safe (should be called exactly one time for each send_req_t)
	virtual void release(send_req_t) throw() = 0;

	// stops listening, guarantees there will be no accepted_cb() calls after function returs
	// not thread-safe (should be called exactly one time)
	virtual void stop_accepting() throw() = 0;

	// retrives ip & port for tcp_conn_t
	// thread-safe
	virtual void get_ip_port(tcp_conn_t, uint32_t& ip, uint16_t& port) throw() = 0;
};

typedef boost::function<
		bool(
			tcp_conn_t, // should be released (external reference is set to 1)
			callbacks_t& //will be filled if true is returned
			)
		> accepted_cb;



#ifndef TCP_FSM_EXPORT
#ifdef _WIN32
#define TCP_FSM_EXPORT __declspec(dllimport)
#else
#define TCP_FSM_EXPORT
#endif
#endif

TCP_FSM_EXPORT itcp_dispatcher* make_tcp_dispatcher( uint16_t port, const accepted_cb& accepted );

}

//struct buf_t
//{
//	const void* buf;
//	int size;
//};

//struct smart_bufs_t
//{
//	const buf_t* bufs;
//	int size;
//	virtual void release() const = 0;
//};

// virtual send_req_t send(tcp_conn_t,
//	const buf_t* bufs,
//	int bufs_count,
//	bool oob, //out-of-band
//	const boost::function<void(bool)>& //bool means if sending was canceled by break_send() call
//	) = 0;

// virtual bool send(tcp_conn_t,
//	const buf_t* bufs,
//	int bufs_count,
//	bool oob
//	) = 0;

// virtual send_req_t send(tcp_conn_t,
//	const smart_bufs_t*,
//	bool oob, //out-of-band
//	const boost::function<void(bool)>& //bool means if sending was canceled by break_send() call
//	) = 0;

//virtual bool send(tcp_conn_t,
//	const smart_bufs_t*,
//	bool oob
//	) = 0;

