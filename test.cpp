#include <iostream>

#include "crisp.h"

struct Test {

    START_PROPS(Test)

    P(int, a, 0);
    P(double, b, 0);

    void test() {

    };

    F(test);

    END_PROPS
};

struct Blub { };

int main () {
    
    std::cout << crisp::has<Test>("a") << std::endl;

    std::cout << crisp::is_crisp<Test>() << std::endl;
}
