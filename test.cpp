#include <iostream>

#include "crisp_func.h"

struct Test {

    CRISP_START(Test)

    P(int, a, 0);
    P(double, b, 0);

    void test() {

    };

    F(test);

    CRISP_END
};

struct Blub { };

int main () {
    
    std::cout << crisp::has<Test>("a") << std::endl;
    std::cout << crisp::is_crisp<Test>() << std::endl;
}
