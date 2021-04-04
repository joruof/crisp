#ifndef COMPILE_AND_RUNTIME_INTROSPECTABLE_PROPERTIES
#define COMPILE_AND_RUNTIME_INTROSPECTABLE_PROPERTIES

#include <typeinfo>
#include <functional>
#include <string_view>
#include <unordered_map>

/**
 * CRISP (Compile- and Runtime IntroSpectable Properties)
 */

namespace crisp {

/**
 * This allows us to retreive the human-readable name of
 * a type at compile time. Very cool!
 *
 * Obviously not my idea. Stolen from here:
 * https://stackoverflow.com/questions/35941045/can-i-obtain-c-type-names-in-a-constexpr-way/35943472
 */

template <typename T> constexpr std::string_view type_name();

template <>
constexpr std::string_view type_name<void>()
{ return "void"; }

namespace detail {

using type_name_prober = void;

template <typename T>
constexpr std::string_view wrapped_type_name() 
{
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

constexpr std::size_t wrapped_type_name_prefix_length() { 
    return wrapped_type_name<type_name_prober>().find(type_name<type_name_prober>()); 
}

constexpr std::size_t wrapped_type_name_suffix_length() { 
    return wrapped_type_name<type_name_prober>().length() 
        - wrapped_type_name_prefix_length() 
        - type_name<type_name_prober>().length();
}

} // namespace detail

template <typename T>
constexpr std::string_view type_name() {
    constexpr auto wrapped_name = detail::wrapped_type_name<T>();
    constexpr auto prefix_length = detail::wrapped_type_name_prefix_length();
    constexpr auto suffix_length = detail::wrapped_type_name_suffix_length();
    constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
    return wrapped_name.substr(prefix_length, type_name_length);
}

/**
 * Now the actual implementation:
 *
 * First we define a compile time list of types.
 *
 * This fancy little construct was stolen from:
 * https://github.com/cbeck88/visit_struct/blob/master/include/visit_struct/visit_struct_intrusive.hpp
 *
 * They, in turn, took it from this stackoverflow post:
 * http://stackoverflow.com/questions/4790721/c-type-registration-at-compile-time-trick
 */
template <class... Ts>
struct TypeList {
      static const int size = sizeof...(Ts);
};

/*
 * This limits the maximum number of introspectable properties
 * per datatype. The actual limit depends on the maximum template
 * recursion depth your compiler supports, which is ~900 for gcc.
 * But here we choose a lower conservative estimate.
 */
enum { maxRegisteredTypes = 512 };

template <int N>
struct Rank : Rank<N - 1> {};

template <>
struct Rank<0> {};

/**
 * Meta function to append elements to type list
 */

template <class List, class T>
struct Append;

template <class... Ts, class T>
struct Append<TypeList<Ts...>, T> {
      typedef TypeList<Ts..., T> type;
};

/**
 * Used to determine at compile time if a type has properties.
 */

template< class, class = void >
struct is_crisp : std::false_type { };

template< class T >
struct is_crisp<T, std::void_t<decltype(std::declval<T&>().isCrisp())> > : std::true_type { };

/**
 * This struct encapsulates the information of a property,
 * which can be retrieved by name at runtime.
 */
struct RuntimeProperty {

    using Map = std::unordered_map<
        std::string, 
        std::function<RuntimeProperty(void*)>>;

    // the actual name of the property
    std::string name;

    // this contains the property type information
    const std::type_info* typeInfo;

    // the human-readable typename
    std::string typeName;

    // this refers to the property data
    void* data = nullptr;

    // if the property type is itself a crisp object
    // this will refer to its property map
    Map* props = nullptr;

    // this is set, if data points to a crips object
    bool isCrisp = true;

    // this will implicitly cast the runtime property
    // into the fitting datatype, a little dangerous
    // but should allow very terse syntax
    template <typename T>
    operator T() const {
        return *((T*)data);
    }
};

/**
 * This struct holds the name and a member reference
 * to the value of a property.
 *
 * C is the class this entry belongs to
 * T is the type of the field
 * K is the index of the type in the class type list
 */
template <typename C, typename T, int K>
struct Property {

    /**
     * It is really important that name is a const char*!
     * We cannot use a std::string because this would be
     * zero initialized and then overwrite the actual value.
     */
    inline static const char* name;
    inline static T C::* value;

    Property (const char* n, T C::* v) { 

        Property::name = n;
        Property::value = v;

        /**
         * Ok, so the idea is that for runtime access we somehow need
         * to store a pointer-to-member, without knowing the type
         * of the struct the member belongs to. I don't think
         * this is directly doable in current C++ versions. But we
         * can help ourselves by introducing a lambda function as
         * a layer of indirection, which resolves the pointer-to-member
         * for us, given a pointer to an object. Of course we cannot
         * access any type information at runtime, which is why the
         * lambda implementation (sadly) uses void pointers.
         */

        C::props[name] = [v]([[maybe_unused]] void* data) {

            RuntimeProperty rt;

            rt.name = name;
            rt.typeInfo = &typeid(T);
            rt.typeName = type_name<T>();

            if constexpr (!std::is_function<T>::value) {

                rt.data = &(((C*)data)->*v);

                if constexpr (is_crisp<T>::value) {
                    rt.props = &T::props;
                    rt.isCrisp = true;
                } else {
                    rt.isCrisp = false;
                }
            }

            return rt;
        };
    }
};

/**
 * Prop definition macros
 */

#define GET_REGISTERED_TYPES \
    decltype(GetTypes(::crisp::Rank<::crisp::maxRegisteredTypes>()))

/**
 * This can be used to declare and register a property.
 */

#define P(T, N, ...) T N = __VA_ARGS__; B(N)

/**
 * In contrast to the previous macro this does not declare a new
 * variable, instead it just appends an already existing variable
 * to the type list.
 *
 * One might say that B is thus a softer version of P.
 */

#define B(N) BA(N, #N)

#define BA(N, A) CRISP_BIND(decltype(N), N, A)

/**
 * This is used to bind functions.
 */

// this is used to remove the "member-" part
// from a "member-function-pointer"
template<typename T>
struct remove_member;

template<typename C, typename T>
struct remove_member<T C::*> {
    using type = T;
};

#define F(N) CRISP_BIND(::crisp::remove_member<decltype(&SelfType::N)>::type, N, #N)

#define CRISP_BIND(T, N, A) \
    inline static ::crisp::Append<GET_REGISTERED_TYPES, T>::type \
        GetTypes(::crisp::Rank<GET_REGISTERED_TYPES::size + 1>) { return {}; } \
    inline static ::crisp::Property<SelfType, T, GET_REGISTERED_TYPES::size> \
        __prop_##N{A, &SelfType::N}

/*
 * The start and end macros:
 *
 * Code that is simply pasted into the struct and contains
 * necessary property registration logic.
 */

#define CRISP_BASE(C) \
    \
    static inline ::crisp::RuntimeProperty::Map props; \
    \
    static ::crisp::TypeList<> GetTypes(::crisp::Rank<0>) { return {}; } \

#define CRISP_START(C) \
    \
    using SelfType = C; \
    using SuperType = C; \
    \
    CRISP_BASE(C) \

#define CRISP_INHERIT(C, S) \
    \
    using SelfType = C; \
    using SuperType = S; \
    \
    CRISP_BASE(C) \

#define CRISP_END \
    \
    typedef GET_REGISTERED_TYPES typesList; \
    \
    bool isCrisp () const { \
        return true; \
    } \

} // namespace crisp

#endif
