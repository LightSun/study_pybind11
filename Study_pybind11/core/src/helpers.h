#pragma once

#include <string>
#include "core/src/irange.h"

#define MFOREACH(t) c10::irange(t)
#define MRFOREACH(t) c10::irange_reverse(t)

namespace strings {

std::string DateToRFC3399(const std::string &str);

std::string FromUtf16(const std::u16string &u16str);

std::u16string Utf8ToUtf16(const std::string& str);

bool EndsWith(const std::string &str, const std::string &suffix);
bool StartsWith(const std::string &str, const std::string &prefix);

std::size_t ReplaceAll(std::string &str,
    const std::string &what, const std::string &with);

}  // namespace strings
