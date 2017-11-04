// Copyright (C) 2017 Rob Caelers <rob.caelers@gmail.com>
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
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef OS_LAMBDA_HPP
#define OS_LAMBDA_HPP

// https://stackoverflow.com/questions/11893141/inferring-the-call-signature-of-a-lambda-or-arbitrary-callable-for-make-functio

namespace os
{
  template<typename T>
  struct remove_class
  {
  };

  template<typename C, typename R, typename... A>
  struct remove_class<R(C::*)(A...)>
  {
    using type = R(A...);
  };

  template<typename C, typename R, typename... A>
  struct remove_class<R(C::*)(A...) const>
  {
    using type = R(A...);
  };

  template<typename C, typename R, typename... A>
  struct remove_class<R(C::*)(A...) volatile>
  {
    using type = R(A...);
  };

  template<typename C, typename R, typename... A>
  struct remove_class<R(C::*)(A...) const volatile>
  {
    using type = R(A...);
  };

  template<typename T>
  struct get_signature_impl
  {
    using type = typename remove_class<decltype(&std::remove_reference<T>::type::operator())>::type;
  };

  template<typename R, typename... A>
  struct get_signature_impl<R(A...)>
  {
    using type = R(A...);
  };

  template<typename R, typename... A>
  struct get_signature_impl<R(&)(A...)>
  {
    using type = R(A...);
  };

  template<typename R, typename... A>
  struct get_signature_impl<R(*)(A...)>
  {
    using type = R(A...);
  };

  template<typename T>
  using get_signature = typename get_signature_impl<T>::type;

  template<typename F>
  using make_function_type = std::function<get_signature<F>>;

  template<typename F>
  make_function_type<F> make_function(F &&f)
  {
    return make_function_type<F>(std::forward<F>(f));
  }
}

#endif // OS_LAMBDA_HPP
