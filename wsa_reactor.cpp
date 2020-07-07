
#include "stdafx.h"
#include "multi_reactor.h"
#include "pthread_stuff.h"
#include "exception.h"

#include <fstream>


#ifdef _WIN32


#if defined(_DEBUG) && defined(_WIN32)
#define new DEBUG_NEW
#endif

namespace tcp_fsm
{

wsa_reactor::wsa_reactor()
{
	isocket = SOCKET_ERROR;
	try
	{
		pthread_mutex_init( &wait_events_mutex, 0 );
		wait_events.reset( new wait_events_t() );

		hInterruptEvent = CreateEvent( NULL, TRUE, FALSE, 0 );
		hExitEvent = CreateEvent( NULL, TRUE, FALSE, 0 );
	}
	catch ( exception& ex ) {
		printf("<wsa_reactor::wsa_reactor> %s\n", ex.what());
		throw;
	}
}

wsa_reactor::~wsa_reactor()
{
	{
		pthread_mutex_locker ml( wait_events_mutex );
		for(wait_events_t::iterator it=wait_events->begin(),end=wait_events->end();it!=end;it++)
			WSACloseEvent( it->first );
	}

	pthread_mutex_destroy( &wait_events_mutex);

	CloseHandle( hInterruptEvent );
	CloseHandle( hExitEvent );
}

void wsa_reactor::add_thread(pthread_t bthread)
{
}

void wsa_reactor::add_conn(tcp_conn_struct*)
{
}

void wsa_reactor::add_event(SOCKET socket, int type)
{
	WSAEVENT new_event;
	if( WSA_INVALID_EVENT==(new_event = WSACreateEvent()) )
		throw exception( " WSACreateEvent failed with error ", WSAGetLastError() );

	if( SOCKET_ERROR==WSAEventSelect( socket, new_event, (long)type) )
		throw exception( " WSAEventSelect failed with error ", WSAGetLastError() );

	{
		pthread_mutex_locker ml( wait_events_mutex );
		wait_events->insert( std::make_pair( new_event, socket ) );
	}
}

void wsa_reactor::del_event(SOCKET socket)
{
	pthread_mutex_locker ml( wait_events_mutex );
	if( 0==wait_events->size() )
		return;
	for( wait_events_t::iterator it = wait_events->begin(),end=wait_events->end();it!=end;it++)
		if( socket==it->second )
		{
			wait_events->erase( it );
			break;
		}
}

bool wsa_reactor::get_interrupt(interrupt& result)
{
		uint32_t i=0;
		WSAEVENT events[ 2000 ];
		uint32_t count = 0;


		i = 0;
		events[i++] = hInterruptEvent;
		events[i++] = hExitEvent;
		{
			pthread_mutex_locker ml( wait_events_mutex );
			count=(DWORD)wait_events->size()+2;
			for(wait_events_t::iterator it=wait_events->begin(),end=wait_events->end();it!=end;it++)
				events[i++] = it->first;
		}

		// temporary code "polling"{
		if( count>60 )
		{
			WSAEVENT ievents[ 60 ];
			for(;;)
			{
				bool catched=false;
				uint32_t j=0;
				while( j<count )
				{
					bool last=count<(j+60);

					for(int l=0;l!=60;l++)
						ievents[l] = events[j+l];

					if( WAIT_FAILED == (i=WaitForMultipleObjects( last?(count%60):60, ievents, FALSE, last?100:0 )) )
						throw exception(" WaitForMultipleObjects failed with error ", GetLastError() );

					if( WAIT_TIMEOUT==i )
					{
						j += 60;
						continue;
					}

					i += j;
					catched = true;
					break;
				}

				if( catched )
					break;
			}
		}
		// temporary code "polling"}

		else
		{
			if( WAIT_FAILED == (i=WaitForMultipleObjects( count, events, FALSE, INFINITE )) )
				throw exception(" WaitForMultipleObjects failed with error %d", GetLastError() );
		}

		if( hInterruptEvent==events[i] )
		{
			if( !ResetEvent( hInterruptEvent ) )
				throw exception(" ResetEvent fail with code %d\n ", GetLastError() );
			result.insert( std::make_pair(0,0) );
			return true; // internal stop
		}

		if( hExitEvent==events[i] )
		{
			if( !ResetEvent( hExitEvent ) )
				throw exception(" ResetEvent failed");
			result.insert( std::make_pair(-1,0) );
			return true;
		}

		wait_events_t::iterator it;
		{
			pthread_mutex_locker ml( wait_events_mutex );
			wait_events_t::iterator ;
			if( wait_events->end()==(it = wait_events->find( events[i] )) )
				return false;

			if( WSAEnumNetworkEvents( it->second, events[i], &wsEvents ) )
			{
				int ierr = WSAGetLastError();
				switch( ierr )
				{
				case WSAENOTSOCK:
				case WSAENOTCONN:
				case WSAECONNRESET:	
					result.insert( std::make_pair(it->second,(int)FD_CLOSE) );
					wait_events->erase( it );
					return true;
				}
				return false;
			}
		}

		if( !what_happened() )
		{
			result.insert( std::make_pair(it->second,(int)FD_CLOSE));
			{
				pthread_mutex_locker ml( wait_events_mutex );
				wait_events->erase( it );
			}
			return true;
		}

		result.insert( std::make_pair(it->second,(int)wsEvents.lNetworkEvents) );
		return true;
}

bool wsa_reactor::what_happened()
{
	if( wsEvents.lNetworkEvents & FD_READ )
	if( wsEvents.iErrorCode[FD_READ_BIT] ) return false;
			
	if( wsEvents.lNetworkEvents & FD_WRITE )
	if( wsEvents.iErrorCode[FD_WRITE_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_OOB )
	if( wsEvents.iErrorCode[FD_OOB_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_ACCEPT )
	if( wsEvents.iErrorCode[FD_ACCEPT_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_CONNECT )
	if( wsEvents.iErrorCode[FD_CONNECT_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_CLOSE )
	if( wsEvents.iErrorCode[FD_CLOSE_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_QOS )
	if( wsEvents.iErrorCode[FD_QOS_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_GROUP_QOS )
	if( wsEvents.iErrorCode[FD_GROUP_QOS_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_ROUTING_INTERFACE_CHANGE )
	if( wsEvents.iErrorCode[FD_ROUTING_INTERFACE_CHANGE_BIT] ) return false;

	if( wsEvents.lNetworkEvents & FD_ADDRESS_LIST_CHANGE )
	if( wsEvents.iErrorCode[FD_ADDRESS_LIST_CHANGE_BIT] ) return false;

	return true;
}

void wsa_reactor::stop(SOCKET socket)
{
	if( -1==socket && !SetEvent( hExitEvent ) )
		throw exception("SetEvent fail with code ", GetLastError());

	if( !SetEvent( hInterruptEvent ) )
		throw exception("SetEvent fail with code ", GetLastError());
}

imulti_reactor* make_wsa_reactor(){	return new wsa_reactor(); }

}

#endif






