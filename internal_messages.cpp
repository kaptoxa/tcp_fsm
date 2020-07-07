
#include "stdafx.h"
#include "internal_messages.h"
#include "pthread_stuff.h"



#if defined(_DEBUG) && defined(_WIN32)
#define new DEBUG_NEW
#endif


namespace tcp_fsm
{

internal_messages::internal_messages()
{
	pthread_mutex_init( &_mutex, 0);
	_messages.reset( new std::queue< internal_message >() );
}

internal_messages::~internal_messages()
{
	pthread_mutex_destroy( &_mutex );
}

void internal_messages::push(uint8_t code)
{
	pthread_mutex_locker ml( _mutex );

	internal_message im;
	*im.code = code;
	_messages->push( im );
}

void internal_messages::push(uint8_t code, char *data)
{
	pthread_mutex_locker ml( _mutex );

	internal_message im;
	*im.code  = code;
	*im.ptr = data;
	_messages->push( im );
}

internal_message internal_messages::pop()
{
	pthread_mutex_locker ml( _mutex );

	std::auto_ptr<internal_message> im( new internal_message() );
	im->code = _messages->front().code;
	im->ptr = _messages->front().ptr ;
	_messages->pop();

	return *im;
}

}
