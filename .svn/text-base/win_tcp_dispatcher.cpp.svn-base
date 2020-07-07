#include "stdafx.h"
#include "itcp_dispatcher.h"
#include "win_tcp_dispatcher.h"
#include <pthread_stuff.h>

#include <fstream>




#if defined(_DEBUG) && defined(_WIN32)
#define new DEBUG_NEW
#endif

#ifdef PTW32_STATIC_LIB
static struct pthread_attacher{
	pthread_attacher(){pthread_win32_process_attach_np();}
	~pthread_attacher(){pthread_win32_process_detach_np();}
} _pthread_attacher;
#endif




namespace tcp_fsm
{

struct itcp_dispatcher;
class win_tcp_dispatcher;


static void* Pool4CPProc( void* arg )
{
	win_tcp_dispatcher* tcpd = (win_tcp_dispatcher*)arg;
	if( tcpd )
		tcpd->thread_proc() ;
	return 0L;
}

void win_tcp_dispatcher::TRACE(const char* msg,...)
{
	//*log << msg << "\n";
	//log->flush();
}

win_tcp_dispatcher::win_tcp_dispatcher(uint16_t listen_port,const accepted_cb& _accepted):
	accepted(accepted)
{
	log.reset( new std::ofstream("dispatcher.log"));
	bws.reset( new bwstop() );

	lpfnAcceptEx = 0;
	lpfnConnectEx = 0;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidConnectEx = WSAID_CONNECTEX;

	listening_socket=-1;

	try
    {
        if (listen_port)
        {
			accepted = _accepted;
			sem_init( &stop_accepting_sem, 0, 0);

			listening_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (SOCKET_ERROR==listening_socket)
				throw exception(" WSASocket failed with error = " ,WSAGetLastError());

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = 0;
			addr.sin_port = htons(listen_port);
			if (-1==::bind(listening_socket,(sockaddr*)&addr,sizeof addr))
				throw exception(" bind failed with error = " ,WSAGetLastError());

			if (-1==::listen(listening_socket,SOMAXCONN))
				throw exception(" listen failed with error = " ,WSAGetLastError());

			DWORD dwBytes = 0;
			WSAIoctl(listening_socket, 
				SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&GuidAcceptEx, 
				sizeof(GuidAcceptEx),
				&lpfnAcceptEx, 
				sizeof(lpfnAcceptEx), 
				&dwBytes, 
				NULL, 
				NULL);
			WSAIoctl(listening_socket, 
				SIO_GET_EXTENSION_FUNCTION_POINTER, 
				&GuidConnectEx, 
				sizeof(GuidConnectEx),
				&lpfnConnectEx, 
				sizeof(lpfnConnectEx), 
				&dwBytes, 
				NULL, 
				NULL);

			i_messages.reset( new internal_messages() );
			cp_r.reset( new cp_reactor() );
			if( int ierr = pthread_create(&hthread, NULL, Pool4CPProc, (void*)this) )
				throw exception(" pthread_create failed with error = " , ierr );
			cp_r->add_thread( hthread ); // 0?! ...
			cp_r->add_socket(listening_socket);

			accept_request();
        }
    }
    catch ( exception& ex )
	{
		if( -1!=listening_socket ) closesocket(listening_socket);
		TRACE("<win_tcp_dispatcher::win_tcp_dispatcher> ", ex.what());
		throw; 
    }
}



win_tcp_dispatcher::~win_tcp_dispatcher(void)
{
	i_messages->push(_P_EXIT);
	cp_r->stop();
	pthread_join( hthread, 0 );

	cp_r.reset(0);
	sem_destroy( &stop_accepting_sem );
}

tcp_conn_t win_tcp_dispatcher::make_conn()
{
	SOCKET new_socket = SOCKET_ERROR;
	try
	{
		sockaddr_in baddr;
		baddr.sin_family=AF_INET;
		baddr.sin_addr.s_addr=INADDR_ANY;
		baddr.sin_port=0;

		new_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (SOCKET_ERROR==new_socket)
			throw exception(" WSASocket failed with error = ", WSAGetLastError());

		if (-1==::bind(new_socket,(sockaddr*)&baddr,sizeof baddr))
			throw exception(" bind failed with error = " ,WSAGetLastError());

		unsigned long a=O_NONBLOCK;
		if (SOCKET_ERROR== ioctlsocket(new_socket,FIONBIO,&a) )
			throw exception(" ioctlsocket failed with error = ", WSAGetLastError());

		cp_r->add_socket(new_socket);

		tcp_conn_t conn = new tcp_conn_struct(new_socket,callbacks_t());
		conn->addref("async_connect");
		conn->addref("async_connect");
		return conn;
	}
	catch( exception& ex ){
		if( SOCKET_ERROR != new_socket ) closesocket(new_socket);
		TRACE("<win_tcp_dispatcher::make_conn>", ex.what());
		return 0;
	}
}

void win_tcp_dispatcher::connect(tcp_conn_t conn, uint32_t ip, uint16_t port, const callbacks_t& cb)
{
	try
	{
		sockaddr_in addr;
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=ip;
		addr.sin_port=htons(port);

		conn->addr = addr;
		conn->cbs = cb;

		i_messages->push( _P_CREATE_CONNECTION, (char*)conn );
		cp_r->stop();
	}
	catch( exception& ex ){
		TRACE("<win_tcp_dispatcher::connect>", ex.what());
		return;
	}
}

tcp_conn_t win_tcp_dispatcher::connect(uint32_t ip, uint16_t port, const callbacks_t& cb)
{
	SOCKET new_socket = SOCKET_ERROR;
	try
	{
		sockaddr_in baddr;
		baddr.sin_family=AF_INET;
		baddr.sin_addr.s_addr=INADDR_ANY;
		baddr.sin_port=0;

		new_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (SOCKET_ERROR==new_socket)
			throw exception(" WSASocket failed with error = ", WSAGetLastError());

		if (-1==::bind(new_socket,(sockaddr*)&baddr,sizeof baddr))
			throw exception(" bind failed with error = " ,WSAGetLastError());

		unsigned long a=O_NONBLOCK;
		if (SOCKET_ERROR== ioctlsocket(new_socket,FIONBIO,&a) )
			throw exception(" ioctlsocket failed with error = ", WSAGetLastError());

		cp_r->add_socket(new_socket);


		sockaddr_in addr;
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=ip;
		addr.sin_port=htons(port);

		tcp_conn_t conn = new tcp_conn_struct(new_socket, cb);
		conn->addref("async_connect");
		conn->addref("async_connect");
		conn->addr = addr;

		i_messages->push( _P_CREATE_CONNECTION, (char*)conn );
		cp_r->stop();
		return conn;
	}
	catch( exception& ex ){
		if( SOCKET_ERROR != new_socket ) closesocket(new_socket);
		TRACE("<win_tcp_dispatcher::connect>", ex.what());
		return 0;
	}
}

void win_tcp_dispatcher::close_conn(tcp_conn_t conn)
{
	conn->addref("close_conn");
	i_messages->push( _P_CLOSE_CONNECTION, (char*)conn );
	cp_r->stop();
}

void win_tcp_dispatcher::close_conn_wait(tcp_conn_t conn)
{
	conn->addref("close_conn_wait");
	i_messages->push( _P_CLOSE_CONNECTION_WAIT, (char*)conn );
	cp_r->stop();

	sem_wait( &conn->close_sem);
}

void win_tcp_dispatcher::release(tcp_conn_t conn)
{
	conn->release("win_tcp_dispatcher::release");
}

bool win_tcp_dispatcher::send(tcp_conn_t conn,const void* buf,int size,bool oob)
{
	send_req_t req=send(conn,buf,size,oob,boost::function<void(bool)>());
	req->release("win_tcp_dispatcher::send");
	return true;
}

send_req_t win_tcp_dispatcher::send(tcp_conn_t conn,const void* buf,int size,
								bool oob,const boost::function<void(bool)>& cb)
{
	try
	{

#ifdef _WIN32
#pragma push_macro("new")
#undef new
#endif

		send_req_struct* sr = new (size) send_req_struct(conn);

#ifdef _WIN32
#pragma pop_macro("new")
#endif

		sr->iov.sock = conn->sock;
		sr->iov.code = FD_WRITE;
		sr->iov.conn = conn;

		sr->total_bytes = size;
		sr->sent_bytes = 0;
		sr->sent_cb = cb;
		sr->addref("win_tcp_dispatcher::send"); //external
		sr->addref("win_tcp_dispatcher::send"); //internal
		memcpy(sr->data,buf,size);

		//fwrite( buf, size, 1, conn->send_dump);
		//fflush( conn->send_dump );
		i_messages->push( _P_SEND_REQUEST, (char*)sr );
		cp_r->stop();
		return sr;
	}
	catch( exception& ex ){
		TRACE("<win_tcp_dispatcher::send>", ex.what());
		return 0;
	}
}

void win_tcp_dispatcher::stop_accepting()
{	
	i_messages->push( _P_STOP_ACCEPTING );
	cp_r->stop();

	sem_wait( &stop_accepting_sem);
}

void win_tcp_dispatcher::get_ip_port(tcp_fsm::tcp_conn_t conn, uint32_t &ip, uint16_t &port)
{
	ip = conn->addr.sin_addr.s_addr;
	port = conn->addr.sin_port;
}

void win_tcp_dispatcher::release(send_req_t send_req)
{
	send_req->release("win_tcp_dispatcher::release");
}

void win_tcp_dispatcher::break_send(send_req_t send_req)
{
	send_req->addref("break_send(sem_wait)");

	i_messages->push( _P_BREAK_SEND, (char*)send_req );
	cp_r->stop();

	sem_init(&send_req->break_sem, 0, 0);
	sem_wait(&send_req->break_sem );
	sem_destroy(&send_req->break_sem);
}

void win_tcp_dispatcher::addref(tcp_conn_t conn)
{
	conn->addref("win_tcp_dispatcher::addref");
}






void win_tcp_dispatcher::thread_proc()
{
	int total_sent_bytes = 0;
	int total_recv_bytes = 0;

	try
	{
		TRACE(" dispatcher thread started..");

		transfered_bytes = 0;
		interrupt result;

		IOVERLAPPED* overlap;

		while( 1 ) // it's endless cycle
		{
			transfered_bytes = cp_r->get_transfer(result,overlap);
			if( result.empty() )
			{
				if( bws->is_enable() && !bws->recalc() )
					sendbt();
				continue;
			}

			if( !result.begin()->first ) // internal stop
			{
				assert( i_messages->size() );
				internal_message im = i_messages->pop();

				switch( *im.code )
				{
				case _P_STOP_ACCEPTING:
					accepted = 0;
					sem_post( &stop_accepting_sem);
					break;
				case _P_CREATE_CONNECTION:
					create((tcp_conn_t)im.get());
					break;
				case _P_CLOSE_CONNECTION:
					close((tcp_conn_t)im.get());
					break;
				case _P_CLOSE_CONNECTION_WAIT:
					close_wait((tcp_conn_t)im.get());
					break;
				case _P_SEND_REQUEST:
					send_request((send_req_t)im.get());
					break;
				case _P_BREAK_SEND:
					call_false_cb((send_req_t)im.get());
					break;
				case _P_DO_NOTHING:
					break;
				case _P_EXIT:
					TRACE("i am doing RETURN!");
					free_not_completed_objects();
					return;
				default:
					TRACE(" unknown internal post!");
					break;
				}
			}
			else // external stop
			{
				total_recv_bytes += transfered_bytes;
				switch( result.begin()->second )
				{
				case FD_CLOSE:
					close_connection(overlap->conn,true);
					break;
				case FD_ACCEPT:
					complete(CONTAINING_RECORD(overlap,accept_struct,iov));
					break;
				case FD_CONNECT:
					complete(CONTAINING_RECORD(overlap,overlap_struct,iov));
					break;
				case FD_READ:
					complete(CONTAINING_RECORD(overlap,recv_req_struct,iov));
					break;
				case FD_WRITE:
					complete(CONTAINING_RECORD(overlap,send_req_struct,iov));
					break;
				}
			}
			result.clear();
		}
	}
	catch( exception& ex )
	{
		TRACE("<win_tcp_dispatcher::thread_proc> : ",ex.what());
	}
}






void win_tcp_dispatcher::create(tcp_conn_t conn)
{

	TRACE("_P_CREATE_CONNECTION (",conn->sock, ")");

	conns.push_back(conn);
	conn->addref("waiting4complete");

	overlap_struct *cos = new overlap_struct(conn->sock,FD_CONNECT);
	cos->addref("connect");
	cos->iov.conn = conn;

	int addrlen=sizeof conn->addr;
	if( !lpfnConnectEx(conn->sock,(sockaddr*)&conn->addr,addrlen,0,0,0,&cos->iov) )
	{
		int ierr = WSAGetLastError();
		if( ERROR_IO_PENDING!=ierr )
			throw exception(" WSAConnect failed with error = ", ierr);
	}

	overlaps.push_back(cos);
}

void win_tcp_dispatcher::close(tcp_conn_t conn)
{
	TRACE("_P_CLOSE_CONNECTION (",conn->sock, ")");

	conns.push_back(conn);
	close_connection(conn,false);
	conn->release("close_conn");
}

void win_tcp_dispatcher::close_wait(tcp_conn_t conn)
{
	TRACE("_P_CLOSE_CONNECTION_WAIT (",conn->sock, ")");

	conns.push_back(conn);
	close_connection(conn,false);
	sem_post(&conn->close_sem);
	conn->release("close_conn_wait");
}

void win_tcp_dispatcher::close_connection(tcp_conn_t conn, bool iocp)
{
	switch( conn->state )
	{
	case cs_wait4connect:
		TRACE("i have no called 'connected' callback for this connection.");
		conn->cbs.connected( 0 );
		conn->cbs.closed = 0;
		conn->cbs.connected = 0;
		conn->cbs.read = 0;
		conn->state = cs_closed;

		free_completed_conn(conn,"close_connection");
		//conn->release("close_connection");
		break;
	case cs_connected:
		TRACE(" i am calling 'closed' callback .");
		conn->cbs.closed( conn );
		conn->cbs.closed = 0;
		conn->cbs.connected = 0;
		conn->cbs.read = 0;
		conn->state = cs_closed;

		bws->erase(conn);

		if( SOCKET_ERROR!=conn->sock )
			if( SOCKET_ERROR==closesocket( conn->sock ) )
				TRACE(" closesocket failed with error = ",WSAGetLastError(),".");
		conn->sock = SOCKET_ERROR;

		if( iocp )
		{
			free_completed_recv(conn->recv,"network error");
			free_completed_conn(conn,"close_connection");
		}
			//conn->release("close_connection");
		break;

	case cs_closed:
		if( SOCKET_ERROR!=conn->sock )
			if( SOCKET_ERROR==closesocket( conn->sock ) )
				TRACE(" closesocket failed with error = ",WSAGetLastError(),".");
		conn->sock = SOCKET_ERROR;

		if( iocp )
		{
			free_completed_recv(conn->recv,"network error");
			free_completed_conn(conn,"close_connection");
		}
			//conn->release("close_connection");
		break;
	}
}

void win_tcp_dispatcher::set_bandwidth(uint32_t bytes_per_second)
{
	bws->set_limit(bytes_per_second);
}

bool win_tcp_dispatcher::activate_receiving(tcp_conn_t conn)
{
	conn->recv = new recv_req_struct(conn);
	conn->recv->addref("tcp_conn_t");

	WSABUF wbuf;
	wbuf.buf = (char*)conn->recv->data;
	wbuf.len = 65536;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	int res = WSARecv( conn->sock , &wbuf, 1, &dwBytes, &dwFlags, &conn->recv->iov, NULL);
	int ierr;
	if (SOCKET_ERROR == res)
	{
		ierr = WSAGetLastError();
		switch( ierr )
		{
		case WSA_IO_PENDING:
			TRACE(" WSA_IO_PENDING (activate receiving)");
			break;
		default:
			TRACE(" WSARecv failed with error ",ierr, " (activate receiving)");
			conn->recv->release("wsarecv_error");
			conn->recv = 0;
			return false;
		}
	}

	recvs.push_back( conn->recv );
	return true;
}

void win_tcp_dispatcher::send_request(send_req_t sr)
{
	//if( !do_send(sr) )
	if( bws->is_enable() )
	{
		sends.push_back( sr );
		return;
	}
	if( !do_send(sr) )
		sends.push_back( sr );
}

bool win_tcp_dispatcher::do_send(send_req_t sr,uint32_t* guess)
{
	tcp_conn_t conn = sr->iov.conn;
	if( SOCKET_ERROR==conn->sock ||  conn->state==cs_closed )
	{
		sr->callback(false);
		free_completed_send(sr,"sending's_not_finished.");
		return true;
	}

	WSABUF wbuf;
	wbuf.buf = (char*)sr->data + sr->sent_bytes;
	wbuf.len = sr->total_bytes - sr->sent_bytes;
	if( guess && wbuf.len>*guess ) wbuf.len =*guess;


	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSASend( sr->iov.sock , &wbuf, 1, &dwBytes, dwFlags, &sr->iov, NULL))
	{
		bws->off(conn);
		int ierr = WSAGetLastError();
		switch( ierr )
		{
		case WSA_IO_PENDING:	break;
		case WSAENOTSOCK:
		case WSAENOTCONN:
		case WSAECONNRESET:	
			TRACE(" WSAENOTSOCK | WSAENOTCONN | WSAECONNRESET (send_request)");
			sr->callback(false);
			free_completed_send(sr,"sending's_not_finished.");
			return true;
		default:
			TRACE(" WSASend failed with error = ", WSAGetLastError(), ". (send_request)");
			sr->callback(false);
			free_completed_send(sr,"sending's_not_finished.");
			throw exception(" WSASend failed with error (send_request)", WSAGetLastError());
		}
	}
	if( guess ) *guess -= wbuf.len;
	return false;
}

void win_tcp_dispatcher::sendbt()
{
	if( sends.empty() )
		return;

	//for(sends_t::iterator sit = sends.begin(),end=sends.end();sit!=end;sit++)
	//	*((*sit)->iov.conn)->guess = 0; // ACHTUNG!

	tcp_conn_t conn = 0;
	for(sends_t::iterator sit = sends.begin(),end=sends.end();sit!=end;sit++)
	{
		conn = (*sit)->iov.conn;
		if( 0==*conn->guess )
			*conn->guess = bws->guess(conn);

		if( do_send(*sit,conn->guess) )
			break;
	}
}


void win_tcp_dispatcher::complete(recv_req_struct * rr)
{
	rr->size = transfered_bytes;
	tcp_conn_t conn = rr->iov.conn;
	try
	{
		TRACE(" ", conn->sock, " cbs.read [ size = ", rr->size, " ] from rec_complete(1)");
		if( rr->size && conn->cbs.read )
			conn->cbs.read( conn, rr->data, rr->size );
	}
	catch( exception& ex )
	{
		TRACE("throw_from_read_callback ", ex.what());
		close_connection(conn,true);
		return;
	}

	if( !rr->size )
	{
		close_connection(conn,true);  // from iocp
		return;
	}

	WSABUF wbuf;
	wbuf.buf = rr->data;
	wbuf.len = MAX_WBUF_SIZE;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	int res = WSARecv( rr->iov.sock , &wbuf, 1, &dwBytes, &dwFlags, &rr->iov, NULL);
	if (SOCKET_ERROR == res)
	{
		int ierr = WSAGetLastError();
		switch( ierr )
		{
		case WSA_IO_PENDING:	break;
		default:
			TRACE(" WSARecv failed with error (recv_complete) ", ierr);
			close_connection(conn,false);
		}
	}
}

void win_tcp_dispatcher::complete(send_req_struct* sr)
{
	bws->on(sr->iov.conn);
	bws->insert(sr->iov.conn,transfered_bytes);

	sr->sent_bytes += transfered_bytes;
	if( sr->sent_bytes==sr->total_bytes )
	{
		sr->callback(true);
		free_completed_send(sr,"sending's_done.");
	}
}

void win_tcp_dispatcher::complete(overlap_struct* connect)
{
	tcp_conn_t conn = connect->iov.conn;
	if( 0!=conn->cbs.connected)
	{
		TRACE(" i am calling 'connected' callback.");
		conn->cbs.connected( conn );
		conn->cbs.connected = 0;
		conn->state = cs_connected;
		
		bws->insert(conn,0);

		if( !activate_receiving(conn) )
			close_connection(conn,true);
	}
	free_completed_overlap(connect,"accept_complete");
	free_completed_conn(conn,"accept_complete");
}

void win_tcp_dispatcher::complete(accept_struct* accept)
{
	if( 0!=accepted )
	{
		tcp_conn_t conn = new tcp_conn_struct( accept->iov.sock, callbacks_t());
		conn->addref("accept_complete");
		conn->addref("accept_complete");
		memcpy(&conn->addr,accept->addr_buf+6,sizeof(conn->addr));
		//conn->addr = addr;

		TRACE("i am calling 'accepted' callback.");
		accepted( conn, conn->cbs );
		conn->cbs.connected = 0;
		conn->state = cs_connected;

		bws->insert(conn,0);

		if( !activate_receiving(conn) )
			close_connection(conn,true);
	}
	free_completed_accept(accept,"accept_complete");
	accept_request();
}

void win_tcp_dispatcher::accept_request()
{
	SOCKET asocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (SOCKET_ERROR==asocket)
		throw exception(" WSASocket failed with error = " ,WSAGetLastError());

	accept_struct* accept = new accept_struct(asocket,FD_ACCEPT);
	accept->addref("accept_request");

	DWORD dwBytes = 0;
	if( !lpfnAcceptEx(listening_socket, asocket, accept->addr_buf, 
			0,
			0,
			sizeof(sockaddr_in)+16,
			&dwBytes,
			&accept->iov) )
	{
		int ierr = WSAGetLastError();
		if( ERROR_IO_PENDING!=ierr )
			TRACE("lpfnAcceptEx failed with error %d", ierr);
	}

	accepts.push_back(accept);
	cp_r->add_socket(asocket);
}

void win_tcp_dispatcher::call_false_cb(send_req_t sr)
{
	sr->callback(false);
	sem_post( &sr->break_sem );
	sr->release("break_send(sem_post)");
}










void win_tcp_dispatcher::free_completed_conn(tcp_conn_t conn, const char* rmark)
{
	for( conns_t::iterator it=conns.begin(),end=conns.end();it!=end;it++)
		if( *it==conn )	{ conns.erase( it ); break;	}
	conn->release(rmark);
}

void win_tcp_dispatcher::free_completed_overlap(overlap_struct* overlap, const char* rmark)
{
	for(overlaps_t::iterator it=overlaps.begin(),end=overlaps.end();it!=end;it++)
		if( *it==overlap )	{ overlaps.erase( it ); break;	}
	overlap->release(rmark);
}

void win_tcp_dispatcher::free_completed_accept(accept_struct* accept, const char* rmark)
{
	for(accepts_t::iterator it=accepts.begin(),end=accepts.end();it!=end;it++)
		if( *it==accept )	{ accepts.erase( it ); break;	}
	accept->release(rmark);
}

void win_tcp_dispatcher::free_completed_recv(recv_req_t rr, const char* rmark)
{
	for( recvs_t::iterator it=recvs.begin(),end=recvs.end();it!=end;it++)
		if( *it==rr ) {	recvs.erase( it ); break; }
	rr->release(rmark);
}

void win_tcp_dispatcher::free_completed_send(send_req_t sr, const char* rmark)
{
	for( sends_t::iterator it=sends.begin(),end=sends.end();it!=end;it++)
		if( *it==sr ) { sends.erase( it ); break; }
	sr->release(rmark);
}

void win_tcp_dispatcher::free_not_completed_objects()
{
	for(overlaps_t::iterator it=overlaps.begin(),end=overlaps.end();it!=end;it++)
		((overlap_struct*)*it)->release("iocp_operation_isn't_done");
	for(accepts_t::iterator it=accepts.begin(),end=accepts.end();it!=end;it++)
		((accept_struct*)*it)->release("iocp_operation_isn't_done");

	for( sends_t::iterator it=sends.begin(),end=sends.end();it!=end;it++)
		((send_req_t)*it)->release("iocp_operation_isn't_done");
	for( recvs_t::iterator it=recvs.begin(),end=recvs.end();it!=end;it++)
		((recv_req_t)*it)->release("iocp_operation_isn't_done");

	for(conns_t::iterator it=conns.begin(),end=conns.end();it!=end;it++)
		((tcp_conn_t)*it)->release("waiting_isn't_done(exit)");

	overlaps.clear();
	accepts.clear();
	sends.clear();
	recvs.clear();
	conns.clear();
}




TCP_FSM_EXPORT itcp_dispatcher* make_tcp_dispatcher( uint16_t port, const accepted_cb& accepted )
{
	return new win_tcp_dispatcher(port,accepted);
}

}

