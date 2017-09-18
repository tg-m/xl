#pragma once

namespace xl {

/**
 * string_view that is guaranteed to be null terminated.
 * adds c_str() as a synonym for std::string_view::data to make it clear that it is null terminated like it is
 *   in std::string::c_str()
 */
class zstring_view : public std::string_view {

public:
    zstring_view() = default;

    // must call strlen to compute length
    zstring_view(char const * const source) : zstring_view(source, strlen(source)) {}

    // more efficient if the length of the char* is already known
    zstring_view(char const * const source, std::size_t length) : std::string_view(source, length) {}

    zstring_view(std::string const & source) : std::string_view(source.c_str(), source.length()) {}


    zstring_view(zstring_view const &) = default;
    zstring_view(zstring_view &&) = default;
    zstring_view & operator=(zstring_view const &) = default;
    zstring_view & operator=(zstring_view &&) = default;

    char const * c_str() const {
        return this->data;
    }

    operator std::string() const {
        return std::string(this->data(), this->length());
    }

    // no need to convert to std::string_view, it already is
};


} // end namespace xl

