#pragma once
#define CAE_included

#include <stdint.h>

#include <string>
#include <sstream>
#include <exception>
#include <functional>

// Base exception class
class system_exception : public std::exception {
public:
  static constexpr uint32_t id = -1;

public:
  system_exception(uintptr_t address, uint32_t exception_code, uint32_t id = system_exception::id)
    : m_address(address), m_exception_code(exception_code), m_id(id) {}

  inline uintptr_t get_address() const { return m_address; }
  inline uint32_t get_exception_code() const { return m_exception_code; }
  inline uint32_t get_id() const { return m_id; }

  // Convert exception object to string
  inline std::string to_string() const {
    std::stringstream ss;
    write_to(ss);
    return ss.str();
  }

  // Write exception object to output string stream
  virtual std::basic_ostream<char>& write_to(std::basic_ostream<char>& os) const {
    os << "system_exception";
    os << " at 0x" << std::hex << m_address;
    os << ": exception_code=" << std::hex << m_exception_code;
    return os;
  }

  virtual const char* what() const { return "system exception"; }

private:
  uintptr_t m_address;
  uint32_t m_exception_code;
  uint32_t m_id;
};

namespace CAE {

  // cae_try/cae_catch/cae_else/cae_finally context
  class context {
    struct exception_catcher {
      template <typename T>
      inline exception_catcher(std::function<void(T&)> const& handler)
        : exception_type_id(T::id),
        handler(
          [handler](system_exception& ex) {
            if (handler) {
              handler(dynamic_cast<T&>(ex));
            }
          }
        )
      {
        static_assert(std::is_base_of_v<system_exception, T>, "You should catch only system exceptions!");
      }

      uint32_t exception_type_id;
      std::function<void(system_exception&)> handler;
    };

  public:
    inline context(std::function<void()> const& worker)
      : m_worker(worker) {}

    inline ~context() {
      invoke();
    }

  public:
    template <typename T>
    inline context& add_catch(std::function<void(T&)> const& handler) {
      m_exceptions_catchers.emplace_back(handler);
      return *this;
    }

    inline context& add_finally(std::function<void()> const& handler) {
      m_finally = handler;
      return *this;
    }

    inline context& add_else(std::function<void()> const& handler) {
      m_else = handler;
      return *this;
    }

    // Invoke context worker in protected enviroment
    void invoke();

  private:
    std::function<void()> m_worker;
    std::vector<exception_catcher> m_exceptions_catchers;
    std::function<void()> m_finally;
    std::function<void()> m_else;
  };

} // namespace CAE

#define cae_try CAE::context([&]()
#define cae_catch(expr) ).add_catch( (std::function<void(expr)>) [&](expr)
#define cae_finally ).add_finally([&]()
#define cae_else ).add_else([&]()
#define cae_end );

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os, system_exception const& ex) {
  return ex.write_to(os);
}

class unknown_system_exception final : public system_exception {
public:
  static constexpr uint32_t id = 0;

public:
  unknown_system_exception(uint32_t address, uint32_t exception_code)
    : system_exception(address, exception_code, id) {}
};

enum class memory_operation {
  unknown,
  read,
  write,
  execute
};

class access_violation final : public system_exception {
public:
  static constexpr uint32_t id = 1;

public:
  access_violation(uintptr_t address, memory_operation operation, uintptr_t accessed_address, uint32_t exception_code)
    : m_operation(operation), m_accessed_address(accessed_address), system_exception(address, exception_code, id) {}

  inline memory_operation get_operation() const { return m_operation; }
  inline const char* get_operation_string() const {
    switch (m_operation) {
      default:
      case memory_operation::unknown:
        return "accessing";
      case memory_operation::read:
        return "reading";
      case memory_operation::write:
        return "writing";
      case memory_operation::execute:
        return "executing";
    }
  }
  inline uintptr_t get_accessed_address() const { return m_accessed_address; }

  std::basic_ostream<char>& write_to(std::basic_ostream<char>& os) const {
    os << "access_violation";
    os << " at 0x" << std::hex << get_address();
    os << ": ";
    os << get_operation_string() << " ";
    os << "0x" << std::hex << m_accessed_address;

    return os;
  }

  const char* what() const { return "access violation"; }

private:
  memory_operation m_operation;
  uintptr_t m_accessed_address;
};


// Internal stuff
extern "C" {

  struct CAE_ProtectedCallContext {
    uintptr_t         stack_pointer;      // ESP/RSP register
    uintptr_t         exception_catcher;  // Where we should jump, when exception is thrown
    system_exception* thrown_exception;   // Pointer to thrown exception. Can be nullptr, if no exception thrown
  };

  // Defined in assembler
  // Throws given exception. Exception must be allocated (with `new` operator)
  // There's must be protected call context at stack (must be used inside of `CAE_PerformProtectedCall`)
  __declspec(noreturn) void __cdecl CAE_Throw(system_exception* exception);

  // Defined in assembler
  // Calls given worker in protected call context.
  // If exception was thrown, returns pointer to it. You must delete exception after use
  system_exception* __cdecl CAE_PerformProtectedCall(std::function<void()> const& worker);

  // Used by assembly to call std::function
  void __cdecl CAE_CallStdFunction(std::function<void()> const& function);

  // Used by assembly
  // Push new protected call context to stack, and return pointer to it
  CAE_ProtectedCallContext* __cdecl CAE_PushProtectedCallContext();

  // Used by assembly
  // Returns protected call context at the top of stack
  CAE_ProtectedCallContext* __cdecl CAE_GetProtectedCallContext();

  // Used by assembly
  // Pop protected call context from stack
  void __cdecl CAE_PopProtectedCallContext();

} // extern "C"
