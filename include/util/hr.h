#pragma once
#include <Windows.h>
#include <stdexcept>
#include <format>

namespace util {

class HrException final : public std::runtime_error
{
public:
    HrException(HRESULT hr, const char* file, int line)
        : std::runtime_error(std::format("HRESULT 0x{:08X} at {}:{}", static_cast<unsigned>(hr), file, line))
        , m_hr(hr)
    {}

    HRESULT hr() const { return m_hr; }

private:
    HRESULT m_hr;
};

} // namespace util

#define HR(expr) \
    do { \
        const HRESULT _hr = (expr); \
        if (FAILED(_hr)) throw ::util::HrException(_hr, __FILE__, __LINE__); \
    } while(0)
