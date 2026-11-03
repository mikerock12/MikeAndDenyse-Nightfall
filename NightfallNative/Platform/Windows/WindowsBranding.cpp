#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "WindowsBranding.h"

namespace SLTBranding {
std::span<const unsigned char> ApplicationIconPng() {
    const HMODULE module = GetModuleHandleW(nullptr);
    const HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(10));
    if (!resource) return {};
    const DWORD size = SizeofResource(module, resource);
    const HGLOBAL data = LoadResource(module, resource);
    if (!data || !size) return {};
    const auto* bytes = static_cast<const unsigned char*>(LockResource(data));
    return bytes ? std::span<const unsigned char>(bytes, size) : std::span<const unsigned char>{};
}
}
