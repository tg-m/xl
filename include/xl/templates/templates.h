
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>

#include "../regex/regexer.h"
#include "../library_extensions.h"
#include "../expected.h"


// All logging is compile-time disabled unless XL_TEMPLATE_LOG_ENABLE is specified
#if defined XL_TEMPLATE_LOG_ENABLE
#define XL_TEMPLATE_LOG(format_string, ...) \
    ::xl::templates::log.info(format_string, ##__VA_ARGS__);
#else
#define XL_TEMPLATE_LOG(format_string, ...)
#endif



namespace xl::templates {


class ErrorList {
private:
    std::vector<std::variant<std::string, ErrorList>> error_list;

public:
    ErrorList() = default;
    ErrorList(std::string error_string) {
        this->error_list.push_back(std::move(error_string));
    }

    ErrorList & append(std::string error_string) {
        this->error_list.push_back(std::move(error_string));
        return *this;
    }

    ErrorList & append(ErrorList other) {
        this->error_list.push_back(std::move(other));
        return *this;
    }

    std::vector<std::variant<std::string, ErrorList>> const & get_errors() const {
        return this->error_list;
    }
    
    std::string get_pretty_string(std::string const & indentation = "") const {
        std::string result = "";
        for (auto const & error : this->error_list) {
            if (auto const string_error = std::get_if<std::string>(&error)) {
                result += indentation + *string_error + "\n";
            } else if (auto const error_list_error = std::get_if<ErrorList>(&error)) {
                result += error_list_error->get_pretty_string(indentation + "  ");
            }
        }
        return result;
    }
    
    /**
     * Mostly for internal unit testing
     * @return number of messages contained in the object
     */
    int get_error_string_count() const {
        int result = 0;
        for (auto const & error : this->error_list) {
            if (auto const string_error = std::get_if<std::string>(&error)) {
                result += 1;
            } else if (auto const error_list_error = std::get_if<ErrorList>(&error)) {
                result += error_list_error->get_error_string_count();
            }
        }
        return result;
    }
};

inline std::ostream & operator<<(std::ostream & os, ErrorList const & error_list) {
    os << error_list.get_pretty_string();
    return os;
}


struct Substitution;
struct SubstitutionState;
class Provider_Interface;
class CompiledTemplate;
struct FillState;

class Template {
XL_PRIVATE_UNLESS_TESTING:
    
    // full, unprocessed template string
    std::string _tmpl;
    
    mutable std::shared_ptr<CompiledTemplate> 
        compiled_template;

public:
    
    explicit Template(std::string tmpl = "{{}}") : _tmpl(std::move(tmpl)) {}
    Template(Template const &) = default;
    ~Template() {
    }
    inline char const * c_str() const {
        return this->_tmpl.c_str(); 
    }

    
    template <typename ProviderContainer = void>
    xl::expected<std::string, ErrorList> fill(SubstitutionState &) const;

    
    template <typename ProviderContainer = void>
    xl::expected<std::string, ErrorList> fill(FillState const &) const;


    template<typename ProviderContainer = void, class T = char const *, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FillState>>>
    xl::expected<std::string, ErrorList> fill(T && source = "", std::map<std::string, Template> template_map = {}) const;

    
    
    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    inline xl::expected<std::shared_ptr<CompiledTemplate>, std::string> compile() const;

    inline bool is_compiled() const;
    
    static inline std::unique_ptr<Template> empty_template = std::make_unique<Template>();
};




} // end namespace xl

// need to call directly to this .h file because circular dependencies make things tricky
#include "../log/log.h"

#include "provider.h"
#include "directory_loader.h"

namespace xl::templates {

class provider_data;


template <typename ProviderContainer>
xl::expected<std::string, ErrorList> Template::fill(FillState const & fill_state) const {

    if (!this->is_compiled()) {
        this->compile();
    }
    
    return this->compiled_template->fill(fill_state);
}


template<typename ProviderContainer, class T, typename>
xl::expected<std::string, ErrorList> Template::fill(T && source, std::map<std::string, Template> template_map) const {

    auto compiled_template = this->compile();
    if (!compiled_template) {
        return xl::make_unexpected(compiled_template.error());
    }
    return (*compiled_template)->fill<ProviderContainer>(std::forward<T>(source), std::move(template_map));
}


template<typename ProviderContainer, class T, typename>
xl::expected<std::string, ErrorList> CompiledTemplate::fill(T && source, std::map<std::string, Template> template_map) const {

    XL_TEMPLATE_LOG(LogT::Subjects::Template, "Filling template: '{}'", this->source_template->c_str());

    // used for storing the provider if necessary
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;
    // used for consistent interface for assigning to reference later
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        assert(false); // this shouldn't happen anymore?
        provider_interface_pointer = &source;
        assert(provider_interface_pointer != nullptr);

    } else if constexpr(std::is_same_v<ProviderPtr, std::decay_t<T>>) {
        provider_interface_pointer = source.get();
    } else {

        // need to store the unique_ptr to maintain object lifetime then assign to normal pointer
        //   so there is a common way to get the object below for assignment to reference type
        provider_interface_unique_pointer = DefaultProviders<ProviderContainer>::template make_provider(std::forward<T>(source));
//        XL_TEMPLATE_LOG("**** got unique ptr at {}", (void *) provider_interface_unique_pointer.get());
        provider_interface_pointer = provider_interface_unique_pointer.get();

        /**
         * IF THIS IS FIRING, PERHAPS YOU ARE USING AN RVALUE PROVIDER MULTIPLE TIMES
         */
        assert(provider_interface_pointer != nullptr);


//        XL_TEMPLATE_LOG("**** set provider interface pointer to {}", (void *) provider_interface_pointer);
    }

//    XL_TEMPLATE_LOG("outside: provider interface pointer to {}", (void *) provider_interface_pointer);

    
    ProviderPtr fillable_provider = {};
    if (!provider_interface_pointer->is_fillable_provider()) {
        fillable_provider = provider_interface_pointer->get_fillable_provider();
        provider_interface_pointer = fillable_provider.get();
    }

    ProviderStack provider_stack{provider_interface_pointer};
//    std::cerr << fmt::format("ps size: {}", provider_stack.size()) << std::endl;
    FillState fill_state(std::move(provider_stack), &template_map);
//    std::cerr << fmt::format("fill state provider stack size: {}", fill_state.provider_stack.size()) << std::endl;
    return fill<ProviderContainer>(fill_state);
}


template<typename ProviderContainer>
xl::expected<std::string, ErrorList> Template::fill(SubstitutionState & substitution_state) const {
    if (auto compiled_template = this->compile()) {
        return compiled_template.error();
    } else {
        return (*compiled_template)->fill(substitution_state);
    }
}
    

template<typename ProviderContainer>
xl::expected<std::string, ErrorList> CompiledTemplate::fill(SubstitutionState & substitution_state) const {

        // if the top provider exists and wants the entire template as-is, provide it directly
    if (!substitution_state.fill_state.provider_stack.empty() && substitution_state.fill_state.provider_stack.front()->needs_raw_template()) {
        substitution_state.current_template = this;
        auto result = substitution_state.fill_state.provider_stack.front()->operator()(substitution_state);
        
        //        std::cerr << fmt::format("substitution state fill if true returning: {}\n", result);
        return result;
    } else {
        auto result = this->fill(substitution_state.fill_state);
//        std::cerr << fmt::format("substitution state fill if false returning: {}\n", result);
        return result;
    }
}



bool Template::is_compiled() const {
//    return !this->static_strings.empty() || !this->substitutions.empty();
    return this->compiled_template.get() != nullptr;
}





} // end namespace xl


#include "substitution_impl.h"