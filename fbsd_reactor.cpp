#include "stdafx.h"
#include "multi_reactor.h"
#include "pthread_stuff.h"

#include <fstream>



#ifdef _FreeBSD

fbsd_reactor::fbsd_reactor()
{
	log.reset( new std::ofstream("multi_reactor.log"));

	try
	{
        if (pipe(pipe_fds) == 0)
        {
            ::fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);
            ::fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK);
        }

		if( -1==(kqfd=kqueue()) )
		{
			*log << " kqueue failed with error " << errno << ".\n";
			throw std::exception();
		}

		struct kevent event;
		EV_SET( &event, pipe_fds[0], EVFILT_READ, EV_ADD, 0, 0, 0);
		if( -1==(kevent( kqfd, &event, 1, 0,0,0)) )
		{
			*log << " kevent failed with error " << errno << ".\n";
			throw std::exception();
		}
	}
	catch (std::exception& ex) {
		*log << "error<multi_reactor::multi_reactor>" << ex.what() << "\n";
		log->flush();
		throw std::exception();
	}
}

fbsd_reactor::~fbsd_reactor()
{
	if( -1!=pipe_fds[0] )
		::close( pipe_fds[0] );
	if( -1!=pipe_fds[1] )
		::close( pipe_fds[1] );

	//for( desc_set_t::iterator it=desc_set.begin(),end=desc_set.end();it!=end;it++)
	//	::close(*it);
	desc_set.clear();
	
	close( kqfd );

	*log << " multi_reactor::~multi_reactor()\n";
	log->flush();
}


void fbsd_reactor::add_event(int socket, int type)
{
	try
	{
		struct kevent events;
		EV_SET( &events, socket, EVFILT_READ, EV_ADD | EV_ENABLE , 0, 0, 0);
		if( -1==(kevent( kqfd, &events, 1, 0,0,0)) )
		{
			*log << " kevent failed with error " << errno << ".\n";
			throw std::exception();
		}

		desc_set.insert( socket );
	}
	catch( std::exception& ex ) {
		*log << "<multi_reactor::add_event>" << ex.what() <<"\n";
		log->flush();
		throw std::exception();
	}
}

bool fbsd_reactor::get_interrupt()
{
	try
	{
		*log << " achtung! it's going to call kevent!\n";
        log->flush();

        struct kevent events[128];
        int res = kevent( kqfd, 0, 0, events, 128, NULL);
        if( -1==res )
        {
            *log << errno <<  " kevent! \n";
            throw std::exception();
        }

        for( int i=0; i<res; i++ )
    	{
			int socket  = 0;
			if( events[i].ident == pipe_fds[0] )
			{
				*log << " kevent from pipe.\n"; log->flush();
				::read( events[i].ident, &socket, sizeof(socket));
			}
			else
			{
				*log << " kevent from socket with event " << events[i].fflags  <<".\n"; log->flush();

				//char buf[1024];
				socket = (int)events[i].ident ;
				//recv( socket, buf, sizeof(buf), 0 );

                //struct kevent event;
        		EV_SET( &events[i], socket, EVFILT_READ, EV_DELETE , 0, 0, 0);
				int res=kevent( kqfd, &events[i], 1, 0,0,0);
				if( -1==res && 2!=errno )  
                {
                    *log << " kevent failed with error " << errno << ".\n";
                    throw std::exception();
                }
			}

    	    *log << " socket = " << socket << "\n"; log->flush();
            isocket = socket;
        }

        return 0!=res;
	}
	catch( std::exception& ex) {
		*log << "error<multi_reactor::get_interrupt>" << ex.what() <<"\n";
		log->flush();
		throw std::exception();
	}
}

void fbsd_reactor::interrupt(int socket)
{
	*log << " interrupt on " << socket << " socket.\n"; log->flush();
	::write( pipe_fds[1], &socket, sizeof(int));
}

imulti_reactor* make_multi_reactor() {
	return new fbsd_reactor();
}

#endif