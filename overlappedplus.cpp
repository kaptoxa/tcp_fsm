#include "stdafx.h"
#include "overlappedplus.h"
#include "connection.h"


#if defined(_DEBUG) && defined(_WIN32)
#define new DEBUG_NEW
#endif


namespace tcp_fsm
{

#ifdef _WIN32

overlap_struct::overlap_struct(SOCKET sock,int code)
{
	dumping_on = true;
	iov = IOVERLAPPED();
	iov.code = code;
	iov.sock = sock;
}
overlap_struct::~overlap_struct(){}

accept_struct::accept_struct(SOCKET sock,int code)
{
	dumping_on = false;
	iov = IOVERLAPPED();
	iov.code = code;
	iov.sock = sock;
}
accept_struct::~accept_struct(){}

recv_req_struct::recv_req_struct(tcp_fsm::tcp_conn_struct* _conn)
{
	dumping_on = false;

	iov = IOVERLAPPED();
	iov.code = FD_READ;
	iov.sock = _conn->sock;
	iov.conn = _conn;
	iov.conn->addref("recv_req_struct");
}
recv_req_struct::~recv_req_struct()
{
	iov.conn->release("~recv_req_struct");
}


send_req_struct::send_req_struct(tcp_conn_struct* _conn)
{
	//dumping_on = true;
	iov = IOVERLAPPED();
	iov.code = FD_WRITE;
	iov.sock = _conn->sock;
	iov.conn = _conn;
	iov.conn->addref("send_req_struct");
}

send_req_struct::~send_req_struct()
{
	iov.conn->release("~send_req_struct");
}

void send_req_struct::callback(bool value)
{
	if( this->sent_cb )
	{
		this->sent_cb(value);
		this->sent_cb = 0;
	}
}

#endif

#ifndef _WIN32

overlap_struct::overlap_struct(SOCKET sock,int code)
{
        dumping_on = false;
        iov = IOVERLAPPED(sock,code);
}

overlap_struct::~overlap_struct(){}

accept_struct::accept_struct(SOCKET sock,int code)
{
        dumping_on = false;
        iov = IOVERLAPPED(sock,code);
}

accept_struct::~accept_struct(){}

recv_req_struct::recv_req_struct(tcp_fsm::tcp_conn_struct* conn)
{
        dumping_on = false;
        iov = IOVERLAPPED(conn->sock,FD_READ);
	iov.conn = conn;
        iov.conn->addref("recv_req_struct");
}

recv_req_struct::~recv_req_struct()
{
        iov.conn->release("~recv_req_struct");
}

send_req_struct::send_req_struct(tcp_conn_struct* conn)
{
        //dumping_on = true;
        iov = IOVERLAPPED(conn->sock,FD_WRITE);
        iov.conn = conn;
        iov.conn->addref("send_req_struct");

        sem_init( &break_sem, 0, 0);
}

send_req_struct::~send_req_struct()
{
        sem_destroy(&break_sem);
        iov.conn->release("~send_req_struct");
}

void send_req_struct::callback(bool value)
{
        if( this->sent_cb )
        {
                this->sent_cb(value);
                this->sent_cb = 0;
        }
}

#endif


}
