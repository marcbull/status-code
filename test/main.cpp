/* Proposed SG14 status_code testing
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

#ifdef _WIN32
#include "com_code.hpp"
#endif

#include "iostream_support.hpp"
#include "system_error2.hpp"

#include <cstdio>
#include <cstring>  // for strlen
#include <memory>
#include <string>

#define CHECK(expr)                                                                                                                                                                                                                                                                                                            \
  if(!(expr))                                                                                                                                                                                                                                                                                                                  \
  {                                                                                                                                                                                                                                                                                                                            \
    fprintf(stderr, #expr " failed at line %d\n", __LINE__);                                                                                                                                                                                                                                                                   \
    retcode = 1;                                                                                                                                                                                                                                                                                                               \
  }

// An error coding with multiple success values
enum class Code : size_t
{
  success1,
  goaway,
  success2,
  error2
};
inline std::ostream &operator<<(std::ostream &s, Code v)
{
  return s << static_cast<size_t>(v);
}
class Code_domain_impl;
using StatusCode = system_error2::status_code<Code_domain_impl>;
// Category for Code
class Code_domain_impl : public system_error2::status_code_domain
{
  using _base = system_error2::status_code_domain;

public:
  using value_type = Code;

  // Custom string_ref using a shared_ptr
  class string_ref : public _base::string_ref
  {
  public:
    using shared_ptr_type = std::shared_ptr<std::string>;

  protected:
    static void _custom_string_thunk(_base::string_ref *_dest, const _base::string_ref *_src, _base::string_ref::_thunk_op op)
    {
      auto *dest = static_cast<string_ref *>(_dest);      // NOLINT
      auto *src = static_cast<const string_ref *>(_src);  // NOLINT
      assert(dest->_thunk == _custom_string_thunk);
      assert(src == nullptr || src->_thunk == _custom_string_thunk);
      switch(op)
      {
      case _base::string_ref::_thunk_op::copy:
        new(reinterpret_cast<shared_ptr_type *>(dest->_state)) shared_ptr_type(*reinterpret_cast<const shared_ptr_type *>(src->_state));  // NOLINT
        return;
      case _base::string_ref::_thunk_op::move:
        new(reinterpret_cast<shared_ptr_type *>(dest->_state)) shared_ptr_type(std::move(*reinterpret_cast<shared_ptr_type *>(const_cast<void **>(src->_state))));  // NOLINT
        return;
      case _base::string_ref::_thunk_op::destruct:
      {
        auto *p = reinterpret_cast<shared_ptr_type *>(dest->_state);  // NOLINT
        p->~shared_ptr_type();
      }
      }
    }

  public:
    explicit string_ref(const _base::string_ref &o)
        : _base::string_ref(o)
    {
    }
    explicit string_ref(_base::string_ref &&o)
        : _base::string_ref(std::move(o))
    {
    }
    string_ref()
        : _base::string_ref(_custom_string_thunk)
    {
      new(reinterpret_cast<shared_ptr_type *>(this->_state)) shared_ptr_type();  // NOLINT
    }                                                                            // NOLINT
    string_ref(const string_ref &) = default;
    string_ref(string_ref &&) = default;
    string_ref &operator=(const string_ref &) = default;
    string_ref &operator=(string_ref &&) = default;
    ~string_ref() = default;

    // Construct from a C string literal, holding ref counted copy of string
    explicit string_ref(const char *str)
        : _base::string_ref(_custom_string_thunk)
    {
      static_assert(sizeof(shared_ptr_type) <= sizeof(this->_state), "A shared_ptr does not fit into status_code's state");
      auto len = strlen(str);
      auto p = std::make_shared<std::string>(str, len);
      new(reinterpret_cast<shared_ptr_type *>(this->_state)) shared_ptr_type(p);  // NOLINT
      this->_begin = p->data();
      this->_end = p->data() + p->size();  // NOLINT
    }
  };
  constexpr Code_domain_impl() noexcept : _base(0x430f120194fc06c7) {}
  static inline constexpr const Code_domain_impl *get();
  virtual _base::string_ref name() const noexcept override final  // NOLINT
  {
    static string_ref v("Code_category_impl");
    return v;  // NOLINT
  }
  virtual bool _failure(const system_error2::status_code<void> &code) const noexcept override final  // NOLINT
  {
    assert(code.domain() == *this);
    return (static_cast<size_t>(static_cast<const StatusCode &>(code).value()) & 1) != 0;  // NOLINT
  }
  virtual bool _equivalent(const system_error2::status_code<void> &code1, const system_error2::status_code<void> &code2) const noexcept override final  // NOLINT
  {
    assert(code1.domain() == *this);
    const auto &c1 = static_cast<const StatusCode &>(code1);  // NOLINT
    if(code2.domain() == *this)
    {
      const auto &c2 = static_cast<const StatusCode &>(code2);  // NOLINT
      return c1.value() == c2.value();
    }
    // If the other category is generic
    if(code2.domain() == system_error2::generic_code_domain)
    {
      const auto &c2 = static_cast<const system_error2::generic_code &>(code2);  // NOLINT
      switch(c1.value())
      {
      case Code::success1:
      case Code::success2:
        return static_cast<system_error2::errc>(c2.value()) == system_error2::errc::success;
      case Code::goaway:
        switch(static_cast<system_error2::errc>(c2.value()))
        {
        case system_error2::errc::permission_denied:
        case system_error2::errc::operation_not_permitted:
          return true;
        default:
          return false;
        }
      case Code::error2:
        return false;
      }
    }
    return false;
  }
  virtual system_error2::generic_code _generic_code(const system_error2::status_code<void> &code) const noexcept override final  // NOLINT
  {
    assert(code.domain() == *this);
    const auto &c1 = static_cast<const StatusCode &>(code);  // NOLINT
    switch(c1.value())
    {
    case Code::success1:
    case Code::success2:
      return system_error2::generic_code(system_error2::errc::success);
    case Code::goaway:
      return system_error2::generic_code(system_error2::errc::permission_denied);
    case Code::error2:
      return {};
    }
    return {};
  }
  virtual _base::string_ref _message(const system_error2::status_code<void> &code) const noexcept override final  // NOLINT
  {
    assert(code.domain() == *this);
    const auto &c1 = static_cast<const StatusCode &>(code);  // NOLINT
    switch(c1.value())
    {
    case Code::success1:
    {
      static string_ref v("success1");
      return v;  // NOLINT
    }
    case Code::goaway:
    {
      static string_ref v("goaway");
      return v;  // NOLINT
    }
    case Code::success2:
    {
      static string_ref v("success2");
      return v;  // NOLINT
    }
    case Code::error2:
    {
      static string_ref v("error2");
      return v;  // NOLINT
    }
    }
    return string_ref{};  // NOLINT
  }
  virtual void _throw_exception(const system_error2::status_code<void> &code) const override final  // NOLINT
  {
    assert(code.domain() == *this);
    const auto &c = static_cast<const StatusCode &>(code);  // NOLINT
    throw system_error2::status_error<Code_domain_impl>(c);
  }
};
constexpr Code_domain_impl Code_domain;
inline constexpr const Code_domain_impl *Code_domain_impl::get()
{
  return &Code_domain;
}


int main()
{
  using namespace SYSTEM_ERROR2_NAMESPACE;
  int retcode = 0;

#ifdef _MSC_VER
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  constexpr generic_code empty1, success1(errc::success), failure1(errc::permission_denied);
  CHECK(empty1.empty());
  CHECK(!success1.empty());
  CHECK(!failure1.empty());
  CHECK(success1.success());
  CHECK(failure1.failure());
  printf("generic_code empty has value %d (%s) is success %d is failure %d\n", static_cast<int>(empty1.value()), empty1.message().c_str(), static_cast<int>(empty1.success()), static_cast<int>(empty1.failure()));
  printf("generic_code success has value %d (%s) is success %d is failure %d\n", static_cast<int>(success1.value()), success1.message().c_str(), static_cast<int>(success1.success()), static_cast<int>(success1.failure()));
  printf("generic_code failure has value %d (%s) is success %d is failure %d\n", static_cast<int>(failure1.value()), failure1.message().c_str(), static_cast<int>(failure1.success()), static_cast<int>(failure1.failure()));

  constexpr StatusCode empty2, success2(Code::success1), failure2(Code::goaway);
  CHECK(success2.success());
  CHECK(failure2.failure());
  printf("\nStatusCode empty has value %zu (%s) is success %d is failure %d\n", static_cast<size_t>(empty2.value()), empty2.message().c_str(), static_cast<int>(empty2.success()), static_cast<int>(empty2.failure()));
  printf("StatusCode success has value %zu (%s) is success %d is failure %d\n", static_cast<size_t>(success2.value()), success2.message().c_str(), static_cast<int>(success2.success()), static_cast<int>(success2.failure()));
  printf("StatusCode failure has value %zu (%s) is success %d is failure %d\n", static_cast<size_t>(failure2.value()), failure2.message().c_str(), static_cast<int>(failure2.success()), static_cast<int>(failure2.failure()));

  printf("\n(empty1 == empty2) = %d\n", static_cast<int>(empty1 == empty2));        // True, empty ec's always compare equal no matter the type
  printf("(success1 == success2) = %d\n", static_cast<int>(success1 == success2));  // True, success maps onto success
  printf("(success1 == failure2) = %d\n", static_cast<int>(success1 == failure2));  // False, success does not map onto failure
  printf("(failure1 == success2) = %d\n", static_cast<int>(failure1 == success2));  // False, failure does not map onto success
  printf("(failure1 == failure2) = %d\n", static_cast<int>(failure1 == failure2));  // True, filename_too_long maps onto nospace
  CHECK(empty1 == empty2);
  CHECK(success1 == success2);
  CHECK(success1 != failure2);
  CHECK(failure1 != success2);
  CHECK(failure1 == failure2);


  // Test status code erasure
  status_code<erased<int>> success3(success1), failure3(failure1);
  CHECK(success3.success());
  CHECK(success3.domain() == success1.domain());
  CHECK(failure3.failure());
  CHECK(failure3.domain() == failure1.domain());
  printf("\nerased<int> success has value %d (%s) is success %d is failure %d\n", success3.value(), success3.message().c_str(), static_cast<int>(success3.success()), static_cast<int>(success3.failure()));
  printf("erased<int> failure has value %d (%s) is success %d is failure %d\n", failure3.value(), failure3.message().c_str(), static_cast<int>(failure3.success()), static_cast<int>(failure3.failure()));
  generic_code success4(success3), failure4(failure3);
  CHECK(success4.value() == success1.value());
  CHECK(success4.domain() == success1.domain());
  CHECK(failure4.value() == failure1.value());
  CHECK(failure4.domain() == failure1.domain());

  // ostream printers
  std::cout << "\ngeneric_code failure: " << failure1 << std::endl;
  std::cout << "StatusCode failure: " << failure2 << std::endl;
  std::cout << "erased<int> failure: " << failure3 << std::endl;

#ifdef _WIN32
  // Test win32_code
  constexpr win32_code success5(0 /*ERROR_SUCCESS*/), failure5(0x5 /*ERROR_ACCESS_DENIED*/);
  CHECK(success5.success());
  CHECK(failure5.failure());
  printf("\nWin32 code success has value %lu (%s) is success %d is failure %d\n", success5.value(), success5.message().c_str(), static_cast<int>(success5.success()), static_cast<int>(success5.failure()));
  printf("Win32 code failure has value %lu (%s) is success %d is failure %d\n", failure5.value(), failure5.message().c_str(), static_cast<int>(failure5.success()), static_cast<int>(failure5.failure()));
  CHECK(success5 == errc::success);
  CHECK(failure5 == errc::permission_denied);
  CHECK(failure5 == failure1);
  CHECK(failure5 == failure2);
  system_code success6(success5), failure6(failure5);
  CHECK(success6 == errc::success);
  CHECK(failure6 == errc::permission_denied);
  CHECK(failure6 == failure1);
  CHECK(failure6 == failure2);

  // Test nt_code
  constexpr nt_code success7(1 /* positive */), failure7(0xC0000022 /*STATUS_ACCESS_DENIED*/);
  CHECK(success7.success());
  CHECK(failure7.failure());
  printf("\nNT code success has value %ld (%s) is success %d is failure %d\n", success7.value(), success7.message().c_str(), static_cast<int>(success7.success()), static_cast<int>(success7.failure()));
  printf("NT code warning has value %ld (%s) is success %d is failure %d\n", failure7.value(), failure7.message().c_str(), static_cast<int>(failure7.success()), static_cast<int>(failure7.failure()));
  CHECK(success7 == errc::success);
  CHECK(failure7 == errc::permission_denied);
  CHECK(failure7 == failure1);
  CHECK(failure7 == failure2);
  CHECK(failure7 == failure5);
  system_code success8(success7), failure8(failure7);
  CHECK(success8 == errc::success);
  CHECK(failure8 == errc::permission_denied);
  CHECK(failure8 == failure1);
  CHECK(failure8 == failure2);
  CHECK(failure8 == failure5);

  // Test com_code
  {
    // Does com_code correctly handle a wrapped Win32 error code?
    com_code win32failure(HRESULT_FROM_WIN32(0x5 /*ERROR_ACCESS_DENIED*/));
    printf("\nCOM code win32 failure has value %ld (%s) is success %d is failure %d\n", win32failure.value(), win32failure.message().c_str(), static_cast<int>(win32failure.success()), static_cast<int>(win32failure.failure()));
    CHECK(win32failure == errc::permission_denied);
    CHECK(win32failure == failure5);
    CHECK(win32failure == failure7);

    // Does com_code correctly handle a wrapped Win32 error code?
    com_code ntfailure(HRESULT_FROM_NT(0xC0000022 /*STATUS_ACCESS_DENIED*/));
    printf("COM code nt failure has value %ld (%s) is success %d is failure %d\n", ntfailure.value(), ntfailure.message().c_str(), static_cast<int>(ntfailure.success()), static_cast<int>(ntfailure.failure()));
    CHECK(ntfailure == errc::permission_denied);
    CHECK(ntfailure == failure5);
    CHECK(ntfailure == failure7);
    CHECK(ntfailure == win32failure);

    // Does com_code correctly handle the common HRESULT codes?
    const std::pair<com_code, errc> common[] = {
    //
    {com_code(S_OK), errc::success},                      //
    {com_code(E_ACCESSDENIED), errc::permission_denied},  //
    {com_code(E_INVALIDARG), errc::invalid_argument},     //
    {com_code(E_OUTOFMEMORY), errc::not_enough_memory}    //
    };
    for(auto &i : common)
    {
      auto &c = i.first;
      auto &e = i.second;
      printf("COM code common has value %ld (%s) is success %d is failure %d\n", c.value(), c.message().c_str(), static_cast<int>(c.success()), static_cast<int>(c.failure()));
      CHECK(c == e);
    }
  }
#endif

  // Test posix_code
  constexpr posix_code success9(0), failure9(EACCES);
  CHECK(success9.success());
  CHECK(failure9.failure());
  printf("\nPOSIX code success has value %d (%s) is success %d is failure %d\n", success9.value(), success9.message().c_str(), static_cast<int>(success9.success()), static_cast<int>(success9.failure()));
  printf("POSIX code failure has value %d (%s) is success %d is failure %d\n", failure9.value(), failure9.message().c_str(), static_cast<int>(failure9.success()), static_cast<int>(failure9.failure()));
  CHECK(success9 == errc::success);
  CHECK(failure9 == errc::permission_denied);
  CHECK(failure9 == failure1);
  CHECK(failure9 == failure2);
  system_code success10(success9), failure10(failure9);
  CHECK(success10 == errc::success);
  CHECK(failure10 == errc::permission_denied);
  CHECK(failure10 == failure1);
  CHECK(failure10 == failure2);

  // Test error
  error errors[] = {errc::permission_denied, failure1, failure2, failure3, failure4, failure9, failure10};
  printf("\n");
  for(size_t n = 0; n < sizeof(errors) / sizeof(errors[0]); n++)
  {
    printf("error[%zu] has domain %s value %zd (%s) and errc::permission_denied == error = %d\n", n, errors[n].domain().name().c_str(), errors[n].value(), errors[n].message().c_str(), static_cast<int>(errc::permission_denied == errors[n]));
    CHECK(errors[n] == errc::permission_denied);
  }

  return retcode;
}