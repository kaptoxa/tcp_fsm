// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32

#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

#define TCP_FSM_EXPORT __declspec(dllexport) 

#else

#define TCP_FSM_EXPORT

#endif

#include <stdio.h>
#include <vector>
#include <map>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/signals.hpp>
#include <boost/bind.hpp>
#include <netsim/netsim.h>
#include <inttypes.h>

#include "config.h"
