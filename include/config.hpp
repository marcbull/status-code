/* Proposed SG14 status_code
(C) 2018 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Feb 2018


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef SYSTEM_ERROR2_CONFIG_HPP
#define SYSTEM_ERROR2_CONFIG_HPP

#include <cstddef>  // for size_t

#ifndef SYSTEM_ERROR2_CONSTEXPR14
#if __cplusplus >= 201400 || _MSC_VER >= 1910 /* VS2017 */
//! Defined to be `constexpr` when on C++ 14 or better compilers. Usually automatic, can be overriden.
#define SYSTEM_ERROR2_CONSTEXPR14 constexpr
#else
#define SYSTEM_ERROR2_CONSTEXPR14
#endif
#endif

#ifndef SYSTEM_ERROR2_NAMESPACE
//! The system_error2 namespace name.
#define SYSTEM_ERROR2_NAMESPACE system_error2
//! Begins the system_error2 namespace.
#define SYSTEM_ERROR2_NAMESPACE_BEGIN                                                                                                                                                                                                                                                                                          \
  namespace system_error2                                                                                                                                                                                                                                                                                                      \
  {
//! Ends the system_error2 namespace.
#define SYSTEM_ERROR2_NAMESPACE_END }
#endif

//! Namespace for the library
SYSTEM_ERROR2_NAMESPACE_BEGIN
namespace detail
{
  inline SYSTEM_ERROR2_CONSTEXPR14 size_t cstrlen(const char *str)
  {
    const char *end = nullptr;
    for(end = str; *end != 0; ++end)  // NOLINT
      ;
    return end - str;
  }
}
SYSTEM_ERROR2_NAMESPACE_END

#ifndef SYSTEM_ERROR2_FATAL
SYSTEM_ERROR2_NAMESPACE_BEGIN
namespace detail
{
  namespace avoid_stdio_include
  {
    extern "C" int write(int, const char *, size_t);
    extern "C" void abort();
  }
  inline void do_fatal_exit(const char *msg)
  {
    avoid_stdio_include::write(2 /*stderr*/, msg, cstrlen(msg));
    avoid_stdio_include::write(2 /*stderr*/, "\n", 1);
    avoid_stdio_include::abort();
  }
}
SYSTEM_ERROR2_NAMESPACE_END
//! Prints msg to stderr, and calls `std::terminate()`. Can be overriden via predefinition.
#define SYSTEM_ERROR2_FATAL(msg) ::SYSTEM_ERROR2_NAMESPACE::detail::do_fatal_exit(msg)
#endif

#endif
