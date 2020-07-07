
#include "stdafx.h"
#include "imulti_reactor.h"
#include "multi_reactor.h"
#include "pthread_stuff.h"
#include "connection.h"

#include <errno.h>
#include <fstream>
#include <fcntl.h>


//#ifdef HAVE_EPOLL_CREATE

namespace tcp_fsm
{

linux_reactor::linux_reactor()
{
	log.reset( new std::ofstream("multi_reactor.log"));

	try
	{
		if (pipe(pipe_fds) == 0)
		{
			::fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);
			::fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK);
		}

		if( -1==(epfd=epoll_create( 1 )) )
			throw exception(" epoll_create failed with error  ", errno);

		epoll_event ev = { 0, { 0 } };
		ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
		ev.data.fd = pipe_fds[0];

		if( -1==epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fds[0], &ev) )
			throw exception(" epoll_ctl failed with error  ", errno);
	}
	catch ( exception& ex ) {
		*log << "error<multi_reactor::multi_reactor>" << ex.what() << "\n";
		log->flush();
		throw;
	}
}

linux_reactor::~linux_reactor()
{
	if( -1!=pipe_fds[0] )
		::close( pipe_fds[0] );
	if( -1!=pipe_fds[1] )
		::close( pipe_fds[1] );

	close( epfd );

	*log << " multi_reactor::~multi_reactor()\n";
	log->flush();
}

void linux_reactor::add_thread(pthread_t bthread)
{
	threads.push_back( bthread );
}

void linux_reactor::add_conn(tcp_conn_t conn)
{
	try
	{
		epoll_event ev = { 0, { 0 } };
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
		ev.data.ptr = conn;

		if( -1==epoll_ctl( epfd, EPOLL_CTL_ADD, conn->sock, &ev))
			throw exception(" epoll_ctl failed with error  ", errno);

		*log << " new event (" << conn->sock << ")\n";
	}
	catch( exception& ex ) {
		*log << "<multi_reactor::add_conn>" << ex.what() <<"\n";
		log->flush();
		throw;
	}
}

void linux_reactor::add_event(int socket, int type)
{
	try
	{
		epoll_event ev = { 0, { 0 } };
		ev.events = EPOLLIN | EPOLLOUT ;
		ev.data.fd = socket;

		if( -1==epoll_ctl( epfd, EPOLL_CTL_ADD, socket, &ev))
			throw exception(" epoll_ctl failed with error  ", errno);

	}
	catch( exception& ex ) {
		*log << "<multi_reactor::add_event>" << ex.what() <<"\n";
		log->flush();
		throw;
	}
}

void linux_reactor::del_event(int socket)
{
	try
    {
		epoll_event ev = { 0, { 0 } };
		ev.events = EPOLLIN | EPOLLOUT;
		ev.data.fd = socket;

		if( -1==epoll_ctl( epfd, EPOLL_CTL_DEL, socket, &ev))
			throw exception(" epoll_ctl failed with error  ", errno);
    }
    catch( exception& ex ) {
		*log << "<multi_reactor::add_event>" << ex.what() <<"\n";
		log->flush();
		throw;
    }
}

bool linux_reactor::get_interrupt(interrupt& result)
{
	
	try
	{
		epoll_event events[128];

lb_ag:
		int res = epoll_wait( epfd, events, 128, 10);
		if( -1==res )
		{
			if (errno == EINTR)
				goto lb_ag;
			throw exception(" epoll_wait failed with error  ", errno);
		}

		int isocket = 0;
		int ievents = 0;
		for( int i=0; i<res; i++ )
		{
			isocket = ievents = 0;
			*log << ".\n"; log->flush();
			what_happened(events[i].events);       

			interrupt_info info;
					
			if( events[i].data.fd == pipe_fds[0] )
			{
				::read( events[i].data.fd, &isocket, sizeof(isocket));
				info.code = (int)ievents;
			}
			else
			{
				info.conn = (tcp_conn_t)events[i].data.ptr;
				isocket = info.conn->sock;

				if( events[i].events & EPOLLIN ) ievents |= FD_READ;
				if( events[i].events & EPOLLOUT ) ievents |= FD_WRITE;
				if( events[i].events & EPOLLERR ) ievents |= FD_CLOSE;
				info.code = (int)ievents;

				*log << ":" << isocket << ":\n"; log->flush();
			}
			result.insert( std::make_pair(isocket,info) );
		}

		return 0!=res;
	}
	catch( exception& ex ) {
		*log << "error<multi_reactor::get_interrupt>" << ex.what() <<"\n";
		log->flush();
		throw;
	}
}

void linux_reactor::stop(int socket)
{
	// it is unnecessary for windows

	for(std::vector<pthread_t>::iterator it=threads.begin(),end=threads.end();it!=end;it++)
		if( pthread_equal( pthread_self(), *it ) )
			return;

	::write( pipe_fds[1], &socket, sizeof(int));
}

void linux_reactor::what_happened(int events)
{
	if( events & EPOLLIN ) *log << " + EPOLLIN" ;
	if( events & EPOLLOUT ) *log << " + EPOLLOUT";
	//if( events & EPOLLRDHUP ) *log << " + EPOLLRDHUP" ;
	if( events & EPOLLPRI ) *log << " + EPOLLPRI";
	if( events & EPOLLERR ) *log << " + EPOLLERR" ;
	if( events & EPOLLHUP ) *log << " + EPOLLHUP";
	if( events & EPOLLET ) *log << " + EPOLLET" ;
	if( events & EPOLLONESHOT ) *log << " + EPOLLONESHOT";

	*log << "\n"; log->flush();
}

imulti_reactor* make_multi_reactor() {
	return new linux_reactor();
}

}

//#endif
