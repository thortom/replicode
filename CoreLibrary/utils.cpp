//	utils.cpp
//
//	Author: Eric Nivel, Thor List
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel, Thor List
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel or Thor List nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "utils.h"

#if defined(WIN32) || defined(WIN64)
#include <intrin.h>
#pragma intrinsic (_InterlockedDecrement)
#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedExchange)
#pragma intrinsic (_InterlockedExchange64)
#pragma intrinsic (_InterlockedCompareExchange)
#pragma intrinsic (_InterlockedCompareExchange64)
#else
#ifdef DEBUG
#include <map>
#include <execinfo.h>
#endif // DEBUG
#endif //platform

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sys/time.h>
#include <string.h>

namespace core {

SharedLibrary *SharedLibrary::New(const char *fileName) {

    SharedLibrary *sl = new SharedLibrary();
    if (sl->load(fileName))
        return sl;
    else {

        delete sl;
        return NULL;
    }
}

SharedLibrary::SharedLibrary(): library(NULL) {
}

SharedLibrary::~SharedLibrary() {
#if defined(WIN32) || defined(WIN64)
    if (library)
        FreeLibrary(library);
#else
    if (library)
        dlclose(library);
#endif
}

SharedLibrary *SharedLibrary::load(const char *fileName) {
#if defined(WIN32) || defined(WIN64)
    library = LoadLibrary(TEXT(fileName));
    if (!library) {

        DWORD error = GetLastError();
        std::cerr << "> Error: unable to load shared library " << fileName << " :" << error << std::endl;
        return NULL;
    }
#else
    /*
    * libraries on Linux are called 'lib<name>.so'
    * if the passed in fileName does not have those
    * components add them in.
    */
    char *libraryName = (char *)calloc(1, strlen(fileName) + 6 + 1);
    /*if (strstr(fileName, "lib") == NULL) {
    strcat(libraryName, "lib");
    }*/
    strcat(libraryName, fileName);
    if (strstr(fileName + (strlen(fileName) - 3), ".so") == NULL) {
        strcat(libraryName, ".so");
    }
    library = dlopen(libraryName, RTLD_NOW | RTLD_GLOBAL);
    if (!library) {
        std::cout << "> Error: unable to load shared library " << fileName << " :" << dlerror() << std::endl;
        free(libraryName);
        return NULL;
    }
    free(libraryName);
#endif
    return this;
}

////////////////////////////////////////////////////////////////////////////////////////////////

std::string Time::ToString_seconds(uint64_t t)
{
    uint64_t us = t % 1000;
    uint64_t ms = t / 1000;
    uint64_t s = ms / 1000;
    ms = ms % 1000;

    std::string _s = std::to_string(s);
    _s += "s:";
    _s += std::to_string(ms);
    _s += "ms:";
    _s += std::to_string(us);
    _s += "us";

    return _s;
}

std::string Time::ToString_year(uint64_t t)
{
    uint64_t us = t % 1000;
    uint64_t ms = t / 1000;
//uint64_t s=ms/1000;
    ms = ms % 1000;

    time_t _gmt_time;
    time(&_gmt_time);
    struct tm *_t = gmtime(&_gmt_time);

    std::string _s = asctime(_t); // _s is: Www Mmm dd hh:mm:ss yyyy but we want: Www Mmm dd yyyy hh:mm:ss:msmsms:ususus
    std::string year = _s.substr(_s.length() - 5, 4);
    _s.erase(_s.length() - 6, 5);
    std::string hh_mm_ss = _s.substr(_s.length() - 9, 8);
    hh_mm_ss += ":";
    hh_mm_ss += std::to_string(ms);
    hh_mm_ss += ":";
    hh_mm_ss += std::to_string(us);

    _s.erase(_s.length() - 9, 9);
    _s += year;
    _s += " ";
    _s += hh_mm_ss;
    _s += " GMT";

    return _s;
}

}
