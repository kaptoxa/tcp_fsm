// tcp_fsm.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "itcp_dispatcher.h"

#ifdef _WIN32
#include "tcp_dispatcher_biocp.h"
#elif
#include "tcp_dispatcher.h"
#endif

#if defined(_DEBUG) && defined(_WIN32)
#define new DEBUG_NEW
#endif

#ifdef _WIN32

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

#endif


