#pragma once

#include <pthread.h>
#include <boost/smart_ptr.hpp>
#include <vector>
#include <queue>


#ifndef _WIN32
#define SOCKET int
#endif


#define _P_CLOSE_CONNECTION			1
#define _P_CREATE_CONNECTION		2
#define _P_CLOSE_CONNECTION_WAIT	7
#define _P_STOP_ACCEPTING			6
#define _P_SEND_REQUEST				3
#define _P_BREAK_SEND				8
#define _P_DO_NOTHING				9
#define _P_EXIT						10

#define _P_EXTERNAL					11

namespace tcp_fsm
{

struct external_struct
{
	SOCKET sock;
	int events;
	external_struct(SOCKET _sock, int _events):
		sock(_sock),events(_events){};
};

struct internal_message
{
	boost::shared_ptr<uint8_t> code;
	boost::shared_ptr<char*> ptr;

	internal_message()
	{
		code.reset( new uint8_t(0) );
		ptr.reset( new char* );
	}

	char* get(){ return *ptr ;}
};

class internal_messages
{
	pthread_mutex_t	_mutex;
	boost::shared_ptr< std::queue<internal_message> > _messages;

public:

	internal_messages();
	~internal_messages();


	virtual void push(uint8_t code);				// without arguments
	virtual internal_message pop();					// means to kill element of queue

	virtual void push(uint8_t code, char* ptr);		// with an one pointer
	virtual size_t size() { return _messages->size() ;}
	virtual bool empty() { return _messages->empty();}

};

}
