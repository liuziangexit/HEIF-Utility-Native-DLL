#pragma once
#ifndef liuzianglib_type
#define liuzianglib_type
#include <stdint.h>
//Version 2.4.21V30
//20170914
//This file only support MSVC

namespace DC {

	using byte_t = unsigned char;
	using size_t = unsigned int;
	using ulong_t = unsigned long;

#ifdef _WIN64

	typedef int64_t pos_type;

#else

	typedef int32_t pos_type;

#endif
	
}

#endif
