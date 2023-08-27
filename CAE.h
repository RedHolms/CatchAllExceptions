#pragma once
#define CAE_included

#include <stddef.h>

#include <string>
#include <exception>
#include <ostream>
#include <vector>
#include <sstream>
#include <functional>
#include <typeinfo>

class system_exception : public std::exception {
public:
  static constexpr uint32_t id = -1;

public:
  system_exception(uintptr_t address, uint32_t exception_code, uint32_t id)
    : m_address(address), m_exception_code(exception_code), m_id(id) {}

  inline uintptr_t get_address() const { return m_address; }
  inline uint32_t get_exception_code() const { return m_exception_code; }
  inline uint32_t get_id() const { return m_id; }

  inline std::string to_string() const {
    std::stringstream ss;
    write_to(ss);
    return ss.str();
  }

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

  class context;

  void ProtectedCall(context& ctx);

  class context {
    friend void ProtectedCall(context&);

    struct exception_handler_description {
      using handler_type = void(system_exception&);
      using handler_function_type = std::function<handler_type>;

      template <typename T>
      inline exception_handler_description(std::function<void(T&)> const& handler)
        : exception_type_id(T::id),
        handler(
          [handler](system_exception& ex) {
            if (handler)
              handler(dynamic_cast<T&>(ex));
          }
        )
      {
        static_assert(std::is_base_of_v<system_exception, T>, "You should catch only system exceptions!");
      }

      uint32_t exception_type_id;
      handler_function_type handler;
    };

  public:
    inline context(std::function<void()> const& worker)
      : m_worker(worker) {}

    inline ~context() {
      ProtectedCall(*this);
    }

  public:
    template <typename T>
    inline context& add_catch(std::function<void(T&)> const& handler) {
      m_exceptions_handlers.emplace_back(handler);
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

  private:
    std::function<void()> m_worker;
    std::vector<exception_handler_description> m_exceptions_handlers;
    std::function<void()> m_finally;
    std::function<void()> m_else;
  };

}

#define cae_try CAE::context([&]()
#define cae_catch(type, ex) ).add_catch<type>([&](type& ex)
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

  virtual std::basic_ostream<char>& write_to(std::basic_ostream<char>& os) const {
    os << "access_violation";
    os << " at 0x" << std::hex << get_address();
    os << ": ";
    os << get_operation_string() << " ";
    os << "0x" << std::hex << m_accessed_address;

    return os;
  }

  virtual const char* what() const { return "access violation"; }

private:
  memory_operation m_operation;
  uintptr_t m_accessed_address;
};
