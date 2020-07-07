#include "stdafx.h"
#include "itcp_dispatcher.h"
#include "tcp_dispatcher_biocp.h"
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
class tcp_dispatcher_biocp;


static void* Pool4CPProc( void* arg )
{
	tcp_dispatcher_biocp* tcpd = (tcp_dispatcher_biocp*)arg;
	if( tcpd )
		tcpd->thread_proc() ;
	return 0L;
}
static void* lproc( void* arg )
{
	tcp_dispatcher_biocp* tcpd = (tcp_dispatcher_biocp*)arg;
	if( tcpd )
		tcpd->listening_thread() ;
	return 0L;
}

tcp_dispatcher_biocp::tcp_dispatcher_biocp(uint16_t listen_port,const accepted_cb& _accepted):
	accepted(accepted)
{
	log.reset( new std::ofstream("dispatcher.log"));

	listening_socket=-1;

	try
    {
        if (listen_port)
        {
			accepted = _accepted;
			sem_init( &stop_accepting_sem, 0, 0);

			listening_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (SOCKET_ERROR==listening_socket)
				throw exception(" WSASocket fialed with error = " ,WSAGetLastError());

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = 0;
			addr.sin_port = htons(listen_port);
			if (-1==::bind(listening_socket,(sockaddr*)&addr,sizeof addr))
				throw exception(" bind fialed with error = " ,WSAGetLastError());

			if (-1==::listen(listening_socket,SOMAXCONN))
				throw exception(" listen fialed with error = " ,WSAGetLastError());

			wsa_r.reset( make_wsa_reactor() );
			if( int ierr = pthread_create(&lthread, NULL, lproc, (void*)this) )
				throw exception(" pthread_create fialed with error = " , ierr );

			wsa_r->add_event( listening_socket, (int)(FD_ACCEPT | FD_CLOSE));

			iocp_r.reset( new iocp_reactor() );
			if( int ierr = pthread_create(&hthread, NULL, Pool4CPProc, (void*)this) )
				throw exception(" pthread_create fialed with error = " , ierr );

			iocp_r->add_thread( hthread ); // 0?! ...

			i_messages.reset( new internal_messages() );
        }
    }
    catch ( exception& ex )
	{
		if( -1!=listening_socket ) closesocket(listening_socket);
        *log << "<tcp_dispatcher_biocp::tcp_dispatcher_biocp> " << ex.what() << "\n";
		log->flush();
		throw; 
    }
}



tcp_dispatcher_biocp::~tcp_dispatcher_biocp(void)
{
	i_messages->push(_P_EXIT);
	iocp_r->stop();
	pthread_join( hthread, 0 );

	wsa_r.reset(0);
	iocp_r.reset(0);

	sem_destroy( &stop_accepting_sem );
}

tcp_conn_t tcp_dispatcher_biocp::make_conn()
{
	SOCKET new_socket = SOCKET_ERROR;
	try
	{
		new_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (SOCKET_ERROR==new_socket)
			throw exception(" WSASocket failed with error = ", WSAGetLastError());

		unsigned long a=O_NONBLOCK;
		if (SOCKET_ERROR== ioctlsocket(new_socket,FIONBIO,&a) )
			throw exception(" ioctlsocket failed with error = ", WSAGetLastError());

		tcp_conn_t conn = new tcp_conn_struct(new_socket, callbacks_t());
		conn->addref("make_conn");
		conn->addref("make_conn");

		return conn;
	}
	catch( exception& ex ){
		if( SOCKET_ERROR != new_socket ) closesocket(new_socket);
		*log << "<tcp_dispatcher_biocp::connect>" << ex.what() << " \n";
		log->flush();
		return 0;
	}
}

void tcp_dispatcher_biocp::connect(tcp_conn_t conn, uint32_t ip, uint16_t port, const callbacks_t& cb)
{
	try
	{
		sockaddr_in addr;
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=ip;
		addr.sin_port=htons(port);

		conn->addr = addr;
		conn->cbs = cb;

		int addrlen=sizeof addr;
		int ierr=WSAConnect(conn->sock,(sockaddr*)&addr,addrlen,NULL,NULL,NULL,NULL);
		if (SOCKET_ERROR==ierr)
		{
			int ierr = WSAGetLastError();
			switch( ierr )
			{
			case WSAETIMEDOUT: return;
			case WSAEWOULDBLOCK: break;
			default:
				throw exception(" WSAConnect failed with error = ", WSAGetLastError());
			}
		}

		i_messages->push( _P_CREATE_CONNECTION, (char*)conn );
		iocp_r->stop();
	}
	catch( exception& ex ){
		*log << "<tcp_dispatcher_biocp::connect>" << ex.what() << " \n";
		log->flush();
		return;
	}
}

tcp_conn_t tcp_dispatcher_biocp::connect(uint32_t ip, uint16_t port, const callbacks_t& cb)
{
	SOCKET new_socket = SOCKET_ERROR;
	try
	{
		new_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (SOCKET_ERROR==new_socket)
			throw exception(" WSASocket failed with error = ", WSAGetLastError());

		unsigned long a=O_NONBLOCK;
		if (SOCKET_ERROR== ioctlsocket(new_socket,FIONBIO,&a) )
			throw exception(" ioctlsocket failed with error = ", WSAGetLastError());

		sockaddr_in addr;
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=ip;
		addr.sin_port=htons(port);
		int addrlen=sizeof addr;

		int ierr=WSAConnect(new_socket,(sockaddr*)&addr,addrlen,NULL,NULL,NULL,NULL);
		if (SOCKET_ERROR==ierr)
		{
			int ierr = WSAGetLastError();
			switch( ierr )
			{
			case WSAETIMEDOUT: return 0;
			case WSAEWOULDBLOCK: break;
			default:
				throw exception(" WSAConnect failed with error = ", WSAGetLastError());
			}
		}

		tcp_conn_t conn = new tcp_conn_struct(new_socket, cb);
		conn->addref("async_connect");
		conn->addref("async_connect");

		conn->addr = addr;

		i_messages->push( _P_CREATE_CONNECTION, (char*)conn );
		iocp_r->stop();

		return conn;
	}
	catch( exception& ex ){
		if( SOCKET_ERROR != new_socket ) closesocket(new_socket);
		*log << "<tcp_dispatcher_biocp::connect>" << ex.what() << " \n";
		log->flush();
		return 0;
	}
}

void tcp_dispatcher_biocp::close_conn(tcp_conn_t conn)
{
	conn->addref("close_conn");
	i_messages->push( _P_CLOSE_CONNECTION, (char*)conn );
	iocp_r->stop();
}

void tcp_dispatcher_biocp::close_conn_wait(tcp_conn_t conn)
{
	conn->addref("close_conn_wait");
	i_messages->push( _P_CLOSE_CONNECTION_WAIT, (char*)conn );
	iocp_r->stop();

	sem_wait( &conn->close_sem);
}

void tcp_dispatcher_biocp::release(tcp_conn_t conn)
{
	conn->release("tcp_dispatcher_biocp::release");
}

void tcp_dispatcher_biocp::listening_thread()
{
	try
	{
		interrupt result;
		while( wsa_r->get_interrupt(result) )
		{
			for(interrupt::iterator revent=result.begin(),end=result.end();revent!=end;revent++)
			{
				if( -1==revent->first )
					return ;

				if(  revent->first==listening_socket &&
					 revent->second==0 )
					break;

				external_struct* em =  new external_struct( revent->first, revent->second );
				i_messages->push( _P_EXTERNAL, (char*)em );
				iocp_r->stop();
			}
			result.clear();
		}
	}
	catch( exception& ex )
	{
        *log << "<tcp_dispatcher_biocp::thread_proc>! <<" << ex.what() << "\n";
		log->flush();
	}
}

void tcp_dispatcher_biocp::thread_proc()
{
	int total_sent_bytes = 0;
	int total_recv_bytes = 0;

	try
	{
		*log << " dispatcher thread started..\n"; log->flush();

		int transfered_bytes = 0;
		interrupt result;

		send_req_t send_req = 0;
		recv_req_t recv_req = 0;
		tcp_conn_t r_conn = 0;

		while( 1 ) // it's endless cycle
		{
			transfered_bytes = iocp_r->get_transfer(result,r_conn,send_req,recv_req);
			if( !result.begin()->first ) // internal stop
			{
				assert( i_messages->size() );
				internal_message im = i_messages->pop();

				switch( *im.code )
				{
				case _P_STOP_ACCEPTING:
					accepted = 0; //printf(" stop accepting.\n");
					sem_post( &stop_accepting_sem);
					break;
				case _P_CREATE_CONNECTION:
					{
						tcp_conn_t conn = (tcp_conn_t)im.get();
						conns.push_back(conn);
						conn->addref("waiting4complete");
						create_connection(conn);
						break;
					}
				case _P_CLOSE_CONNECTION:
					{
						tcp_conn_t conn = (tcp_conn_t)im.get(); 
						conns.push_back(conn);
						close_connection(conn);
						conn->release("close_conn");
						break;
					}
				case _P_CLOSE_CONNECTION_WAIT:
					{
						tcp_conn_t conn = (tcp_conn_t)im.get(); 
						conns.push_back(conn);
						close_connection(conn);
						conn->release("close_conn_wait");
						sem_post( &conn->close_sem );
						break;
					}
				case _P_SEND_REQUEST:			send_request((send_req_t)im.get());	break;
				case _P_BREAK_SEND:				call_false_cb((send_req_t)im.get()); break;
				case _P_DO_NOTHING: break;
				case _P_EXIT:
					{
						*log << " i am doing RETURN!\n"; log->flush();
						free_not_completed_objects();

						wsa_r->stop(-1);	// to stop listening threads ( 2! )
						pthread_join( lthread, 0 );
						return;
					}



				case _P_EXTERNAL:
					{
						external_struct* em =  (external_struct*)im.get();
						switch( em->events )
						{
						case FD_ACCEPT:		accept_connection();
						case FD_CONNECT: 
							{
								for( conns_t::iterator it=conns.begin(),end=conns.end();it!=end;it++)
									if( (*it)->sock==em->sock )
									{
										connection_established( *it );
										(*it)->release("waiting_is_finished(FD_CONNECT)");
										conns.erase(it);
										break;
									}
								break;
							}
						case FD_CLOSE:
							{
								for( conns_t::iterator it=conns.begin(),end=conns.end();it!=end;it++)
									if( (*it)->sock==em->sock )
									{
										close_connection( *it,true );
										(*it)->release("waiting_is_finished(FD_CLOSE)");
										conns.erase(it);
										break;
									}
								break;
							}
						}
						delete em;
						break;
					}
				default:

					*log << " unknown internal post! \n";
					log->flush();
					break;
				}
			}
			else // external stop
			{
				switch( result.begin()->second )
				{
				case FD_CLOSE:
					//r_conn->recv->release("iocp_error");
					close_connection(r_conn,true);
					break;

				case FD_READ:
					{
						total_recv_bytes += transfered_bytes;
						recv_complete( recv_req );
						break;
					}
				case FD_WRITE:
					{
						total_sent_bytes += transfered_bytes;
						send_complete( send_req );
						break;
					}
				}
			}

			result.clear();
		}
	}
	catch( exception& ex )
	{
		*log << "<tcp_dispatcher_biocp::thread_proc> : " << ex.what() << "!\n";
		log->flush();

		wsa_r->stop(-1);	// to stop listening threads ( 2! )
		pthread_join( lthread, 0 );
	}
}

bool tcp_dispatcher_biocp::send(tcp_conn_t conn,const void* buf,int size,bool oob)
{
	send_req_t req=send(conn,buf,size,oob,boost::function<void(bool)>());
	req->release("tcp_dispatcher_biocp::send");
	return true;
}

send_req_t tcp_dispatcher_biocp::send(tcp_conn_t conn,const void* buf,int size,
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
		//sr->conn = conn;
		sr->total_bytes = size;
		sr->sent_bytes = 0;
		sr->sent_cb = cb;
		sr->addref("tcp_dispatcher_biocp::send"); //external
		sr->addref("tcp_dispatcher_biocp::send"); //internal
		memcpy(sr->data,buf,size);

		//fwrite( buf, size, 1, conn->send_dump);
		//fflush( conn->send_dump );

		i_messages->push( _P_SEND_REQUEST, (char*)sr );
		iocp_r->stop();

		return sr;
	}
	catch( exception& ex ){
		*log << "<tcp_dispatcher_biocp::send> " << ex.what() << "\n";
		log->flush();
		return 0;
	}
}

void tcp_dispatcher_biocp::stop_accepting()
{	
	i_messages->push( _P_STOP_ACCEPTING );
	iocp_r->stop();

	sem_wait( &stop_accepting_sem);
}

void tcp_dispatcher_biocp::get_ip_port(tcp_fsm::tcp_conn_t conn, uint32_t &ip, uint16_t &port)
{
	ip = conn->addr.sin_addr.s_addr;
	port = conn->addr.sin_port;
}

void tcp_dispatcher_biocp::release(send_req_t send_req)
{
	send_req->release("tcp_dispatcher_biocp::release");
}

void tcp_dispatcher_biocp::break_send(send_req_t send_req)
{
	send_req->addref("break_send(sem_wait)");

	i_messages->push( _P_BREAK_SEND, (char*)send_req );
	iocp_r->stop();

	sem_wait( &send_req->break_sem );
}

void tcp_dispatcher_biocp::addref(tcp_conn_t conn)
{
	conn->addref("tcp_dispatcher_biocp::addref");
}






void tcp_dispatcher_biocp::create_connection(tcp_conn_t conn)
{
	*log << "_P_CREATE_CONNECTION (" << conn->sock << ")\n"; log->flush();

	wsa_r->add_event(conn->sock,(FD_CONNECT | FD_CLOSE));
	wsa_r->stop();

}

void tcp_dispatcher_biocp::close_connection(tcp_conn_t conn, bool iocp)
{
	*log << "_P_CLOSE_CONNECTION (" << conn->sock << ")\n"; log->flush();

	switch( conn->state )
	{
	case cs_wait4connect:
		*log << " i have no called 'connected' callback for this connection.\n"; log->flush();
		conn->cbs.connected( 0 );
		conn->cbs.closed = 0;
		conn->cbs.connected = 0;
		conn->cbs.read = 0;
		conn->state = cs_closed;

		wsa_r->del_event( conn->sock );
		wsa_r->stop();

		free_completed_conn(conn,"close_connection");
		//conn->release("close_connection");
		break;
	case cs_connected:
		*log << " i am calling 'closed' callback .\n"; log->flush();
		conn->cbs.closed( conn );
		conn->cbs.closed = 0;
		conn->cbs.connected = 0;
		conn->cbs.read = 0;
		conn->state = cs_closed;

		if( SOCKET_ERROR!=conn->sock )
			if( SOCKET_ERROR==closesocket( conn->sock ) )
				*log << " closesocket failed with error = " << WSAGetLastError() << ".\n", log->flush();
		conn->sock = SOCKET_ERROR;

		if( iocp )
			free_completed_conn(conn,"close_connection");
			//conn->release("close_connection");
		break;

	case cs_closed:
		if( SOCKET_ERROR!=conn->sock )
			if( SOCKET_ERROR==closesocket( conn->sock ) )
				*log << " closesocket failed with error = " << WSAGetLastError() << ".\n", log->flush();
		conn->sock = SOCKET_ERROR;

		if( iocp )
			free_completed_conn(conn,"close_connection");
			//conn->release("close_connection");
		break;
	}
}

tcp_conn_t tcp_dispatcher_biocp::accept_connection()
{
	if( 0==accepted )
		return 0;

	sockaddr_in addr;
	int addrlen=sizeof addr;
	SOCKET new_socket=WSAAccept(listening_socket,(sockaddr*)&addr,&addrlen,NULL,NULL);
	if (SOCKET_ERROR==new_socket)
	{
		int ierr = WSAGetLastError();
		switch( ierr )
		{
		case WSAEWOULDBLOCK:	
			*log << " WSAEWOULDBLOCK\n";log->flush();
			return 0;
		default:
			throw exception(" WSAAccept failed with error = ", WSAGetLastError());
		}
	}

	tcp_conn_t conn = new tcp_conn_struct( new_socket, callbacks_t());
	conn->addref("from_accept");
	conn->addref("from_accept");

	conn->addr = addr;

	*log << " i am calling 'accepted' callback.\n"; log->flush();
	accepted( conn, conn->cbs );

	iocp_r->add_conn( conn );
	conn->cbs.connected = 0;
	conn->state = cs_connected;

	if( activate_receiving(conn) )
	{
		*log << "accept is done.\n"; log->flush();
		return conn;
	}

	close_connection(conn,true); // not from iocp, but we have no activated receiving
	return 0;
}

void tcp_dispatcher_biocp::connection_established(tcp_conn_t conn)
{
	*log << " connect....\n"; log->flush();
	if( 0!=conn->cbs.connected)
	{
		wsa_r->del_event( conn->sock );
		wsa_r->stop();

		*log << " i am calling 'connected' callback.\n"; log->flush();
		conn->cbs.connected( conn );
		conn->cbs.connected = 0;
		conn->state = cs_connected;

		iocp_r->add_conn( conn );

		if( activate_receiving( conn ) )
		{
			*log << "connect is done.\n"; log->flush();
			return;
		}

		*log << "connect isn't done.\n"; log->flush();
		close_connection(conn,true); // not from iocp, but we have no activated receiving
	}
}

bool tcp_dispatcher_biocp::activate_receiving(tcp_conn_t conn)
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
		case WSA_IO_PENDING:	*log << " WSA_IO_PENDING (activate receiving)\n"; break;
		default:
			*log << " WSARecv failed with error " << ierr << " (activate receiving)\n";
			conn->recv->release("wsarecv_error");
			conn->recv = 0;
			return false;
		}
		log->flush();
	}

	// rewrite everything to the way using AcceptEx !

	recvs.push_back( conn->recv );
	return true;
}

void tcp_dispatcher_biocp::send_request(send_req_t sr)
{
	if( SOCKET_ERROR==sr->iov.conn->sock )
	{
		sr->callback(false);
		sr->release("sending's_not_finished.");
		return;
	}

	WSABUF wbuf;
	wbuf.buf = (char*)sr->data;
	wbuf.len = sr->total_bytes;

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSASend( sr->iov.sock , &wbuf, 1, &dwBytes, dwFlags, &sr->iov, NULL))
	{
		int ierr = WSAGetLastError();
		switch( ierr )
		{
		case WSA_IO_PENDING:	break;//*log << " WSA_IO_PENDING (send_request)\n"; break;
		case WSAENOTSOCK:
		case WSAENOTCONN:
		case WSAECONNRESET:	
			*log << " WSAENOTSOCK | WSAENOTCONN | WSAECONNRESET (send_request)\n";
			sr->callback(false);
			sr->release("sending's_not_finished.");
			break;
		default:
			*log << " WSASend failed with error = " << WSAGetLastError() << ". (send_request)\n";
			sr->callback(false);
			sr->release("sending's_not_finished.");
			throw exception(" WSASend failed with error (send_request)", WSAGetLastError());
		}
		log->flush();
	}

	sends.push_back( sr );
}

void tcp_dispatcher_biocp::recv_complete(recv_req_struct * rr)
{
	tcp_conn_t conn = rr->iov.conn;
	try
	{
		*log << " " << conn->sock << " cbs.read [ size = " <<  rr->size << " ] from rec_complete(1)\n"; log->flush();
		if( rr->size && conn->cbs.read )
			conn->cbs.read( conn, rr->data, rr->size );
	}
	catch( exception& ex )
	{
		*log << "throw_from_read_callback" << ex.what() <<"\n"; log->flush();
		free_completed_recv(rr,"from_read_callback");
		close_connection(conn,true);
		return;
	}

	if( !rr->size )
	{
		free_completed_recv(rr,"wsarecv 0");
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
		case WSA_IO_PENDING:	break;//*log << " WSA_IO_PENDING (recv_complete)\n"; break;
		default:
			*log << " WSARecv failed with error (recv_complete) " <<  ierr << "\n"; log->flush();
			free_completed_recv(rr,"wsarecv_error");
			close_connection(conn,false);
		}
	}
}

void tcp_dispatcher_biocp::send_complete(send_req_struct* sr)
{
	if( sr->sent_bytes < sr->total_bytes )
	{
		WSABUF wbuf;
		wbuf.buf = (char*)(sr->data + sr->sent_bytes);
		wbuf.len = sr->total_bytes - sr->sent_bytes;

		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		if (SOCKET_ERROR == WSASend( sr->iov.sock, &wbuf, 1, &dwBytes, dwFlags, &sr->iov, NULL))
		{
			int ierr = WSAGetLastError();
			switch( ierr )
			{
			case WSA_IO_PENDING:	*log << " WSA_IO_PENDING (send_complete)\n"; break;
			case WSAENOTSOCK:
			case WSAENOTCONN:
			case WSAECONNRESET:	
				*log << " WSAENOTSOCK | WSAENOTCONN | WSAECONNRESET (send_complete)\n";
				sr->callback(false);
				free_completed_send(sr,"sending's_not_finished.");
				break;
			default:
				sr->callback(false);
				free_completed_send(sr,"sending's_not_finished.");
				throw exception(" WSASend failed with error (send_complete)", WSAGetLastError());
			}
			log->flush();
		}
	}
	else
	{
		//*log << ">" << sr->sent_bytes << "\n"; log->flush(); // << str << "\n"
		sr->callback(true);
		free_completed_send(sr,"sending's_done.");
	}
}

void tcp_dispatcher_biocp::call_false_cb(send_req_t sr)
{
	sr->callback(false);
	sem_post( &sr->break_sem );
	sr->release("break_send(sem_post)");
}


void tcp_dispatcher_biocp::free_completed_conn(tcp_conn_t conn, const char* rmark)
{
	for( conns_t::iterator it=conns.begin(),end=conns.end();it!=end;it++)
		if( *it==conn )
		{
			conns.erase( it );
			break;
		}
	conn->release(rmark);
}

void tcp_dispatcher_biocp::free_completed_recv(recv_req_t rr, const char* rmark)
{
	for( recvs_t::iterator it=recvs.begin(),end=recvs.end();it!=end;it++)
		if( *it==rr )
		{
			recvs.erase( it );
			break;
		}
	rr->release(rmark);
}

void tcp_dispatcher_biocp::free_completed_send(send_req_t sr, const char* rmark)
{
	for( sends_t::iterator it=sends.begin(),end=sends.end();it!=end;it++)
		if( *it==sr )
		{
			sends.erase( it );
			break;
		}
	sr->release(rmark);
}

void tcp_dispatcher_biocp::free_not_completed_objects()
{
	for( sends_t::iterator it=sends.begin(),end=sends.end();it!=end;it++)
		((send_req_t)*it)->release("iocp_operation_has_not_been_done");
	sends.clear();

	for( recvs_t::iterator it=recvs.begin(),end=recvs.end();it!=end;it++)
		((recv_req_t)*it)->release("iocp_operation_has_not_been_done");
	recvs.clear();

	for(conns_t::iterator it=conns.begin(),end=conns.end();it!=end;it++)
		((tcp_conn_t)*it)->release("waiting_isn't_done(exit)");
	conns.clear();
}




//TCP_FSM_EXPORT itcp_dispatcher* make_tcp_dispatcher( uint16_t port, const accepted_cb& accepted )
//{
//	return new tcp_dispatcher_biocp(port,accepted);
//}

}
