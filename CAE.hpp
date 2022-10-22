#pragma once

#include <stdint.h>

#include <functional>
#include <vector>

#define cae_scope ::cae::cae __CAE_SCOPE

namespace cae {

   void install_catchers();
   void remove_catchers();

   class cae {
   public:
      cae() { install_catchers(); }
      ~cae() { remove_catchers(); }
   };

}

namespace cae {

   class BaseException {
   public:
      BaseException(uintptr_t exceptionAddress)
         : m_exceptionAddress(exceptionAddress) {}

      virtual ~BaseException() {}

      virtual int GetTypeID() = 0;
      virtual uintptr_t GetExceptionAddress() { return m_exceptionAddress; }

   private:
      uintptr_t m_exceptionAddress;
   };

   using MemoryAccessType = int;
   enum MemoryAccessType_ {
      MemoryAccessType_UNKNOWN = 0,
      MemoryAccessType_READ = 1,
      MemoryAccessType_WRITE = 2,
      MemoryAccessType_EXECUTE = 3
   };

   class ExceptionAccessViolation : public BaseException {
   public:
      static constexpr int TypeID = 1;

   public:
      ExceptionAccessViolation(
         uintptr_t exceptionAddress, uintptr_t accessedAddress, MemoryAccessType accessType
     ) : m_accessedAddress(accessedAddress), m_accessType(accessType), BaseException(exceptionAddress) {}

   public:
      uintptr_t GetAccessedAddress() { return m_accessedAddress; }
      MemoryAccessType GetAccessType() { return m_accessType; }
      int GetTypeID() { return TypeID; }

   private:
      uintptr_t m_accessedAddress;
      MemoryAccessType m_accessType;
   };

}