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

#ifndef SYSTEM_ERROR2_NT_CODE_HPP
#define SYSTEM_ERROR2_NT_CODE_HPP

#if !defined(_WIN32) && !defined(STANDARDESE_IS_IN_THE_HOUSE)
#error This file should only be included on Windows
#endif

#include "win32_code.hpp"

SYSTEM_ERROR2_NAMESPACE_BEGIN

//! \exclude
namespace win32
{
  // A Win32 NTSTATUS
  using NTSTATUS = long;
  // A Win32 HMODULE
  using HMODULE = void *;
  // Used to retrieve where the NTDLL DLL is mapped into memory
  extern HMODULE __stdcall GetModuleHandleW(const wchar_t *lpModuleName);
#pragma comment(lib, "kernel32.lib")
#pragma comment(linker, "/alternatename:?GetModuleHandleW@win32@system_error2@@YAPEAXPEB_W@Z=GetModuleHandleW")
}  // namespace win32

class _nt_code_domain;
//! (Windows only) A NT error code, those returned by NT kernel functions.
using nt_code = status_code<_nt_code_domain>;
//! (Windows only) A specialisation of `status_error` for the NT error code domain.
using nt_error = status_error<_nt_code_domain>;

/*! (Windows only) The implementation of the domain for NT error codes, those returned by NT kernel functions.
*/
class _nt_code_domain : public status_code_domain
{
  template <class DomainType> friend class status_code;
  template <class StatusCode> friend class detail::indirecting_domain;
  friend class _com_code_domain;
  using _base = status_code_domain;
  static int _nt_code_to_errno(win32::NTSTATUS c)
  {
    if(c >= 0)
    {
      return 0;  // success
    }
    switch(static_cast<unsigned>(c))
    {
#include "detail/nt_code_to_generic_code.ipp"
    }
    return -1;
  }
  static win32::DWORD _nt_code_to_win32_code(win32::NTSTATUS c)  // NOLINT
  {
    if(c >= 0)
    {
      return 0;  // success
    }
    switch(static_cast<unsigned>(c))
    {
#include "detail/nt_code_to_win32_code.ipp"
    }
    return static_cast<win32::DWORD>(-1);
  }
  //! Construct from a NT error code
  static _base::string_ref _make_string_ref(win32::NTSTATUS c) noexcept
  {
    wchar_t buffer[32768];
    static win32::HMODULE ntdll = win32::GetModuleHandleW(L"NTDLL.DLL");
    win32::DWORD wlen = win32::FormatMessageW(0x00000800 /*FORMAT_MESSAGE_FROM_HMODULE*/ | 0x00001000 /*FORMAT_MESSAGE_FROM_SYSTEM*/ | 0x00000200 /*FORMAT_MESSAGE_IGNORE_INSERTS*/, ntdll, c, (1 << 10) /*MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)*/, buffer, 32768, nullptr);
    size_t allocation = wlen + (wlen >> 1);
    win32::DWORD bytes;
    if(wlen == 0)
    {
      return _base::string_ref("failed to get message from system");
    }
    for(;;)
    {
      auto *p = static_cast<char *>(malloc(allocation));  // NOLINT
      if(p == nullptr)
      {
        return _base::string_ref("failed to get message from system");
      }
      bytes = win32::WideCharToMultiByte(65001 /*CP_UTF8*/, 0, buffer, (int) (wlen + 1), p, (int) allocation, nullptr, nullptr);
      if(bytes != 0)
      {
        char *end = strchr(p, 0);
        while(end[-1] == 10 || end[-1] == 13)
        {
          --end;
        }
        *end = 0;  // NOLINT
        return _base::atomic_refcounted_string_ref(p, end - p);
      }
      free(p);  // NOLINT
      if(win32::GetLastError() == 0x7a /*ERROR_INSUFFICIENT_BUFFER*/)
      {
        allocation += allocation >> 2;
        continue;
      }
      return _base::string_ref("failed to get message from system");
    }
  }

public:
  //! The value type of the NT code, which is a `win32::NTSTATUS`
  using value_type = win32::NTSTATUS;
  using _base::string_ref;

public:
  //! Default constructor
  constexpr explicit _nt_code_domain(typename _base::unique_id_type id = 0x93f3b4487e4af25b) noexcept : _base(id) {}
  _nt_code_domain(const _nt_code_domain &) = default;
  _nt_code_domain(_nt_code_domain &&) = default;
  _nt_code_domain &operator=(const _nt_code_domain &) = default;
  _nt_code_domain &operator=(_nt_code_domain &&) = default;
  ~_nt_code_domain() = default;

  //! Constexpr singleton getter. Returns the constexpr nt_code_domain variable.
  static inline constexpr const _nt_code_domain &get();

  virtual string_ref name() const noexcept override { return string_ref("NT domain"); }  // NOLINT
protected:
  virtual bool _do_failure(const status_code<void> &code) const noexcept override  // NOLINT
  {
    assert(code.domain() == *this);
    return static_cast<const nt_code &>(code).value() < 0;  // NOLINT
  }
  virtual bool _do_equivalent(const status_code<void> &code1, const status_code<void> &code2) const noexcept override  // NOLINT
  {
    assert(code1.domain() == *this);
    const auto &c1 = static_cast<const nt_code &>(code1);  // NOLINT
    if(code2.domain() == *this)
    {
      const auto &c2 = static_cast<const nt_code &>(code2);  // NOLINT
      return c1.value() == c2.value();
    }
    if(code2.domain() == generic_code_domain)
    {
      const auto &c2 = static_cast<const generic_code &>(code2);  // NOLINT
      if(static_cast<int>(c2.value()) == _nt_code_to_errno(c1.value()))
      {
        return true;
      }
    }
    if(code2.domain() == win32_code_domain)
    {
      const auto &c2 = static_cast<const win32_code &>(code2);  // NOLINT
      if(c2.value() == _nt_code_to_win32_code(c1.value()))
      {
        return true;
      }
    }
    return false;
  }
  virtual generic_code _generic_code(const status_code<void> &code) const noexcept override  // NOLINT
  {
    assert(code.domain() == *this);
    const auto &c = static_cast<const nt_code &>(code);  // NOLINT
    return generic_code(static_cast<errc>(_nt_code_to_errno(c.value())));
  }
  virtual string_ref _do_message(const status_code<void> &code) const noexcept override  // NOLINT
  {
    assert(code.domain() == *this);
    const auto &c = static_cast<const nt_code &>(code);  // NOLINT
    return _make_string_ref(c.value());
  }
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(STANDARDESE_IS_IN_THE_HOUSE)
  SYSTEM_ERROR2_NORETURN virtual void _do_throw_exception(const status_code<void> &code) const override  // NOLINT
  {
    assert(code.domain() == *this);
    const auto &c = static_cast<const nt_code &>(code);  // NOLINT
    throw status_error<_nt_code_domain>(c);
  }
#endif
};
//! (Windows only) A constexpr source variable for the NT code domain, which is that of NT kernel functions. Returned by `_nt_code_domain::get()`.
constexpr _nt_code_domain nt_code_domain;
inline constexpr const _nt_code_domain &_nt_code_domain::get()
{
  return nt_code_domain;
}

SYSTEM_ERROR2_NAMESPACE_END

#endif
