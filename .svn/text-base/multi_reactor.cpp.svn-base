
#include "stdafx.h"



#ifdef _WIN32
#elif HAVE_EPOLL_CREATE
#elif _FreeBSD
#else

socket_reactor::socket_reactor()
{
	log.reset( new std::ofstream("multi_reactor.log"));

	try
	{
		int dummy_listener;
		dummy_listener = pipe_fds[1] = pipe_fds[0] = -1;
		//create dummu sockets:
		dummy_listener=(int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (0==dummy_listener)
			throw std::exception();
		pipe_fds[1]=(int)socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (0==dummy_listener)
			throw std::exception();

		sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		addr.sin_family = PF_INET;
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		addr.sin_port = 0;
		int res=bind(dummy_listener,(sockaddr*)&addr,addr_len);
		if (-1==res)
			throw std::exception();
		res=getsockname(dummy_listener,(sockaddr*)&addr,&addr_len);
		if (-1==res)
			throw std::exception();
		res=listen(dummy_listener,0x7fffffff);
		if (-1==res)
			throw std::exception();

		res=connect(pipe_fds[1],(sockaddr*)&addr,addr_len);
		if (-1==res)
			throw std::exception();
		pipe_fds[0]=(int)accept(dummy_listener,(sockaddr*)&addr,&addr_len);
		if (-1==pipe_fds[0])
			throw std::exception();

		::fcntl(pipe_fds[0],F_SETFL,O_NONBLOCK);

		if (-1==res)
		close(dummy_listener);
	}
	catch (std::exception& ex) {
		*log << "error<multi_reactor::multi_reactor>" << ex.what() << "\n";
		log->flush();
		throw std::exception();
	}
}

socket_reactor::~socket_reactor()
{
	if( -1!=pipe_fds[0] )
		::close( pipe_fds[0] );
	if( -1!=pipe_fds[1] )
		::close( pipe_fds[1] );

        //for( desc_set_t::iterator it=desc_set.begin(),end=desc_set.end();it!=end;it++)
        //        ::close(*it);
        r_desc_set.clear();
		w_desc_set.clear();

	*log << " multi_reactor::~multi_reactor()\n";
	log->flush();
}


void socket_reactor::add_event(int socket, int type)
{
	if( type & FD_ACCEPT )
		r_desc_set.insert( socket );
	else
		w_desc_set.insert( socket );
}

bool socket_reactor::get_interrupt()
{
	try
	{
		fd_set rset,wset,eset;
		FD_ZERO(&rset); FD_ZERO(&wset); FD_ZERO(&eset);

		int max=pipe_fds[0];
		FD_SET(pipe_fds[0],&rset);
		
		for( desc_set_t::iterator it=r_desc_set.begin(),end=r_desc_set.end();it!=end;it++)
		{
			FD_SET(*it,&rset);
			FD_SET(*it,&eset);
			if( *it>max ) max = *it;
		}

		for( desc_set_t::iterator it=w_desc_set.begin(),end=w_desc_set.end();it!=end;it++)
		{
			FD_SET(*it,&wset);
			FD_SET(*it,&eset);
			if( *it>max ) max = *it;
		}

	    *log << " achtung! it's going to call select!\n"; log->flush();
		int res=select(max+1,&rset,&wset,&eset,0);
		if (-1==res)
		{
			*log << " errno = " << errno << ".\n";
			throw std::exception();
		}
		*log << " select has return " << res << " something.\n"; log->flush();


		if( FD_ISSET(pipe_fds[0],&rset) )
			::read( pipe_fds[0], &ready_socket, sizeof(ready_socket));

		for( desc_set_t::iterator it=desc_set.begin(),end=desc_set.end();it!=end;it++)
			if( FD_ISSET( *it, &rset ) )
			{
				ready_socket = *it;

				//char buf[1024];
				//recv( ready_socket, buf, sizeof(buf), 0 );

				//FD_CLR( *it, &rset );				

				close( *it );
				desc_set.erase( *it );
			}

		return 0!=res;
	}
	catch( std::exception& ex) {
		*log << "error<multi_reactor::get_interrupt>" << ex.what() <<"\n";
		log->flush();
		throw std::exception();
	}
}

void socket_reactor::interrupt(int socket)
{
	*log << " interrupt on " << socket << " socket.\n"; log->flush();
	char byte = 0;
	::send( pipe_fds[1] ,(const char*) &byte,(int)1,0);
}

imulti_reactor* make_multi_reactor()
{
	return new socket_reactor();
}

#endif

