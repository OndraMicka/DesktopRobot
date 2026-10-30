#pragma once
#include "../SharedGraphics.h"
namespace poses {
using namespace desktoprobot;
inline constexpr DrawCommand idle[] = {
 roundedRectangle(18,11,32,28,12), roundedRectangle(78,11,32,28,12), roundedRectangle(54,49,20,5,2)
};
inline constexpr DrawCommand closed[] = {
 roundedRectangle(18,24,32,5,2), roundedRectangle(78,24,32,5,2), roundedRectangle(54,49,20,5,2)
};
template <std::size_t N> constexpr Frame frame(uint32_t ms, const DrawCommand (&commands)[N]) {
 return {ms, commands, N};
}
}
