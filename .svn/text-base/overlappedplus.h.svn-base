#pragma once

#include "refcounted_obj.h"
#ifndef _WIN32
#define SOCKET int
#endif


namespace tcp_fsm
{

const unsigned int MAX_WBUF_SIZE = 65536;

struct tcp_conn_struct;

#ifdef _WIN32
struct IOVERLAPPED :
	OVERLAPPED
{
    SOCKET sock;
	int code;
	tcp_conn_struct* conn;

	IOVERLAPPED(){
		memset(static_cast<OVERLAPPED*>(this),0, sizeof(OVERLAPPED));
		sock = code = 0;
	}
};
#else
struct IOVERLAPPED
{
    int sock;
	int code;
	tcp_conn_struct* conn;

	IOVERLAPPED(){ sock = code = 0;};
	IOVERLAPPED(int _sock, int _code): sock(_sock), code(_code) {};
};
#endif


struct overlap_struct:
	refcounted_obj
{
	IOVERLAPPED iov;

	overlap_struct(SOCKET,int);
	~overlap_struct();
};

struct accept_struct:
	refcounted_obj
{
	IOVERLAPPED iov;
	char addr_buf[sizeof(sockaddr_in)+16];

	accept_struct(SOCKET,int);
	~accept_struct();
};

struct recv_req_struct:
	refcounted_obj
{
	IOVERLAPPED iov;

	unsigned int size;
	char data[MAX_WBUF_SIZE]; //last member!

	recv_req_struct();
	recv_req_struct(tcp_fsm::tcp_conn_struct*);
	~recv_req_struct();
};

struct send_req_struct :
	refcounted_obj
{
	IOVERLAPPED iov;

	sem_t break_sem;					//  for break_send request

	uint32_t total_bytes;
	uint32_t sent_bytes;

	boost::function<void(bool)> sent_cb;

	uint8_t data[1]; //last member!

	send_req_struct(tcp_conn_struct*);
	~send_req_struct();
	void* operator new(size_t size, size_t size2){ return malloc(size + size2 - 1); }
	void operator delete(void* p, size_t size2) {free(p);}
	void callback(bool value=false);
};

}
