#pragma once

#include <common_structs.h>
#include <boost/signals.hpp>

#ifndef _WIN32
#include <config.h>
#endif
#ifdef _WIN32

	#define O_NONBLOCK 1
	#define F_SETFL 0
	#define F_GETFL 1
	#ifndef EWOULDBLOCK
		#define EWOULDBLOCK WSAEWOULDBLOCK     /* This is the value in winsock.h. */
	#endif
	#include <winsock2.h>

	#ifndef INT64_C
	#define INT64_C(c) (c ## LL)
	#define UINT64_C(c) (c ## ULL)
	#endif

	inline void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine) {
		return _malloc_dbg(nSize,_NORMAL_BLOCK,lpszFileName,nLine); }
	inline void __cdecl operator delete(void* pData, LPCSTR lpszFileName, int nLine) {
		::operator delete(pData); }

	#ifdef _DEBUG
	#ifndef DEBUG_NEW
	#define DEBUG_NEW new(__FILE__, __LINE__)
	#endif
	#endif

	#define _CRTDBG_MAP_ALLOC
	#include <Crtdbg.h>

#else

//	#ifdef HAVE_EPOLL_CREATE
		#include <sys/epoll.h>
//	#endif
	#ifdef _FreeBSD
		#include <sys/event.h>
		#include <sys/time.h>
	#endif

	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <stdint.h>
	#ifndef INT64_C
        #define INT64_C(c) (c ## LL)
        #define UINT64_C(c) (c ## ULL)
        #endif

	#define _CrtCheckMemory int
#endif
