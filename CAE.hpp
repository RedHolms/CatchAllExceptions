#pragma once

#include <functional>

#include "CAE_Exceptions.hpp"

__declspec(noinline)
CaeBaseException* cae_protectedcall(std::function<void(void)>&);

/*
   Main class
   Example can be found at `test.cpp`

   NOTE:
      You need to not store `cae` in any variable!
      Function called when destructor called, so storing this class may cause an errors
*/
class cae {
public:
   cae(std::function<void(void)> fn) {
      m_fn = fn;
   }

   ~cae() {
      auto res = cae_protectedcall(m_fn);
      if (res) {
         m_exceptionHandler(*res);
         delete res;
      } else
         m_successHandler();
      m_onendHandler();
   }

   // Set handler that will be called on exception
   cae& except(std::function<void(CaeBaseException&)> fn) {
      m_exceptionHandler = fn;
      return *this;
   }

   // Set handler that will be called when execution ended without exceptions
   cae& success(std::function<void(void)> fn) {
      m_successHandler = fn;
      return *this;
   }
   
   // Set handle that will be called after execution
   cae& onend(std::function<void(void)> fn) {
      m_onendHandler = fn;
      return *this;
   }

private:
   std::function<void(CaeBaseException&)> m_exceptionHandler;
   std::function<void(void)> m_successHandler;
   std::function<void(void)> m_onendHandler;
   std::function<void(void)> m_fn;
};