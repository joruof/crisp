#ifndef COMPILE_AND_RUNTIME_INTROSPECTABLE_PROPERTIES_VISITORS
#define COMPILE_AND_RUNTIME_INTROSPECTABLE_PROPERTIES_VISITORS

#include <sstream>
#include <vector>

#include "helpers/Table.h"
#include "helpers/Crisp.h"

/*
 * Helper macros for easier visitor definition.
 */

#define is_props_object(T) typename std::enable_if< \
            is_crisp<T>::value>::type* = nullptr

#define is_common_object(T) typename std::enable_if<! \
            is_crisp<T>::value>::type* = nullptr

/**
 * This checks whether an object can be printed.
 *
 * Source: https://stackoverflow.com/questions/5768511/using-sfinae-to-check-for-global-operator
 */
namespace has_insertion_operator_impl {
    typedef char no;
    typedef char yes[2];

    struct any_t {
        template<typename T> any_t( T const& );
    };

    no operator<<( std::ostream const&, any_t const& );

    yes& test( std::ostream& );
    no test( no );

    template<typename T>
    struct has_insertion_operator {
        static std::ostream &s;
        static T const &t;
        static bool const value = sizeof( test(s << t) ) == sizeof( yes );
    };
}

template<typename T>
struct has_insertion_operator :
    has_insertion_operator_impl::has_insertion_operator<T> {
};

#define is_printable(T) typename std::enable_if< \
            has_insertion_operator<T>::value>::type* = nullptr

#define is_not_printable(T) typename std::enable_if<! \
            has_insertion_operator<T>::value>::type* = nullptr

/**
 * This can be used to check whether a == operator is defined.
 */
namespace has_equals_operator {

    struct NoEqualExists {}; 
    template<typename T, typename Arg> 
    NoEqualExists operator== (const T&, const Arg&);
       
    template<typename T, typename Arg = T>
    struct EqualExists {
        enum { value = !std::is_same<
            decltype(*(T*)(0) == *(Arg*)(0)),
            NoEqualExists>::value };
    };  
}

#define has_equals(T) typename std::enable_if< \
        has_equals_operator::EqualExists<T>::value>::type* = nullptr

#define has_no_equals(T) typename std::enable_if< \
        !has_equals_operator::EqualExists<T>::value>::type* = nullptr


/**
 * An instance of this class can be used to convert a property object
 * to a std::string. The properties will be written as
 * "<property-name> = <property-value>\n". If a member of the given
 * property object is derived from the "Properties" class the
 * Property StringWriter will be applied recursively to this member.
 */
class PropertyStringWriter {

    std::ostringstream str;
    int indentLevel;

    int maxPropertyIndex = 0;
    int currentPropertyIndex = 0;

public:

    template<typename C, is_props_object(C)>
    PropertyStringWriter (C& c, std::string name = "", int il = 0) 
        : indentLevel{il} {

        str << std::string(indentLevel * 2, ' ');

        if (name.empty() && il == 0) {
            name = c.selfName;
        }

        if (!name.empty()) {
            str << name << " = ";
        }

        str << std::endl;

        maxPropertyIndex = c.getPropertyCount();

        indentLevel += 1;
        c.apply(*this);
        indentLevel -= 1;
    }

    template <typename C, is_common_object(C), is_printable(C)>
    PropertyStringWriter(C& c, std::string name = "", int il = 0) 
        : indentLevel{il} {

        str << std::string(indentLevel * 2, ' ');

        if (!name.empty()) {
            str << name << " = ";
        }

        str << c;
    }

    template <typename C, is_common_object(C), is_not_printable(C)>
    PropertyStringWriter(C&, std::string name = "", int il = 0)
        : indentLevel{il} {

        str << std::string(indentLevel * 2, ' ') << name;
    }

    template <typename C>
    PropertyStringWriter(std::vector<C>& c, std::string name = "", int il = 0)
        : indentLevel{il} {

        str << std::string(indentLevel * 2, ' ');

        if (!name.empty()) {
            str << name << " = ";
        }

        str << "[";

        for (C& i : c) {
            str << PropertyStringWriter(i).get();
            if (&c.back() != &i) {
                str << ", ";
            }
        }

        str << "]";
    }

    template <typename T, typename C>
    void apply (const std::string& name, T& value, T C::*) {

        currentPropertyIndex += 1;

        str << PropertyStringWriter(
                value, name, indentLevel).get();

        if (currentPropertyIndex < maxPropertyIndex) {
            str << std::endl;
        }
    }

    std::string get() {

        return str.str();
    };
};

/**
 * This can be used to check if any property in an
 * object has changed.
 */

template<typename C, is_props_object(C)>
class PropertyCompare {

    C* before = nullptr;

    bool hasChanged = false;

public:

    PropertyCompare() {};

    PropertyCompare (C& c) : before{new C(c)} {
    };

   ~PropertyCompare () {

        if (nullptr != before) {
            delete before;
        }
    }

    bool changed (C& previous, C& now) {

        before = &previous;

        bool result = changed(now);

        before = nullptr;

        return result;
    }

    bool changed (C& now) {

        if (before == nullptr) {
            throw std::runtime_error("No previous version to compare to!");
        }

        hasChanged = false;
        now.apply(*this);

        return hasChanged;
    }

    template <typename O, typename T, is_props_object(T)>
    void apply (const std::string&, T& value, T O::* ref) {

        hasChanged |= PropertyCompare<T>(before->*ref).changed(value);
    }

    template <typename O, typename T, is_common_object(T), has_equals(T)>
    void apply (const std::string&, T& value, T O::* ref) {

        hasChanged |= before->*ref != value;
    }

    template <typename O, typename T, is_common_object(T), has_no_equals(T)>
    void apply (const std::string&, T&, T O::*) {

        // if there is no way to test for equality, we are done
    }
};

/**
 * An instance of this class can be used to convert a property object
 * to a JSON file.  If a member of the given property object is
 * derived from the "Properties" class the PropertyJsonWriter
 * will be applied recursively to this member.
 */
class PropertyJsonWriter {

    std::ostringstream json;
    int indentLevel = 1;

public:

    template <typename T, typename C, is_props_object(T)>
    void apply (const std::string& name, T& value, T C::*) {
        json << std::string(indentLevel * 2, ' ')
            << name
            << ": {"
            << std::endl;

        indentLevel += 1;
        value.apply(*this);
        indentLevel -= 1;

        json << std::string(indentLevel * 2, ' ')
            << "},"
            << std::endl;
    }

    template <typename T, typename C, is_common_object(T)>
    void apply (const std::string& name, T& value, T C::*) {
        json << std::string(indentLevel * 2, ' ')
            << name
            << ": " << value << ","
            << std::endl;
    }

    void apply(const std::string& name, std::string value) {

        json << std::string(indentLevel * 2, ' ')
            << name
            << ": \"" << value << "\","
            << std::endl;
    }

    std::string get() {

        std::string out = "{";
        out += "\n";
        out += json.str();
        out += "}";

        return out;
    };
};

#endif
