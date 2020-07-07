
#include "stdafx.h"
#include "multi_reactor.h"
#include "pthread_stuff.h"
#include "exception.h"
#include <fstream>

#include "connection.h"



#if defined(_DEBUG) && defined(_WIN32)
#define new DEBUG_NEW
#endif

#ifdef _WIN32


namespace tcp_fsm
{

struct tcp_conn_struct;
struct send_req_struct;

typedef tcp_conn_struct* tcp_conn_t;
typedef send_req_struct* send_req_t;


iocp_reactor::iocp_reactor()
{
	if ((hcport = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, NULL, 0 )) == NULL )
		throw exception(" CreateIoCompletionPort failed with error ", GetLastError());
}

iocp_reactor::~iocp_reactor()
{
	CloseHandle( hcport );
	threads.clear();
}

void iocp_reactor::add_thread(pthread_t bthread)
{
	threads.push_back( bthread );
}


void iocp_reactor::add_conn(tcp_conn_t conn)
{
	if ( NULL == CreateIoCompletionPort( (HANDLE)conn->sock, hcport, (ULONG_PTR)conn, 0 ) )
		throw exception(" CreateIoCompletionPort failed with error ", GetLastError());
}

int iocp_reactor::get_transfer(interrupt& result, tcp_conn_t& rconn, send_req_struct*& s_req, recv_req_struct*& r_req)   // this is fucntion that block calling thread
{
	DWORD			size  = 0;
	ULONG_PTR   	key;
	IOVERLAPPED*	overlap;

	SOCKET isocket = 0;
	int ievents = 0;
	s_req = 0;
	r_req = 0;

	BOOL b = GetQueuedCompletionStatus( hcport, &size, &key, (OVERLAPPED**)&overlap, INFINITE );
	if( !b )
	{
		int ierr = GetLastError();
		switch( ierr )
		{
		case ERROR_CONNECTION_ABORTED:
		case ERROR_NETNAME_DELETED:
			{
				r_req = CONTAINING_RECORD( overlap, recv_req_struct, iov);
				r_req->size = size;

				result.insert( std::make_pair(overlap->sock,FD_READ) );
				return size;
			}
		default:

			assert( 0!=overlap);
			result.insert( std::make_pair(overlap->sock,FD_CLOSE) );
			return 0;
		}
	}

	if( 0==overlap )
	{
		result.insert( std::make_pair(0,(int)0) );
		return 0;
	}

	if( FD_READ==overlap->code )
	{
		r_req = CONTAINING_RECORD( overlap, recv_req_struct, iov);
		r_req->size = size;
		overlap->sock = r_req->iov.conn->sock;
	}

	if( FD_WRITE==overlap->code )
	{
		s_req = CONTAINING_RECORD( overlap, send_req_struct, iov);
		s_req->sent_bytes = size;
	}

	result.insert( std::make_pair(overlap->sock,overlap->code) );
	return size;
}

void iocp_reactor::what_happened()
{
}

void iocp_reactor::stop(SOCKET socket, int events)
{
	// it was remarked because it is unnecessary for windows

	//for(std::vector<pthread_t>::iterator it=threads.begin(),end=threads.end();it!=end;it++)
	//	if( pthread_equal( pthread_self(), *it ) )
	//		return;

	PostQueuedCompletionStatus( hcport, 0, 0, 0 );
}

}

#endif

