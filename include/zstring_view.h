#pragma once

#include <string>

namespace xl {
/**
 * string_view that is guaranteed to be null terminated.
 * adds c_str() as a synonym for std::string_view::data to make it clear that it is null terminated like it is
 *   in std::string::c_str()
 */
template<
    class CharT,
    class Traits = std::char_traits<CharT>>
class basic_zstring_view : public std::basic_string_view<CharT, Traits> {

public:
    using std::basic_string_view<CharT, Traits>::basic_string_view;

    // There is intentionally no constructor that takes a std::string_view because it's not guaranteed to be
    //   NUL terminated and there's nowhere to store a memory allocation to store a copy of the string with a NUL

    CharT const * c_str() const {return this->data();}

    operator std::basic_string<CharT>() const {return std::basic_string<CharT>(*this);}
};

using zstring_view = basic_zstring_view<char>;
using wzstring_view = basic_zstring_view<wchar_t>;
using u16zstring_view = basic_zstring_view<char16_t>;
using u32zstring_view = basic_zstring_view<char32_t>;

}