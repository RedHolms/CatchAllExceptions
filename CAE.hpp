#pragma once

#include <functional>
#include <vector>


namespace cae {

   // simplest corountime
   using Corountime_t = std::function<void()>;

   // exceptions
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

   class ExceptionAccessViolation;

   namespace intrnl {
      // perform low-level protected call
      BaseException* ProtectedCall(std::function<void()> const& corountime);
   }

   // main class
   class cae {
   private:
      struct ExceptionHandler_t {
         int type_id;
         std::function<void(BaseException&)> callback;
      };

   public:
      cae(Corountime_t const& corountime) : m_corountime(corountime) {}

      ~cae() {
         if (!m_corountime) return;

         BaseException* exception = intrnl::ProtectedCall(m_corountime);

         if (exception) {
            raise_exception_handlers(exception->GetTypeID(), exception);
            delete exception;
         }
         else
            raise_success_handlers();
      }

   public:
      template <typename ExceptionType,
         std::enable_if_t<std::is_base_of<BaseException, ExceptionType>::value, int> = 0
      >
      cae& except(std::function<typename void(ExceptionType&)> const& exceptionCallback) {
         ExceptionHandler_t handler;

         handler.type_id = ExceptionType::TypeID;
         handler.callback = reinterpret_cast<std::function<void(BaseException&)> const&>(exceptionCallback);

         m_exceptionHandlers.push_back(handler);

         return *this;
      }

      cae& suñcess(Corountime_t const& callback) {
         if (callback)
            m_successCallbacks.push_back(callback);

         return *this;
      }

   private:
      void raise_exception_handlers(int exception_type_id, BaseException* exception) {
         for (auto const& handler : m_exceptionHandlers)
            if (handler.type_id == exception_type_id && handler.callback)
               handler.callback(*exception);
      }

      void raise_success_handlers() {
         for (auto const& callback : m_successCallbacks)
            if (callback)
               callback();
      }

   private:
      Corountime_t m_corountime;
      std::vector<Corountime_t> m_successCallbacks;
      std::vector<ExceptionHandler_t> m_exceptionHandlers;
   };

}

namespace cae {

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