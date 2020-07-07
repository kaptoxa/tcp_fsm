#include "stdafx.h"
#include "tcp_dispatcher.h"

#include <errno.h>
#include <fstream>
#include <fcntl.h>
#include "pthread_stuff.h"



#ifndef _WIN32

uint32_t sockets_counter;

using namespace tcp_fsm;

namespace tcp_fsm
{


struct itcp_dispatcher;
class tcp_dispatcher;

void* PoolProc( void* arg );

void tcp_dispatcher::TRACE(const char* msg,...)
{
	//*log << msg << "\n";
	//log->flush();
}

tcp_dispatcher::tcp_dispatcher(uint16_t listen_port,const accepted_cb& _accepted):
	accepted(accepted)
{
	sockets_counter = 0;

	log.reset( new std::ofstream("dispatcher.log"));
	bws.reset( new bwstop() );
	listening_socket=-1;

	accepted = _accepted;

	try
    {
        if (listen_port)
        {
			listening_socket=::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (-1==listening_socket)
				throw exception(" socket fialed with error = " ,errno);

			if (-1==::fcntl(listening_socket,F_SETFL,O_NONBLOCK))
				throw exception(" fcntl fialed with error = " ,errno);

			sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = 0;
			addr.sin_port = htons(listen_port);
			if (-1==::bind(listening_socket,(sockaddr*)&addr,sizeof addr))
				throw exception(" bind fialed with error = " ,errno);

			//int one=1;
			//if (-1==::setsockopt(listening_socket,SOL_SOCKET,SO_OOBINLINE,&one,sizeof one))
			//{
			//	::close(listening_socket);
			//	throw exception(" setsockopt fialed with error = " ,errno);
			//}

			if (-1==::listen(listening_socket,SOMAXCONN))
				throw exception(" listen fialed with error = " ,errno);
			
			mr.reset( make_multi_reactor() );

			if( int ierr=pthread_create(&hthread, NULL, PoolProc, (void*)this) )
				throw exception(" pthread_create fialed with error = " ,ierr);

			ec.reset( new tcp_conn_struct( listening_socket, callbacks_t()) );
			mr->add_conn(ec.get());			// empty conn for listening

			TRACE("my listening socket is ",listening_socket,".");

			i_messages.reset( new internal_messages() );
        }
    }
    catch ( exception& ex )
    {
		if( -1!=listening_socket ) close(listening_socket);
		TRACE("<tcp_dispatcher::tcp_dispatcher>",ex.what());
		throw; 
    }
}

void* PoolProc( void* arg )
{
	tcp_dispatcher* tcpd = (tcp_dispatcher*)arg;
	if( tcpd )
		tcpd->thread_proc() ;

	return 0L;
}

tcp_dispatcher::~tcp_dispatcher() throw()
{
	i_messages->push( _P_EXIT );
	mr->stop();
	pthread_join( hthread, 0 );

	assert(0==sockets_counter);

	mr.reset(0);
}

tcp_conn_t tcp_dispatcher::make_conn() throw()
{
	SOCKET new_socket = -1;
	try
	{
                new_socket=::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (-1==new_socket)
                        throw exception(" socket fialed with error = " ,errno);

                if( -1==::fcntl(new_socket,F_SETFL,O_NONBLOCK))
                        throw exception(" fcntl fialed with error = " ,errno);

		tcp_conn_t conn = new tcp_conn_struct(new_socket, callbacks_t());
		conn->addref("make_conn");
		conn->addref("make_conn");

		return conn;
	}
	catch( exception& ex ){
		if( -1!=new_socket ) close(new_socket);
		TRACE("<tcp_dispatcher_biocp::connect>",ex.what());
		return 0;
	}
}

void tcp_dispatcher::connect(tcp_conn_t conn, uint32_t ip, uint16_t port, const callbacks_t& cb) throw()
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
                if (-1==::connect(conn->sock,(sockaddr*)&addr,addrlen))
			if (EINPROGRESS!=errno)
				throw exception(" connect fialed with error = " ,errno);

                conn->recv = new recv_req_struct(conn);
                conn->recv->addref("from_connect");

		i_messages->push( _P_CREATE_CONNECTION, (char*)conn );
		mr->stop();
	}
	catch( exception& ex ){
		TRACE("<tcp_dispatcher_biocp::connect>",ex.what());
		return;
	}
}

tcp_conn_t tcp_dispatcher::connect(uint32_t ip, uint16_t port, const callbacks_t& cb) throw()
{
	int new_socket = -1;
	try
	{
		new_socket=::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (-1==new_socket)
			throw exception(" socket fialed with error = " ,errno);

		if( -1==::fcntl(new_socket,F_SETFL,O_NONBLOCK))
			throw exception(" fcntl fialed with error = " ,errno);

		//int one=1;
		//if (-1==::setsockopt(new_socket,SOL_SOCKET,SO_OOBINLINE,&one,sizeof one))
		//{
		//	::close(new_socket);
		//	throw exception(" setsockopt fialed with error = " ,errno);
		//}

		sockaddr_in addr;
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=ip;
		addr.sin_port=htons(port);
		int addrlen=sizeof addr;
		if (-1==::connect(new_socket,(sockaddr*)&addr,addrlen))
			if (EINPROGRESS!=errno)
				throw exception(" connect fialed with error = " ,errno);

		//printf("async connect...\n");

		tcp_conn_t conn = new tcp_conn_struct(new_socket, cb);
		conn->addref("async_connect");
		conn->addref("async_connect");

		conn->addr = addr;

		conn->recv = new recv_req_struct(conn);
		conn->recv->addref("from_connect");



		i_messages->push( _P_CREATE_CONNECTION, (char*)conn );
		mr->stop();

		atomic_inc(&sockets_counter);

		return conn;

	}
	catch( exception& ex ){
		if( -1!=new_socket ) close(new_socket);
		TRACE("<tcp_dispatcher::connect> ",ex.what());
		return 0;
	}
}

void tcp_dispatcher::close_conn(tcp_conn_t conn) throw()
{
	conn->addref("close_conn");

	i_messages->push( _P_CLOSE_CONNECTION, (char*)conn );
	mr->stop();
}

void tcp_dispatcher::close_conn_wait(tcp_conn_t conn) throw()
{
	conn->addref("close_conn_wait");

	i_messages->push( _P_CLOSE_CONNECTION_WAIT, (char*)conn );
	mr->stop();

	sem_wait( &conn->close_sem);
}

void tcp_dispatcher::release(tcp_conn_t conn) throw()
{
	conn->release("tcp_dispatcher::release");
}

void tcp_dispatcher::thread_proc()
{
	int total_sent_bytes = 0;
	int total_recv_bytes = 0;

	try
	{
		TRACE("dispatcher thread started..");

		uint32_t guess = 0;
		tcp_conn_t r_conn = 0;
		interrupt result;
		while( 1 )
		{
			mr->get_interrupt(result);
			for(interrupt::iterator revent=result.begin(),end=result.end();revent!=end;revent++)
			{
				if( !revent->first ) // internal stop
				{
					//assert( i_messages->size() );
					if( !i_messages->size() )
						continue;

					internal_message im = i_messages->pop();


					switch( *im.code )
					{
					case _P_STOP_ACCEPTING:			accepted = 0; printf(" stop accepting.\n"); break;
					case _P_CREATE_CONNECTION:
						{
							tcp_conn_t conn = (tcp_conn_t)im.get();
							//conns.insert( std::make_pair( conn->sock, conn ) );
							conn->addref("waiting4_network_event(connect)");
							create_connection(conn);
							break;
						}
					case _P_CLOSE_CONNECTION:
						{
                                                        tcp_conn_t conn = (tcp_conn_t)im.get();
							close_connection(conn);
							conn->release("close_conn");
							break;
						}
					case _P_CLOSE_CONNECTION_WAIT:
						{
							tcp_conn_t conn = (tcp_conn_t)im.get(); 
							close_connection(conn);
							conn->release("close_conn_wait");
							sem_post( &conn->close_sem );
							break;
						}
					case _P_SEND_REQUEST:
						{
							send_req_t sr = (send_req_t)im.get();
							if( sr->iov.conn->state!=cs_closed )
							{
								sendings_t* sendings = 0;
								outgoings_t::iterator oit = outgoings.find(sr->iov.conn);
								if( oit==outgoings.end() )
								{
									sendings = new sendings_t();
									outgoings.insert( std::make_pair(sr->iov.conn,sendings) );
								}
								else
									sendings = oit->second;

								sendings->push_back(sr);
							}

							break;
						}
					case _P_BREAK_SEND:				call_false_cb((send_req_t)im.get()); break;
					case _P_DO_NOTHING: break;
					case _P_EXIT:
						{
							TRACE(" i am doing RETURN!");
							return;
						}

					default:
						TRACE(" unknown internal post!");
						break;
					}
				}
				else
				{
					r_conn = revent->second.conn;
					if( 0==r_conn )
						continue;

					if( (revent->second.code & FD_CLOSE)  )
					{
						printf(" FD_CLOSE!");
						close_connection( r_conn );
						continue;
					}

					if( (revent->second.code & FD_WRITE)  )
						if( 0!=r_conn->cbs.connected )
						{
							connection_established( r_conn );
							r_conn->release("waiting4_network_connect's_done.");
						}
						else
						{
							bws->on(r_conn);
							if( !bws->is_enable() )
							{
								outgoings_t::iterator oit = outgoings.find(r_conn);
								if( oit!=outgoings.end() )
									for( sendings_t::iterator sit=oit->second->begin(),
									s_end=oit->second->end();sit!=s_end;sit++)
									{
										if( do_send(*sit) )
										{
											oit->second->erase( sit );
											break;
										}
									}
							}
						}

					if( revent->second.code & FD_READ )
						if( listening_socket==r_conn->sock )
						{
							TRACE("it's listening socket.");
							while( 0!=accept_connection() );
						}
						else
						{
							TRACE(" recv (",r_conn->sock,")");
							if( !do_recv(r_conn) )
								close_connection(r_conn);
						}
				}
			}
			if( result.empty() )
			{
				if( !bws->is_enable() )
					for(outgoings_t::iterator oit=outgoings.begin(),end=outgoings.end();oit!=end;oit++)
						for( sendings_t::iterator sit=oit->second->begin(),s_end=oit->second->end();sit!=s_end;sit++)
							if( do_send(*sit) )
							{
								oit->second->erase( sit );
								break;
							}
				if( !bws->recalc() )
					for(outgoings_t::iterator oit=outgoings.begin(),end=outgoings.end();oit!=end;oit++)
					{
						guess = bws->guess(oit->first);
						for( sendings_t::iterator sit=oit->second->begin(),s_end=oit->second->end();sit!=s_end;sit++)
							if( do_send(*sit,&guess) )
							{
								oit->second->erase( sit );
								break;
							}
					}
			}
			result.clear();
		}
	}
	catch( exception& ex )
	{
		TRACE("<tcp_dispatcher::thread_proc> ",ex.what());
		throw;
	}
}

bool tcp_dispatcher::send(tcp_conn_t conn,const void* buf,int size,bool oob) throw()
{
	send_req_t req=send(conn,buf,size,oob,boost::function<void(bool)>());
	if (!req)
		return true;
	req->release("tcp_dispatcher::send");
	return false;
}
/*
void stack_send(const void* data1, int size1,
				const void* data2, int size2)
{
	int total_size = size1 + size2;
	void* buf;
	if (total_size < 1024*2) //2 kb
	{
		buf = alloca(total_size); //stack
	} else
	{
		alocated = true;
		//buf = malloc(total_size); //heap
		send(sock, data1, size1);//
		send(sock, data2, size2);//
	}

	memcpy(buf, data, size1);
	memcpy(buf + size1, data2, size1);

	send(sock, buf, total_size);//

	if (alocated)
		free(buf);
}
*/
send_req_t tcp_dispatcher::send(tcp_conn_t conn,const void* buf,int size,
								bool oob,const boost::function<void(bool)>& cb) throw()
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
		sr->addref("tcp_dispatcher::send"); //external
		sr->addref("tcp_dispatcher::send"); //internal
		memcpy(sr->data,buf,size);

		//fwrite( buf, size, 1, conn->dump);
		//fflush( conn->dump );

		i_messages->push( _P_SEND_REQUEST, (char*)sr );
		mr->stop();

		return sr;
	}
	catch( exception& ex ){
		TRACE("<tcp_dispatcher_biocp::send> ",ex.what());
		return 0;
	}
}

void tcp_dispatcher::stop_accepting() throw()
{	
	i_messages->push( _P_STOP_ACCEPTING );
	mr->stop();
	//buf[0]=6;
}

void tcp_dispatcher::get_ip_port(tcp_conn_t conn, uint32_t &ip, uint16_t &port) throw()
{
	ip = conn->addr.sin_addr.s_addr;
	port = conn->addr.sin_port;
}


void tcp_dispatcher::release(send_req_t send_req) throw()
{
	send_req->release("x3");
	//send_req->release("tcp_dispatcher_biocp::release");
}

void tcp_dispatcher::break_send(send_req_t send_req) throw()
{
	send_req->addref("break_send");
	i_messages->push( _P_BREAK_SEND, (char*)send_req );
	mr->stop();
	//buf[0]=8;

	sem_init(&send_req->break_sem,0,0);
	sem_wait(&send_req->break_sem);
	sem_destroy(&send_req->break_sem);
}

void tcp_dispatcher::addref(tcp_conn_t conn) throw()
{
	conn->addref("tcp_dispatcher::addref");
}

void tcp_dispatcher::set_bandwidth(uint32_t bytes_per_second) throw()
{
	bws->set_limit(bytes_per_second);
}









void tcp_dispatcher::create_connection(tcp_conn_t conn)
{
	TRACE("_P_CREATE_CONNECTION (",conn->sock,")");

	mr->add_conn(conn);
	//mr->add_connect_event( conn->sock );
}

void tcp_dispatcher::close_connection(tcp_conn_t conn)
{
	TRACE("_P_CLOSE_CONNECTION (",conn->sock,")");

	switch( conn->state )
	{
	case cs_wait4connect:
		TRACE(" i have no called 'connected' callback for this connection.");
		conn->cbs.connected( 0 );
		conn->cbs.closed = 0;
		conn->cbs.connected = 0;
		conn->cbs.read = 0;
		conn->state = cs_closed;

		conn->release("waiting4connect's not finished.");
                if( close( conn->sock ) )
                        throw exception(" close(socket) failed with error = ", errno );
		break;
	case cs_connected:
		TRACE(" i am calling 'closed' callback .");
		conn->cbs.closed( conn );
		conn->cbs.closed = 0;
		conn->cbs.connected = 0;
		conn->cbs.read = 0;
		conn->state = cs_closed;

                if( close( conn->sock ) )
                        throw exception(" close(socket) failed with error = ", errno );
		break;

	case cs_closed:
                //if( close( conn->sock ) )
                        //throw exception(" close(socket) failed with error = ", errno );
		return;
	}


	bws->erase(conn);

	//conn->release("close_connection");
	conn->recv->release("close_connection");
	conn->release("close_connection");
	atomic_dec(&sockets_counter);
}

tcp_conn_t tcp_dispatcher::accept_connection()
{
	TRACE(" accept...");
	if( 0==accepted )
		return 0;

	sockaddr_in addr;
	socklen_t addrlen=sizeof addr;
	SOCKET new_socket=::accept(listening_socket,(sockaddr*)&addr,&addrlen);
	if (-1==new_socket)
		switch( errno )
		{
		case EAGAIN: TRACE("EAGAIN"); return 0;
		default:
			throw exception(" accept failed with error ", errno );
		}

	if (-1==::fcntl(new_socket,F_SETFL,O_NONBLOCK))
	{
		::close(new_socket);
		throw exception(" fcntl fialed with error = " ,errno);
	}

	tcp_conn_t conn = new tcp_conn_struct( new_socket, callbacks_t());
	conn->addref("from_accept");
	conn->addref("from_accept");

	conn->addr = addr;

	conn->recv = new recv_req_struct(conn);
	conn->recv->addref("from_accept");

	accepted( conn, conn->cbs );

	mr->add_conn( conn );
	//activate_receiving( conn );

	conn->cbs.connected = 0;
	conn->state = cs_connected;

	bws->insert(conn,0);

	atomic_inc(&sockets_counter);

	TRACE("accept is done.");
	return conn;
}

void tcp_dispatcher::connection_established(tcp_conn_t conn)
{
	TRACE(" connect...");
	if( 0!=conn->cbs.connected)
	{
		//activate_receiving( conn );

		conn->cbs.connected( conn );
		conn->cbs.connected = 0;

		conn->state = cs_connected;
			
		bws->insert(conn,0);

		TRACE("connect is done.");
	}
}

void tcp_dispatcher::activate_receiving(tcp_conn_t conn)
{
	TRACE(" activate receiving...");
}

int tcp_dispatcher::send_request(send_req_t sr)
{
}

void tcp_dispatcher::call_false_cb(send_req_t sr)
{
	if( sr->sent_cb )
	{
		sr->sent_cb(false);
		sr->sent_cb = 0;
	}
	sem_post( &sr->break_sem );
	sr->release("callback_have_just_been_called");
}

void tcp_dispatcher::recv_complete(recv_req_struct* rr)
{
	TRACE("recv_complete");
}

void tcp_dispatcher::send_complete(send_req_struct* ss)
{
	TRACE("send_complete");
}

int tcp_dispatcher::do_recv(tcp_conn_struct* conn)
{
	int transferred_bytes = 0;
	if( 0!=conn->cbs.read )
	{
		int dwFlags = 0;
		if( -1==(transferred_bytes=recv( conn->sock, (char*)conn->recv->data, 65536, dwFlags)) )
		{
			switch( errno )
			{
			case EWOULDBLOCK:	TRACE(" EWOULDBLOCK"); break;
			case EINPROGRESS:	TRACE(" EINPROGRESS"); break;

			default:
				TRACE(" error ",errno);
				transferred_bytes = 0;	// to close connection
				break;

			}
		}
		if( transferred_bytes>0 )
		{
			try
			{
				conn->recv->size = transferred_bytes;
				conn->cbs.read( conn, conn->recv->data, conn->recv->size );
			}
			catch( exception& ex )
			{
				TRACE("throw_from_read_callback",ex.what());
				return 0;				
			}
		}
	}

	return transferred_bytes;
}

bool tcp_dispatcher::do_send(send_req_t sr,uint32_t* guess)
{
	TRACE(" do_send...");

	if( sr->iov.conn->state==cs_closed )
		return true;

	bool done = false;

	int	flags = 0;
	int sent_size = 0;

	int sts = sr->total_bytes-sr->sent_bytes;
	if( guess && sts>*guess ) sts =*guess;

	if( !sts )
		return false;

	sent_size=::send(sr->iov.conn->sock,(char*)&sr->data+sr->sent_bytes,sts,flags);
	if( -1==sent_size )
	{
		bws->off(sr->iov.conn);
		switch( errno )
		{
		case EWOULDBLOCK:	TRACE(" EWOULDBLOCK"); break;
		case EINPROGRESS:	TRACE(" EINPROGRESS"); break;
		default:
			TRACE(" send failed with error = ",errno,".");
			if( sr->sent_cb )
			{
				sr->sent_cb(false);
				sr->sent_cb = 0;
			}
			sr->release("sending's_not_finished.");
			done = true;
		}
	}
	else
	{
		if( guess )
			*guess -= sent_size;
		if( sent_size )
		{
			sr->sent_bytes += sent_size;
			bws->insert(sr->iov.conn,sent_size);
			TRACE("[",sent_size,"] have been sent immediately");
		}
		
		if( sr->sent_bytes==sr->total_bytes )
		{
			TRACE("sending is done.");
			if( sr->sent_cb )
			{
				sr->sent_cb(true);
				sr->sent_cb = 0;
			}
			sr->release("sending's_done.");
			done = true;
		}

		if( sent_size<sts )
			bws->off(sr->iov.conn);
	}
	
	return done;
}

TCP_FSM_EXPORT itcp_dispatcher* make_tcp_dispatcher( uint16_t port, const accepted_cb& accepted )
{
	return new tcp_dispatcher(port,accepted);
}

}

#endif




