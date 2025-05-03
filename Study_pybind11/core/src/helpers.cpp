#include "helpers.h"

#include <stdio.h>

#include <codecvt>
#include <locale>

namespace strings {

std::string DateToRFC3399(const std::string &str) {
  if (str.empty()) return str;
  std::string s = str;
  if (strings::StartsWith(s, "D:")) {
    s.erase(0, 2);
  }
  s.insert(4, "-");
  s.insert(7, "-");
  s.insert(10, "T");
  s.insert(13, ":");
  s.insert(16, ":");
  strings::ReplaceAll(s, "'", ":");
  if (strings::EndsWith(s, ":")) {
    s.erase(s.size()-1);
  }
  return s;
}

}  // namespace fpdf

namespace strings {

// ref:
//  https://en.cppreference.com/w/cpp/locale/codecvt_utf8_utf16
//  https://en.cppreference.com/w/cpp/locale/wstring_convert/to_bytes
std::string FromUtf16(const std::u16string &u16str) {
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
  return conv.to_bytes(u16str);
}

std::u16string Utf8ToUtf16(const std::string& str){
    //std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    std::u16string utf16 = std::wstring_convert<
           std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(str.data());
    return utf16;
}

// ref: https://stackoverflow.com/a/42844629
bool EndsWith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
      0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}
bool StartsWith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() &&
      0 == str.compare(0, prefix.size(), prefix);
}

// ref: https://en.cppreference.com/w/cpp/string/basic_string/replace
std::size_t ReplaceAll(std::string &inout,
    const std::string &what, const std::string &with) {
  std::size_t count{};
  for (std::string::size_type pos{};
      inout.npos != (pos = inout.find(what.data(), pos, what.length()));
      pos += with.length(), ++count) {
    inout.replace(pos, what.length(), with.data(), with.length());
  }
  return count;
}

}  // namespace strings
