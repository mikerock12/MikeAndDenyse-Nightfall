#pragma once
#include <span>

namespace SLTBranding {
// Views immutable PNG data embedded in this executable; no external file needed.
std::span<const unsigned char> ApplicationIconPng();
}
