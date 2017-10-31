#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <iostream>

#include "../library_extensions.h"
#include "../magic_ptr.h"
#include "../demangle.h"

#include "exceptions.h"
#include "template.h"
#include "provider_data.h"


namespace xl::templates {


template<typename ProviderContainer>
struct DefaultProviders {

    template<class T, class = void>
    class Provider {
        static_assert(!std::is_pointer_v<T>, "make sure the type of the dereferenced pointer has a get_provider method for it");
        Provider(T &&);
        Provider(T &);
        Provider(T *);
    };


//    template<class... Keys, class... Values>
//    Provider(std::pair<Keys, Values>
//        &&...) ->
//    Provider<std::map<std::string, ProviderPtr>>;


    template<class T, class = ProviderContainer, class = void>
    struct has_get_provider_in_provider_container : public std::false_type {};

    template<class T, class PC_COPY>
    struct has_get_provider_in_provider_container<T, PC_COPY, std::void_t<decltype(PC_COPY::get_provider(std::declval<T>()))>> : public std::true_type {};

    template<class T>
    static constexpr bool has_get_provider_in_provider_container_v = has_get_provider_in_provider_container<T>::value;



    template<class, class = void>
    class is_provider_type : public std::false_type {
    };

    template<class T>
    class is_provider_type<T, std::enable_if_t<std::is_same_v<
        std::string,
            std::result_of_t<Provider < std::remove_reference_t<T>>(ProviderData const &)
                >
            > // is same
        > // enable_if
    > : public std::true_type {};

    template<class T>
    static constexpr bool is_provider_type_v = is_provider_type<T>::value;


    template<class T, class = void>
    class is_provider_callback_type : public std::false_type {
    };

    template<class T>
    class is_provider_callback_type<T, std::enable_if_t<
        is_provider_type_v<
            std::result_of_t<
                T()
            > // result_of
        > // is_provider_type
    > // enable_if
    > : public std::true_type {
    };

    template<class T>
    static constexpr bool is_provider_callback_type_v = is_provider_callback_type<T>::value;



    template<class, class = void>
    struct has_get_provider_member : public std::false_type {
    };
    template<class T>
    struct has_get_provider_member<T, std::enable_if_t<std::is_same_v<
        ProviderPtr,
        decltype(std::declval<T>().get_provider())>>> : public std::true_type {
    };

    template<class T>
    static inline constexpr bool has_get_provider_member_v = has_get_provider_member<T>::value;


    template<class, class = void>
    struct has_get_provider_free_function : public std::false_type {
    };

    template<class T>
    struct has_get_provider_free_function<T, std::enable_if_t<std::is_same_v<
        ProviderPtr,
        decltype(get_provider(std::declval<T>()))> // end is_same
    > // end enable_if
    > : public std::true_type {
    };

    template<class T>
    static inline constexpr bool has_get_provider_free_function_v = has_get_provider_free_function<T>::value;


    template<class, class = void>
    struct can_get_provider_for_type : public std::false_type {
    };

    template<class T>
    struct can_get_provider_for_type<T, std::enable_if_t<
        std::disjunction_v< // logical OR
            has_get_provider_member<T>,
            has_get_provider_free_function<T>,
            has_get_provider_in_provider_container<T>
        > // end disjunction
    > // end enable_if
    > : public std::true_type {
    };


    template<class T>
    static inline constexpr bool can_get_provider_for_type_v = can_get_provider_for_type<T>::value;

    template<class T>
    class Provider<T, std::enable_if_t<std::is_convertible_v<T, std::string>>> : public Provider_Interface {
        std::string string;

    public:
        Provider(T string) : string(std::move(string)) {
            XL_TEMPLATE_LOG("Created string provider with: '{}'", this->string);
        }

        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed string provider for string '{}'", this->string);
        }

        std::string operator()(ProviderData const & data) override {
            return this->string;
        }

        std::string get_name() const override {
            return fmt::format("Provider: {} is convertible to std::string: '{}'", demangle<T>(), string);
        }
    };


    template<class... Ts, std::enable_if_t<std::conjunction_v<is_pair<Ts>...> && !(sizeof...(Ts) <= 1), int> = 0>
    static ProviderPtr make_provider(Ts&&... pairs) {
        return ProviderPtr(
            new Provider<std::map<std::string, ProviderPtr>>(std::forward<Ts>(pairs)...));
    };




    /**
     * Callback Provider
     * @tparam T Callback type
     */
    template<class T>
    class Provider<T, std::enable_if_t<is_provider_callback_type_v<T>>> : public Provider_Interface {

        using NoRefT = std::remove_reference_t<T>;
        NoRefT callback;

        using CallbackResultT = std::remove_reference_t<std::result_of_t<T()>>;

    public:
        Provider(std::remove_reference_t<T> && callback) : callback(std::move(callback)) {
            XL_TEMPLATE_LOG("Created callback provider with rvalue callback");
        }

        Provider(std::remove_reference_t<T> & callback) : callback(callback) {
            XL_TEMPLATE_LOG("Created callback provider with lvalue callback");
        }
        
        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed callback provider");
        }


        std::string operator()(ProviderData const & data) override {
            auto result = this->callback();
            return Provider<CallbackResultT>(std::move(result))(data);
        }

        std::string get_name() const override {
            return fmt::format("Provider: Callback {} returning Providerable type {}", demangle<NoRefT>(), demangle<CallbackResultT>());
        }
    };




    template<class T>
    static ProviderPtr make_provider(T && t) {
        if constexpr(std::is_same_v<std::decay_t<T>, ProviderPtr>) {
            return std::move(t);
        } else if constexpr(is_pair_v<T>) {
            std::map<std::string, decltype(t.second)> map;

//            std::cerr << fmt::format("ref {} rvalue {} lvalue {}",
//            std::is_reference_v<T&&>,
//            std::is_rvalue_reference_v<T&&>,
//            std::is_lvalue_reference_v<T&&>) << std::endl;

//            if constexpr(std::is_rvalue_reference_v<T>) {
                map.insert(std::forward<T>(t));
//            } else {
//                map.emplace(t.first, t.second);
//            }
            return ProviderPtr(new Provider<decltype(map)>(std::move(map)));
        } else {
            return ProviderPtr(new Provider<T>(std::forward<T>(t)));
        }
    }

    template<class R, class... Args, std::enable_if_t<is_provider_type_v<R>, int> = 0>
    static ProviderPtr make_provider(R(* f)(Args...)) {
        return ProviderPtr(new Provider<std::function<R(Args...)>>(std::function<R(Args...)>(f)));
    };



    /**
     * get_provider Provider
     * @tparam T type which can have get_provider called with/on it
     */
    template<class T>
    class Provider<T, std::enable_if_t<can_get_provider_for_type_v<T>>> : public Provider_Interface {
        using NoRefT = std::remove_reference_t<T>;
        static_assert(!std::is_pointer_v<T>, "do not make get_provider for pointer types");
    private:
        xl::magic_ptr<NoRefT> t;

    public:

        using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

        Provider(NoRefT && t) : t(std::move(t)) {
            XL_TEMPLATE_LOG("Created can_get_provider Provider with rvalue {} moved to {}", (void*)&t, (void*)this->t.get());
        }

        Provider(NoRefT & t) : t(t) {
            XL_TEMPLATE_LOG("Created can_get_provider Provider with lvalue {}", (void*)this->t.get());
        }
        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed can_get_provider Provider for value at {}", (void*)this->t.get());
        }


        std::string operator()(ProviderData const & data) override {
            ProviderPtr provider = get_underlying_provider();
//            std::cerr << fmt::format("got underlying provider name: {}", provider->get_name()) << std::endl;
//            std::cerr << fmt::format("t.get: {}", (void*)this->t.get()) << std::endl;

            if (data.inline_template) {
                return data.inline_template->fill<ProviderContainer>(provider);
            } else {
                return provider->operator()(data);
            }
        }

        auto get_underlying_provider() {
            if constexpr(has_get_provider_free_function_v<NoRefT>)
            {
                return get_provider(*t);
            } else if constexpr(has_get_provider_member_v<NoRefT>)
            {
                return t->get_provider();
            } else if constexpr(has_get_provider_in_provider_container_v<NoRefT>) {
                return ProviderContainer::get_provider(*t);
            } else {
                throw xl::templates::TemplateException("this shouldn't happen");
            }
        }

        std::string get_name() const override {
            return fmt::format("Provider: can_get_provider_for_type_v<{}>", demangle<T>());
        }

    };



    /**
     * Unique_ptr Provider
     * @tparam T
     * @tparam Deleter
     */
    template<class UniquePtrT>
    class Provider<UniquePtrT, std::enable_if_t<is_template_for_v<std::unique_ptr, UniquePtrT>>> : public Provider_Interface {
        using T = unique_ptr_type_t<UniquePtrT>;
        T & t;
    public:
        using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

        Provider(UniquePtrT & t) :
            t(*t) {
            XL_TEMPLATE_LOG("Created std::unique_ptr Provider for value at {}", (void*)&this->t);
        }

        ~Provider() {
            XL_TEMPLATE_LOG("unique_ptr provider destructor called");
        }

        std::string operator()(ProviderData const & data) override {
            return make_provider(t)->operator()(data);
        }

        auto get_underlying_provider() {
            return Provider<T>(t);
        }
        std::string get_name() const override {
            return fmt::format("Provider: unique_ptr<{}> passthrough", demangle<T>());
        }

    };

    static_assert(is_passthrough_provider_v<Provider<std::unique_ptr<int>>>);
    static_assert(is_passthrough_provider_v<Provider<std::unique_ptr<int> const>>);



    /**
     * Pointer Provider -- except char (const) *
     * @tparam T
     */
    template<class T>
    class Provider<T, std::enable_if_t<
            std::is_pointer_v<T> &&
            !std::is_same_v<std::decay_t<std::remove_pointer_t<T>>, char>>
        > : public Provider_Interface
    {

        using NoPtrT = std::remove_pointer_t<T>;

        NoPtrT * const t;
        static_assert(is_provider_type_v<NoPtrT>, "make sure the ProviderContainer was specified, if needed");

    public:
        Provider(NoPtrT * const t) : t(t) {
            XL_TEMPLATE_LOG("Creating pointer provider with pointer to {}", (void*)this->t);
        }

        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed pointer Provider for {}", (void*)this->t);
        }

        std::string operator()(ProviderData const & data) override {
            return Provider<NoPtrT>(*t)(data);
        }

        std::string get_name() const override {
            return "Pointer provider";
        }

        using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

        auto get_underlying_provider() {
            return Provider<NoPtrT>(*t);
        }
    };


    /**
     * Container Provider
     * @tparam T Container type
     */
    template<class T>
    class Provider<T, std::enable_if_t<xl::is_range_for_loop_able_v<T> &&
                                       !std::is_convertible_v<T, std::string> && // std::string is iteratable
                                       !is_map_v<T>>> // maps are handled differently
        : public Provider_Interface {

        using NoRefT = std::remove_reference_t<T>;
    private:
        magic_ptr<NoRefT> t;
    public:

        Provider(NoRefT & t) : t(t) {
            XL_TEMPLATE_LOG("Created container Provider for lvalue at {}", (void*)this->t.get());
        }

        Provider(NoRefT && t) : t(std::move(t)) {
            XL_TEMPLATE_LOG("Created container Provider for rvalue moved from {} to {}", (void*)&t, (void*)this->t.get());

        }

        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed container provider for container at {}", (void*)this->t.get());
        }


        std::string operator()(ProviderData const & data) override {
            XL_TEMPLATE_LOG("container provider looking at substution data for: {}, {}", data.name, (bool)data.inline_template);
            std::stringstream result;


            XL_TEMPLATE_LOG("inline template exists? {}", (bool)data.inline_template);
            Template const & tmpl = [&] {
                if (data.inline_template) {
                    return *data.inline_template;
                } else {
                    auto template_iterator = data.templates->find(data.parameters);
                    if (template_iterator == data.templates->end()) {
                        if (data.templates->empty()) {
                            throw TemplateException(
                                "ContainerProvider received empty template map so it can't possibly find a template for its members" +
                                data.name);
                        }
                        throw TemplateException(
                            fmt::format("ContainerProvider couldn't find template named: '{}'", data.parameters));
                    }
                    return template_iterator->second;
                }
            }();

            bool needs_join_string = false;

            // Iterate through the container
            std::cerr << fmt::format("provider iterator iterating through container of size {}", t->size()) << std::endl;
            for (auto & element : *t) {
                if constexpr(std::is_pointer_v<std::remove_reference_t<decltype(element)>>) {
                    std::cerr << fmt::format("pointer element {}", (void*)element) << std::endl;
                } else {
                    std::cerr << fmt::format("non-pointer element {}", (void*)&element) << std::endl;

                }
                auto p = Provider<std::remove_reference_t<decltype(element)>>(element);

                if (needs_join_string) {
                    result << data.join_string;
                    XL_TEMPLATE_LOG("inserting join string '{}' on subsequent pass", data.join_string);
                } else {
                    XL_TEMPLATE_LOG("skipping join string '{}' on first pass", data.join_string);
                }


                needs_join_string = true;
                auto fill_result = tmpl.fill<ProviderContainer>(p, *data.templates);

                XL_TEMPLATE_LOG("replacement is {}, ignore is {}", fill_result, data.ignore_empty_replacements);
                if (fill_result == "" && data.ignore_empty_replacements) {
                    needs_join_string = false;
                }
                result << fill_result;

            }
            return result.str();
        }

        std::string get_name() const override {
            return fmt::format("Provider: container of {}", demangle<T>());
        }
    };



    /**
     * Map Provider
     * @tparam Key
     * @tparam Value
     */
    template<class Key, class Value>
    class Provider<std::map<Key, Value>> : public Provider_Interface {
    public:

        using MapT = std::map<Key, Value>;

        MapT map;

        template<class... Keys, class... Values>
        Provider(std::pair<Keys, Values> && ... pairs) {

            (this->map.emplace(std::move(pairs.first), make_provider(pairs.second)),...);

            XL_TEMPLATE_LOG("done adding pairs to map at: {}", (void*)this);
            XL_TEMPLATE_LOG("map size: {}", this->map.size());
            for(auto const & [a,b] : this->map) {
                XL_TEMPLATE_LOG("{}: {}", a, (void*)b.get());
            }
        }

        Provider(std::map<std::string, Value> map) :
            map(std::move(map))
        {
            XL_TEMPLATE_LOG("added entire map into map provider");

            XL_TEMPLATE_LOG("done moving map into map");
            XL_TEMPLATE_LOG("map size: {}", this->map.size());
            for(auto const & [a,b] : this->map) {
                XL_TEMPLATE_LOG("{}: {}", a, (void*)&b);
            }
        }

        ~Provider() {
            XL_TEMPLATE_LOG("std::map provider destructor called for provider at {}", (void*)this);
        }


        std::string operator()(ProviderData const & data) override {
            auto provider_iterator = this->map.find(data.name);
            if (provider_iterator != this->map.end()) {

                if constexpr(std::is_base_of_v<Provider_Interface, Value>) {
                    XL_TEMPLATE_LOG("value is a provider interface");
                    return provider_iterator->second()(data);
                } else if constexpr(std::is_same_v<ProviderPtr, Value>) {
                    XL_TEMPLATE_LOG("value is a unique_ptr<provider interface>");
                    return provider_iterator->second->operator()(data);
                } else {
                    XL_TEMPLATE_LOG("value needs to be converted to provider");
                    return Provider<Value>(provider_iterator->second)(data);
                }

            } else {
                XL_TEMPLATE_LOG("in map:");
                for(auto const & [k,v] : this->map) {
                    XL_TEMPLATE_LOG("key: {}", k);
                }
                std::string template_text = "<unknown template name>";
                if (data.current_template != nullptr) {
                    template_text = data.current_template->c_str();
                }
                throw TemplateException("provider " + this->get_name() + " does not provide name: '" + data.name + "' - in template: '" + template_text + "'");
            }
        }


        std::string get_name() const override {

            std::string result = fmt::format("Provider: map ({}) with keys:", (void*)this);
            for(auto const & [key, value] : this->map) {
                result += " " + key;
            }
            return result;
        }

    };



    static_assert(!can_get_provider_for_type_v<std::string>);
    static_assert(is_provider_type_v<std::string>);
    static_assert(is_provider_callback_type_v<std::__1::function<std::__1::basic_string<char>()> &>);

};


template<typename ProviderContainer = void, typename... Ts>
ProviderPtr make_provider(Ts&&... ts) {
    return DefaultProviders<ProviderContainer>::make_provider(std::forward<Ts>(ts)...);
}


} // end namespace xl