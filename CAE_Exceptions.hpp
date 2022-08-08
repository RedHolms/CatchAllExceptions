#pragma once

#include <stdint.h>

/*
   Base exception class
   You can determine exception by GetID() method (Every exception has static constexpr ID member)
*/
class CaeBaseException {
public:
   CaeBaseException(uintptr_t exceptionPtr)
      : m_exceptionPtr(exceptionPtr) {}

   uintptr_t getExceptionAddress() { return m_exceptionPtr; }
   virtual uint64_t GetID() = 0;

private:
   uintptr_t m_exceptionPtr;
};

/*
   Unknown exception
   ID: 0
*/
class CaeExceptionUnknown : public CaeBaseException {
public:
   static constexpr uint64_t ID = 0;

   CaeExceptionUnknown(uintptr_t exceptionPtr)
      : CaeBaseException(exceptionPtr) {}

   uint64_t GetID() { return ID; }
};

/*
   Access Violation exception
   Causes when reading/writing/executing at invalid address(for example, 0 address)
   ID: 1
*/
class CaeExceptionAccessViolation : public CaeBaseException {
public:
   enum class AccessType {
      UNKNOWN = 0,
      READ = 1,
      WRITE = 2,
      EXECUTE = 3,
   };

   static constexpr uint64_t ID = 1;

   CaeExceptionAccessViolation(uintptr_t exceptionPtr, uintptr_t accessedPtr, AccessType accessType)
      : CaeBaseException(exceptionPtr), m_accessedPtr(accessedPtr), m_accessType(accessType) {}

   uint64_t GetID() { return ID; }

   uintptr_t getAccessedAddress() { return m_accessedPtr; }
   AccessType getAccessType() { return m_accessType; }

private:
   uintptr_t m_accessedPtr;
   AccessType m_accessType;
};

// TODO: add more exceptions

const char* cae_tostring(CaeExceptionAccessViolation::AccessType);