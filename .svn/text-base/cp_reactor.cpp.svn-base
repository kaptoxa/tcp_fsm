#include "stdafx.h"

#include <fstream>
#include "pthread_stuff.h"
#include "exception.h"

#include "connection.h"
#include "cp_reactor.h"



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


cp_reactor::cp_reactor()
{
	if ((hcport = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, NULL, 0 )) == NULL )
		throw exception(" CreateIoCompletionPort failed with error ", GetLastError());
}

cp_reactor::~cp_reactor()
{
	CloseHandle( hcport );
	threads.clear();
}

void cp_reactor::add_thread(pthread_t bthread)
{
	threads.push_back( bthread );
}

void cp_reactor::add_socket(SOCKET socket)
{
	if ( NULL == CreateIoCompletionPort( (HANDLE)socket, hcport, (ULONG_PTR)0, 0 ) )
		throw exception(" CreateIoCompletionPort failed with error ", GetLastError());
}

void cp_reactor::add_conn(tcp_conn_t conn)
{
	if ( NULL == CreateIoCompletionPort( (HANDLE)conn->sock, hcport, (ULONG_PTR)conn, 0 ) )
		throw exception(" CreateIoCompletionPort failed with error ", GetLastError());
}

int cp_reactor::get_transfer(interrupt& result, IOVERLAPPED*& overlap)//send_req_struct*& s_req, recv_req_struct*& r_req)   // this is fucntion that block calling thread
{
	DWORD			size  = 0;
	ULONG_PTR   	key;

	SOCKET isocket = 0;
	int ievents = 0;

	if( !GetQueuedCompletionStatus( hcport, &size, &key, (OVERLAPPED**)&overlap, 1 ) )
	{
		int ierr = GetLastError();
		if( WAIT_TIMEOUT==ierr )
			return 0;

		assert( 0!=overlap);
		result.insert( std::make_pair(overlap->sock,FD_CLOSE) );
		return 0;
	}

	if( 0==overlap )
	{
		result.insert( std::make_pair(0,(int)0) );
		return 0;
	}

	result.insert( std::make_pair(overlap->sock,overlap->code) );
	return size;
}

void cp_reactor::what_happened()
{
}

void cp_reactor::stop(SOCKET socket, int events)
{
	// it was remarked because it is unnecessary for windows

	//for(std::vector<pthread_t>::iterator it=threads.begin(),end=threads.end();it!=end;it++)
	//	if( pthread_equal( pthread_self(), *it ) )
	//		return;

	PostQueuedCompletionStatus( hcport, 0, 0, 0 );
}

}

#endif

