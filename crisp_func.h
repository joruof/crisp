#ifndef COMPILE_AND_RUNTIME_INTROSPECTABLE_PROPERTIES_FUNC
#define COMPILE_AND_RUNTIME_INTROSPECTABLE_PROPERTIES_FUNC

#include "crisp.h"

namespace crisp {

/*
 * Iterate over a type list excluding functions. 
 */

template <typename V, typename C, int K, typename H, typename... Ts>
constexpr void iterateTypeList ([[maybe_unused]] C* target, V& visitor) {

    typedef Property<C, H, K> prop;

    if constexpr (!std::is_function<H>::value) {
        visitor.apply(prop::name, (*target).*(prop::value), prop::value);
    } 

    if constexpr (TypeList<Ts...>::size > 0) {
        iterateTypeList<V, C, K + 1, Ts...>(target, visitor);
    }
}

/*
 * Iterate over type list including functions.
 */

template <typename V, typename C, int K, typename H, typename... Ts>
constexpr void iterateTypeListWithFunctions ([[maybe_unused]] C* target, V& visitor) {

    typedef Property<C, H, K> prop;

    if constexpr (std::is_function<H>::value) {
        visitor.applyFunction(prop::name, prop::value);
    } else {
        visitor.apply(prop::name, (*target).*(prop::value), prop::value);
    }

    if constexpr (TypeList<Ts...>::size > 0) {
        iterateTypeListWithFunctions<V, C, K + 1, Ts...>(target, visitor);
    }
}

template<typename C>
int getPropertyCount () {
    return typename C::typesList{}.size;
}

template<typename C, typename T>
T& at (C& that, std::string name) {
    return *(T*)(C::props.at(name)(that));
}

template<typename C>
bool has (std::string name) {
    return C::props.find(name) != C::props.end();
}

template <typename C, typename V>
void apply (C& that, V& visitor) {

    apply(that, visitor, typename C::typesList{});
}

template <typename C, typename V>
void applyWithFunctions (C& that, V& visitor) {

    applyWithFunctions(that, visitor, typename C::typesList{});
}

template <typename C, typename V>
static void staticApply (V& visitor) {

    staticApply(visitor, typename C::typesList{});
}

template <typename C, typename V>
static void staticApplyWithFunctions (V& visitor) {

    staticApplyWithFunctions(visitor, typename C::typesList{});
}

template <typename C, typename V, typename... Ts>
void apply (C& that, V& visitor, TypeList<Ts...>) {

    iterateTypeList<V, typename C::SelfType, 1, Ts...>(that, visitor);

    if constexpr (!std::is_same<typename C::SelfType, typename C::SuperType>::value) {
        C::SuperType::apply(visitor, typename C::SuperType::typesList{});
    }
}

template <typename C, typename V, typename... Ts>
void applyWithFunctions (C& that, V& visitor, TypeList<Ts...>) {

    iterateTypeListWithFunctions<V, typename C::SelfType, 1, Ts...>(that, visitor);

    if constexpr (!std::is_same<typename C::SelfType, typename C::SuperType>::value) {
        C::SuperType::applyWithFunctions(visitor, typename C::SuperType::typesList{});
    }
}

template <typename C, typename V, typename... Ts>
static void staticApply (V& visitor, TypeList<Ts...>) {

    iterateTypeList<V, typename C::SelfType, 1, Ts...>(nullptr, visitor);

    if constexpr (!std::is_same<typename C::SelfType, typename C::SuperType>::value) {
        C::SuperType::staticApply(visitor, typename C::SuperType::typesList{});
    }
}

template <typename C, typename V, typename... Ts>
static void staticApplyWithFunctions (V& visitor, TypeList<Ts...>) {

    iterateTypeListWithFunctions<V, typename C::SelfType, 1, Ts...>(nullptr, visitor);

    if constexpr (!std::is_same<typename C::SelfType, typename C::SuperType>::value) {
        C::SuperType::staticApplyWithFunctions(visitor, typename C::SuperType::typesList{});
    }
}

} // namespace crisp

#endif
