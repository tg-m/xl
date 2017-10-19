
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <regex>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>
#include <regex/regexer.h>

#include "../library_extensions.h"
#include "../regex/regexer.h"


namespace xl::templates {

class ProviderData;
class Provider_Interface;


class Template {
private:
    std::string _tmpl;

    // static portions of the template as separated by substitutions
    mutable std::vector<std::string> compiled_static_strings;

    // substitutions in the template
    mutable std::vector<ProviderData> compiled_substitutions;

    // sum of the length of all the fixed portions of the template
    mutable size_t minimum_result_length = 0;

public:
    Template(std::string const & tmpl) : _tmpl(tmpl) {}
    Template(std::string && tmpl = "") : _tmpl(std::move(tmpl)) {}

    inline char const * c_str() const { return this->_tmpl.c_str(); }

//    std::string fill(Provider_Interface const & interface = EmptyProvider{}, std::map<std::string, Template> const & templates = {});

    template<class T = std::string>
    std::string fill(T && source = T{}, std::map<std::string, Template> const & templates = {}) const;

    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    inline void compile() const;

    inline bool is_compiled() const;
};

using TemplateMap = std::map<std::string, Template>;

} // end namespace xl

#include "provider_data.h"
#include "provider.h"
#include "directory_loader.h"

namespace xl::templates {

//std::string Template::fill(Provider_Interface const & interface, std::map<std::string, Template> const & templates) {
//    return this->fill<>(interface, templates);
//}

template<class T>
std::string Template::fill(T && source, TemplateMap const & templates) const {
    if (!this->is_compiled()) {
        this->compile();
    }

//    std::cerr << fmt::format("filling template: '{}'", this->_tmpl) << std::endl;

    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        provider_interface_pointer = & source;
    } else {
        provider_interface_unique_pointer = make_provider(source);
        provider_interface_pointer = provider_interface_unique_pointer.get();
    }
    Provider_Interface & provider = *provider_interface_pointer;
//    std::cerr << fmt::format("providers vector size: {}", this->providers.size()) << std::endl;

    std::string result{};
    result.reserve(this->minimum_result_length);

    bool previous_static_string_ended_with_newline = false;

    for(int i = 0; i < this->compiled_static_strings.size(); i++) {

        previous_static_string_ended_with_newline = false;
        auto & static_string = this->compiled_static_strings[i];
        if (static_string[static_string.length()-1]=='\n') {
            std::cerr << fmt::format("static string ends in newline") << std::endl;
            previous_static_string_ended_with_newline = true;
        }

        result.insert(result.end(), this->compiled_static_strings[i].begin(), this->compiled_static_strings[i].end());

        if (this->compiled_substitutions.size() > i) {
            auto & data = this->compiled_substitutions[i];

            if (data.template_name != "") {
                if (templates.empty()) {
                    throw TemplateException("Cannot refer to another template if no other templates specified: " + data.template_name);
                }
                auto template_iterator = templates.find(data.template_name);
                if (template_iterator == templates.end()) {
                    throw TemplateException("No template found named: " + data.template_name);
                }
                auto inline_template_result = template_iterator->second.fill(provider, templates);
                result.insert(result.end(), begin(inline_template_result), end(inline_template_result));

            } else {

                auto substitution_result = provider(
                    ProviderData(data.name, &templates, data.parameters, data.inline_template).set_ignore_empty_replacements(data.ignore_empty_replacements));
                result.insert(result.end(), substitution_result.begin(), substitution_result.end());
            }
        }
    }
    return result;
}


bool Template::is_compiled() const {
    return !this->compiled_static_strings.empty() || !this->compiled_substitutions.empty();
}


void Template::compile() const {



/*
 * this isn't useable by std::regex, but can be pasted into regex101.com to work with
 * Then when you're done, trim out the whitespace and comments to generate the std::regex below
 * https://regex101.com/r/fz8g7j

 *** MISSING ***
 */

    // DO NOT EDIT THIS DIRECTLY, EDIT THE COMMENTED VERSION ABOVE AND THEN COPY IT AND TRIM OUT THE WHITESPACE AND COMMENTS
    static xl::Regex r(R"(

(?(DEFINE)(?<NotEmptyAssertion>(?=(?:.|\n))))
(?(DEFINE)(?<OpenDelimiterHead>\{))
(?(DEFINE)(?<CloseDelimiterHead>\}))
(?(DEFINE)(?<OpenDelimiterTail>\{))
(?(DEFINE)(?<CloseDelimiterTail>\}))

(?(DEFINE)(?<OpenDelimiter>(?&OpenDelimiterHead)(?&OpenDelimiterTail)))
(?(DEFINE)(?<CloseDelimiter>(?&CloseDelimiterHead)(?&CloseDelimiterTail)))
(?(DEFINE)(?<EitherDelimiter>( (?&OpenDelimiter) | (?&CloseDelimiter))))
(?(DEFINE)(?<UntilDoubleBrace>(?:\s|\\(?&OpenDelimiterHead)|\\(?&CloseDelimiterHead)|[^{}]|[{](?!\{)|[}](?!\}))*))
(?(DEFINE)(?<UntilEndOfLine>[^\n]*\n))



(?&NotEmptyAssertion)
(?<Literal>(?&UntilDoubleBrace))
(?:(?<Substitution>(?<OpenDelimiterHere>\{\{)\s*

(?<TemplateInsertionMarker>!)?

# Everything up to the optional |
(?:(?<SubstitutionName>(?:\\\}|\\\{|[^|\s])*?)\s*(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\||$))

 (?<PIPE>[|]

# Everything after the | before the }}
 (?<InlineTemplateMarker>!)?
 (?<IgnoreEmptyMarker><)?
 (?<IgnoreWhitespaceTilEndOfLine>!(?&UntilEndOfLine))?
(?<SubstitutionData>((?&UntilDoubleBrace)(?&Substitution)?)*)

   # end "stuff after |"
)? # end PIPE
  (?<CloseDelimiterHere>\}\})
 ) # end Substition
| (?<UnmatchedOpen>\{\{) | (?<UnmatchedClose>\}\}) | $)


)",
        xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);



    // turn double curlies {{ or }} into { or } and anything with a backslash before it into just the thing
    //   after the backslash
    static std::regex post_process_regex("(?:\\\\(.))");



    std::string remaining_template = this->c_str();
    std::cerr << fmt::format("compiling template: '{}'", this->_tmpl.c_str()) << std::endl;

    while (auto matches = r.match(remaining_template)) {

        for(auto [s,i] : each_i(matches.get_all_matches())) {
            if (s != "") {
                std::cerr << fmt::format("{}: '{}'", i, s) << std::endl;
            }
        }
        std::cerr << fmt::format("literal: '{}', substutition: '{}'",
                                 matches["Literal"],
                                 matches["Substitution"]
        ) << std::endl;

//        for (int i = 0; i < matches.size(); i++) {
//            std::cerr << fmt::format("match[{}]: '{}'", i, matches[i].str()) << std::endl;
//        }


        // check for open but no close or incorrect brace type
        if (matches.length("UnmatchedOpen")) {
            throw TemplateException("Unmatched Open");
        }
        if (matches.length("UnmatchedClose")) {
            throw TemplateException("Unmatched Close");
        }


        // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
        //   this loop
        if (matches.length("Substitution") == 0) {
            break;
        }


        remaining_template = matches.suffix();

        std::string literal_string = matches["Literal"];
        std::cerr << fmt::format("postprocessing: '{}'", literal_string) << std::endl;

        literal_string = std::regex_replace(literal_string, post_process_regex, "$1");
        std::cerr << fmt::format("into: '{}'", literal_string) << std::endl;

        this->compiled_static_strings.push_back(literal_string);
        this->minimum_result_length += this->compiled_static_strings.back().size();

        bool ignore_empty_replacements = matches.length("IgnoreEmptyMarker");
        std::cerr << fmt::format("ignoring empty replacements? {}", ignore_empty_replacements) << std::endl;

//        // if there was an inline template specified
        if (matches.length("TemplateInsertionMarker") == 1) {
            this->compiled_substitutions.emplace_back("", nullptr, "", std::optional<Template>(), matches["SubstitutionName"]).set_ignore_empty_replacements(ignore_empty_replacements);
        }else if (matches.length("InlineTemplateMarker") == 1) {
            this->compiled_substitutions.emplace_back(matches["SubstitutionName"], nullptr, "", Template(matches["SubstitutionData"])).set_ignore_empty_replacements(ignore_empty_replacements);
        } else {
            this->compiled_substitutions.emplace_back(matches["SubstitutionName"], nullptr, matches["SubstitutionData"]).set_ignore_empty_replacements(ignore_empty_replacements);
        }

//            if (!provider.provides(matches[REPLACEMENT_NAME_INDEX])) {
//                throw TemplateException(
//                    fmt::format("Provider doesn't provide value for name: '{}'", matches[REPLACEMENT_NAME_INDEX]));
//            }

//            if (matches[REPLACEMENT_OPTIONS_INDEX].str() != "") {
//                std::cerr << fmt::format("GOT REPLACEMENT OPTIONS: {}", matches[REPLACEMENT_OPTIONS_INDEX].str()) << std::endl;
//            }


    }

std::cerr << fmt::format("remaining template: '{}'", remaining_template) << std::endl;
    std::cerr << fmt::format("pushing on remaining template: '{}'", std::regex_replace(remaining_template, post_process_regex, "$1")) << std::endl;
    this->compiled_static_strings.push_back(std::regex_replace(remaining_template, post_process_regex, "$1"));
}

} // end namespace xl


/*
Attempt at using "fancy" regex stuff
(?(DEFINE)(?<NotEmptyAssertion>(?=(?:.|\n))))
(?(DEFINE)(?<OpenDelimiterHead>\{))
(?(DEFINE)(?<CloseDelimiterHead>\}))
(?(DEFINE)(?<OpenDelimiterTail>\{))
(?(DEFINE)(?<CloseDelimiterTail>\}))

(?(DEFINE)(?<OpenDelimiter>(?&OpenDelimiterHead)(?&OpenDelimiterTail)))
(?(DEFINE)(?<CloseDelimiter>(?&CloseDelimiterHead)(?&CloseDelimiterTail)))
(?(DEFINE)(?<EitherDelimiter>(
  (?&OpenDelimiter) |
  (?&CloseDelimiter))))
(?(DEFINE)(?<UntilDoubleBrace>(?:\s|\\(?&OpenDelimiter)|\\(?&CloseDelimiter)|[^{}]|[{](?!\{)|[}](?!\}))*))
(?(DEFINE)(?<TemplateSubstitutionMarker>!))
(?(DEFINE)(?<InlineTemplateMarker>!))
(?(DEFINE)(?<UntilEndOfLine>![^\n]*\n))

(?(DEFINE)(?<SubstitutionPattern>(?&OpenDelimiter)((?&TemplateSubstitutionMarker)?)

# Everything up to the optional |
((?:\\\}|\\\{|[^|])*?(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\||$))

 (?:[|]

# Everything after the | before the }}
 (!?)
 (?:!(?&UntilEndOfLine))?
((?:\\\}|\\\{|[^|])*?(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\|))

  )? # end "stuff after |"

  (?&CloseDelimiter)
 ) # end Substitution named pattern
) # end DEFINE Substitution

^(?&NotEmptyAssertion)
(?<StringLiteral>(?&UntilDoubleBrace))
(?<Substitution>(?&CloseDelimiter)|(?&SubstitutionPattern)|$)

 */