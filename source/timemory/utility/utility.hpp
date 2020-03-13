// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

/** \file utility/utility.hpp
 * \headerfile utility/utility.hpp "timemory/utility/utility.hpp"
 * General utility functions
 *
 */

#pragma once

#include "timemory/utility/macros.hpp"
#include "timemory/utility/types.hpp"

// C library
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
// I/O
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
// general
#include <functional>
#include <limits>
#include <utility>
// container
#include <map>
#include <vector>
// threading
#include <atomic>
#include <mutex>
#include <thread>

#if defined(_UNIX)
#    include <cxxabi.h>
#    include <errno.h>
#    include <stdio.h>
#    include <string.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#elif defined(_WINDOWS)
#    include <direct.h>
#endif

#if !defined(DEFAULT_UMASK)
#    define DEFAULT_UMASK 0777
#endif

//--------------------------------------------------------------------------------------//

// stringify some macro -- uses TIMEMORY_STRINGIZE2 which does the actual
//   "stringify-ing" after the macro has been substituted by it's result
#if !defined(TIMEMORY_STRINGIZE)
#    define TIMEMORY_STRINGIZE(X) TIMEMORY_STRINGIZE2(X)
#endif

// actual stringifying
#if !defined(TIMEMORY_STRINGIZE2)
#    define TIMEMORY_STRINGIZE2(X) #    X
#endif

// stringify the __LINE__ macro
#if !defined(TIMEMORY_TIM_LINESTR)
#    define TIMEMORY_TIM_LINESTR TIMEMORY_STRINGIZE(__LINE__)
#endif

//--------------------------------------------------------------------------------------//

namespace tim
{
//--------------------------------------------------------------------------------------//

template <typename _Tp>
inline bool
isfinite(const _Tp& arg)
{
#if defined(_WINDOWS)
    // Windows seems to be missing std::isfinite
    return (arg == arg && arg != std::numeric_limits<_Tp>::infinity() &&
            arg != -std::numeric_limits<_Tp>::infinity())
               ? true
               : false;
#else
    return std::isfinite(arg);
#endif
}

//--------------------------------------------------------------------------------------//

using string_t    = std::string;
using str_list_t  = std::vector<string_t>;
using mutex_t     = std::recursive_mutex;
using auto_lock_t = std::unique_lock<mutex_t>;

//======================================================================================//
//
//  General functions
//
//======================================================================================//

template <typename _Tp>
mutex_t&
type_mutex(const uint64_t& _n = 0)
{
    static mutex_t* _mutex = new mutex_t();
    if(_n == 0)
        return *_mutex;

    static std::vector<mutex_t*> _mutexes;
    if(_n > _mutexes.size())
        _mutexes.resize(_n, nullptr);
    if(!_mutexes[_n])
        _mutexes[_n] = new mutex_t();
    return *(_mutexes[_n - 1]);
}

//--------------------------------------------------------------------------------------//

inline std::string
demangle(const char* _cstr)
{
#if defined(TIMEMORY_ENABLE_DEMANGLE)
    // demangling a string when delimiting
    int   _ret    = 0;
    char* _demang = abi::__cxa_demangle(_cstr, 0, 0, &_ret);
    if(_demang && _ret == 0)
        return std::string(const_cast<const char*>(_demang));
    else
        return _cstr;
#else
    return _cstr;
#endif
}

//--------------------------------------------------------------------------------------//

inline std::string
demangle(const std::string& _str)
{
    return demangle(_str.c_str());
}

//--------------------------------------------------------------------------------------//

template <typename _Tp>
inline std::string
demangle()
{
    return demangle(typeid(_Tp).name());
}

//--------------------------------------------------------------------------------------//

template <typename T>
inline T
from_string(const std::string& str)
{
    std::stringstream ss;
    ss << str;
    T val{};
    ss >> val;
    return val;
}

//--------------------------------------------------------------------------------------//

template <typename T>
inline T
from_string(const char* cstr)
{
    std::stringstream ss;
    ss << cstr;
    T val{};
    ss >> val;
    return val;
}

//--------------------------------------------------------------------------------------//

inline std::string
dirname(std::string _fname)
{
#if defined(_UNIX)
    char* _cfname = realpath(_fname.c_str(), NULL);
    _fname        = std::string(_cfname);
    free(_cfname);

    while(_fname.find("\\\\") != std::string::npos)
        _fname.replace(_fname.find("\\\\"), 2, "/");
    while(_fname.find("\\") != std::string::npos)
        _fname.replace(_fname.find("\\"), 1, "/");

    return _fname.substr(0, _fname.find_last_of("/"));
#elif defined(_WINDOWS)
    while(_fname.find("/") != std::string::npos)
        _fname.replace(_fname.find("/"), 1, "\\");

    _fname = _fname.substr(0, _fname.find_last_of("\\"));
    return (_fname.at(_fname.length() - 1) == '\\')
               ? _fname.substr(0, _fname.length() - 1)
               : _fname;
#endif
}

//--------------------------------------------------------------------------------------//

inline int
makedir(std::string _dir, int umask = DEFAULT_UMASK)
{
#if defined(_UNIX)
    while(_dir.find("\\\\") != std::string::npos)
        _dir.replace(_dir.find("\\\\"), 2, "/");
    while(_dir.find("\\") != std::string::npos)
        _dir.replace(_dir.find("\\"), 1, "/");

    if(_dir.length() == 0)
        return 0;

    if(mkdir(_dir.c_str(), umask) != 0)
    {
        std::stringstream _sdir;
        _sdir << "mkdir -p " << _dir;
        return system(_sdir.str().c_str());
    }
#elif defined(_WINDOWS)
    consume_parameters(umask);
    while(_dir.find("/") != std::string::npos)
        _dir.replace(_dir.find("/"), 1, "\\");

    if(_dir.length() == 0)
        return 0;

    if(_mkdir(_dir.c_str()) != 0)
    {
        std::stringstream _sdir;
        _sdir << "dir " << _dir;
        return system(_sdir.str().c_str());
    }
#endif
    return 0;
}

//--------------------------------------------------------------------------------------//

inline int32_t
get_max_threads()
{
    int32_t _fallback = std::thread::hardware_concurrency();
#ifdef ENV_NUM_THREADS_PARAM
    return get_env<int32_t>(TIMEMORY_STRINGIZE(ENV_NUM_THREADS_PARAM), _fallback);
#else
    return _fallback;
#endif
}

//--------------------------------------------------------------------------------------//
//  delimit a string into a set
//
template <typename _Container = std::vector<std::string>,
          typename _Predicate = std::function<string_t(string_t)>>
inline _Container
delimit(const string_t& line, const string_t& delimiters = ",; ",
        _Predicate&& predicate = [](string_t s) -> string_t { return s; })
{
    auto _get_first_not_of = [&delimiters](const string_t& _string, const size_t& _beg) {
        return _string.find_first_not_of(delimiters, _beg);
    };

    auto _get_first_of = [&delimiters](const string_t& _string, const size_t& _beg) {
        return _string.find_first_of(delimiters, _beg);
    };

    _Container _result;
    size_t     _beginp = 0;  // position that is the beginning of the new string
    size_t     _delimp = 0;  // position of the delimiter in the string
    while(_beginp < line.length() && _delimp < line.length())
    {
        // find the first character (starting at _end) that is not a delimiter
        _beginp = _get_first_not_of(line, _delimp);
        // if no a character after or at _end that is not a delimiter is not found
        // then we are done
        if(_beginp == string_t::npos)
        {
            break;
        }
        // starting at the position of the new string, find the next delimiter
        _delimp = _get_first_of(line, _beginp);
        // if(d2 == string_t::npos) { d2 = string_t::npos; }
        string_t _tmp = "";
        try
        {
            // starting at the position of the new string, get the characters
            // between this position and the next delimiter
            _tmp = line.substr(_beginp, _delimp - _beginp);
        } catch(std::exception& e)
        {
            // print the exception but don't fail, unless maybe it should?
            std::stringstream ss;
            ss << e.what();
            fprintf(stderr, "%s\n", ss.str().c_str());
        }
        // don't add empty strings
        if(!_tmp.empty())
        {
            _result.insert(_result.end(), predicate(_tmp));
        }
    }
    return _result;
}

//--------------------------------------------------------------------------------------//
//  delimit line : e.g. delimit_line("a B\t c", " \t") --> { "a", "B", "c"}
inline str_list_t
delimit(std::string _str, const std::string& _delims)
{
    str_list_t _list;
    while(_str.length() > 0)
    {
        size_t _end = 0;
        size_t _beg = _str.find_first_not_of(_delims, _end);
        if(_beg == std::string::npos)
            break;
        _end = _str.find_first_of(_delims, _beg);
        if(_beg < _end)
        {
            _list.push_back(_str.substr(_beg, _end - _beg));
            _str.erase(_beg, _end - _beg);
        }
    }
    return _list;
}

//--------------------------------------------------------------------------------------//
//  delimit line : e.g. delimit_line("a B\t c", " \t") --> { "a", "B", "c"}
template <typename _Func>
inline str_list_t
delimit(std::string _str, const std::string& _delims,
        const _Func& strop = [](const std::string& s) { return s; })
{
    str_list_t _list;
    while(_str.length() > 0)
    {
        size_t _end = 0;
        size_t _beg = _str.find_first_not_of(_delims, _end);
        if(_beg == std::string::npos)
            break;
        _end = _str.find_first_of(_delims, _beg);
        if(_beg < _end)
        {
            _list.push_back(strop(_str.substr(_beg, _end - _beg)));
            _str.erase(_beg, _end - _beg);
        }
    }
    return _list;
}

//======================================================================================//
//
//  path
//
//======================================================================================//

class path_t : public std::string
{
public:
    using string_t   = std::string;
    using size_type  = string_t::size_type;
    using stl_string = std::basic_string<char>;

public:
    path_t(const std::string& _path)
    : string_t(osrepr(_path))
    {}
    path_t(char* _path)
    : string_t(osrepr(string_t(_path)))
    {}
    path_t(const path_t& rhs)
    : string_t(osrepr(rhs))
    {}
    path_t(const char* _path)
    : string_t(osrepr(string_t(const_cast<char*>(_path))))
    {}

    path_t& operator=(const string_t& rhs)
    {
        string_t::operator=(osrepr(rhs));
        return *this;
    }

    path_t& operator=(const path_t& rhs)
    {
        if(this != &rhs)
            string_t::operator=(osrepr(rhs));
        return *this;
    }

    path_t& insert(size_type __pos, const stl_string& __s)
    {
        string_t::operator=(osrepr(string_t::insert(__pos, __s)));
        return *this;
    }

    path_t& insert(size_type __pos, const path_t& __s)
    {
        string_t::operator=(osrepr(string_t::insert(__pos, __s)));
        return *this;
    }

    string_t os() const
    {
#if defined(_WINDOWS)
        return "\\";
#elif defined(_UNIX)
        return "/";
#endif
    }

    string_t inverse() const
    {
#if defined(_WINDOWS)
        return "/";
#elif defined(_UNIX)
        return "\\";
#endif
    }

    // OS-dependent representation
    string_t osrepr(string_t _path)
    {
#if defined(_WINDOWS)
        while(_path.find("/") != std::string::npos)
            _path.replace(_path.find("/"), 1, "\\");
#elif defined(_UNIX)
        while(_path.find("\\\\") != std::string::npos)
            _path.replace(_path.find("\\\\"), 2, "/");
        while(_path.find("\\") != std::string::npos)
            _path.replace(_path.find("\\"), 1, "/");
#endif
        return _path;
    }
};

//--------------------------------------------------------------------------------------//

inline size_t
unaligned_load(const char* p)
{
    size_t result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

//--------------------------------------------------------------------------------------//

#if __SIZEOF_SIZE_T__ == 8

// Loads n bytes, where 1 <= n < 8.
inline size_t
load_bytes(const char* p, int n)
{
    size_t result = 0;
    --n;
    do
        result = (result << 8) + static_cast<unsigned char>(p[n]);
    while(--n >= 0);
    return result;
}

//--------------------------------------------------------------------------------------//

inline size_t
shift_mix(size_t v)
{
    return v ^ (v >> 47);
}

#endif

//--------------------------------------------------------------------------------------//

#if __SIZEOF_SIZE_T__ == 4

//--------------------------------------------------------------------------------------//
// Implementation of Murmur hash for 32-bit size_t.
//
inline size_t
hash_bytes(const void* ptr, size_t len, size_t seed)
{
    const size_t m    = 0x5bd1e995;
    size_t       hash = seed ^ len;
    const char*  buf  = static_cast<const char*>(ptr);

    // Mix 4 bytes at a time into the hash.
    while(len >= 4)
    {
        size_t k = unaligned_load(buf);
        k *= m;
        k ^= k >> 24;
        k *= m;
        hash *= m;
        hash ^= k;
        buf += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array.
    switch(len)
    {
        case 3: hash ^= static_cast<unsigned char>(buf[2]) << 16; [[gnu::fallthrough]];
        case 2: hash ^= static_cast<unsigned char>(buf[1]) << 8; [[gnu::fallthrough]];
        case 1: hash ^= static_cast<unsigned char>(buf[0]); hash *= m;
    };

    // Do a few final mixes of the hash.
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;
    return hash;
}

//--------------------------------------------------------------------------------------//
// Implementation of FNV hash for 32-bit size_t.
// N.B. This function should work on unsigned char, otherwise it does not
// correctly implement the FNV-1a algorithm (see PR59406).
// The existing behaviour is retained for backwards compatibility.
//
inline size_t
fnv_hash_bytes(const void* ptr, size_t len, size_t hash)
{
    const char* cptr = static_cast<const char*>(ptr);
    for(; len; --len)
    {
        hash ^= static_cast<size_t>(*cptr++);
        hash *= static_cast<size_t>(16777619UL);
    }
    return hash;
}

#elif __SIZEOF_SIZE_T__ == 8

//--------------------------------------------------------------------------------------//
// Implementation of Murmur hash for 64-bit size_t.
//
inline size_t
hash_bytes(const void* ptr, size_t len, size_t seed)
{
    static constexpr size_t mul =
        (((size_t) 0xc6a4a793UL) << 32UL) + (size_t) 0x5bd1e995UL;
    const char* const buf = static_cast<const char*>(ptr);

    // Remove the bytes not divisible by the sizeof(size_t).  This
    // allows the main loop to process the data as 64-bit integers.
    const size_t len_aligned = len & ~(size_t) 0x7;
    const char* const end = buf + len_aligned;
    size_t hash = seed ^ (len * mul);
    for(const char* p = buf; p != end; p += 8)
    {
        const size_t data = shift_mix(unaligned_load(p) * mul) * mul;
        hash ^= data;
        hash *= mul;
    }
    if((len & 0x7) != 0)
    {
        const size_t data = load_bytes(end, len & 0x7);
        hash ^= data;
        hash *= mul;
    }
    hash = shift_mix(hash) * mul;
    hash = shift_mix(hash);
    return hash;
}

//--------------------------------------------------------------------------------------//
// Implementation of FNV hash for 64-bit size_t.
// N.B. This function should work on unsigned char, otherwise it does not
// correctly implement the FNV-1a algorithm (see PR59406).
// The existing behaviour is retained for backwards compatibility.
//
inline size_t
fnv_hash_bytes(const void* ptr, size_t len, size_t hash)
{
    const char* cptr = static_cast<const char*>(ptr);
    for(; len; --len)
    {
        hash ^= static_cast<size_t>(*cptr++);
        hash *= static_cast<size_t>(1099511628211ULL);
    }
    return hash;
}

#else

//--------------------------------------------------------------------------------------//
// Dummy hash implementation for unusual sizeof(size_t).
//
inline size_t
hash_bytes(const void* ptr, size_t len, size_t seed)
{
    size_t      hash = seed;
    const char* cptr = reinterpret_cast<const char*>(ptr);
    for(; len; --len)
        hash = (hash * 131) + *cptr++;
    return hash;
}

//--------------------------------------------------------------------------------------//
//
inline size_t
fnv_hash_bytes(const void* ptr, size_t len, size_t seed)
{
    return hash_bytes(ptr, len, seed);
}

#endif /* __SIZEOF_SIZE_T__ */

//--------------------------------------------------------------------------------------//

template <typename T>
inline size_t
get_hash(T&& obj)
{
    return std::hash<T>()(std::forward<T>(obj));
}

//--------------------------------------------------------------------------------------//

inline size_t
get_hash(const std::string& str)
{
    static constexpr size_t seed = static_cast<size_t>(0xc70f6907UL);
    return hash_bytes(static_cast<const void*>(str.data()), str.length(), seed);
}

//--------------------------------------------------------------------------------------//

inline size_t
get_hash(std::string&& str)
{
    static constexpr size_t seed = static_cast<size_t>(0xc70f6907UL);
    return hash_bytes(static_cast<const void*>(str.data()), str.length(), seed);
}

//--------------------------------------------------------------------------------------//

inline size_t
get_hash(const char* cstr)
{
    static constexpr size_t seed = static_cast<size_t>(0xc70f6907UL);
    return hash_bytes(static_cast<const void*>(cstr), strlen(cstr), seed);
}

//--------------------------------------------------------------------------------------//

inline size_t
get_hash(char* cstr)
{
    static constexpr size_t seed = static_cast<size_t>(0xc70f6907UL);
    return hash_bytes(static_cast<const void*>(cstr), strlen(cstr), seed);
}

//--------------------------------------------------------------------------------------//

template <typename T>
struct hasher
{
    inline size_t operator()(T&& val) const { return get_hash(std::forward<T>(val)); }
    inline size_t operator()(const T& val) const { return get_hash(val); }
};

//--------------------------------------------------------------------------------------//

}  // namespace tim

//--------------------------------------------------------------------------------------//
