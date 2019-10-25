#include <z3++.h>
#include <Configuration.h>


bool doesDeMorganHold() {
    z3::context c;

    auto x = c.bool_const("x");
    auto y = c.bool_const("y");
    auto conjecture = (!(x && y)) == (!x || !y);

    z3::solver s(c);
    s.add(!conjecture);

    std::cout << "Conjecture: " << s << "\n";
    switch (s.check()) {
    case z3::unsat:
        std::cout << "de-Morgan is valid\n";
        return true;
    case z3::sat:
        std::cout << "de-Morgan is not valid\n";
        return false;
    case z3::unknown:
        std::cout << "unknown\n";
        return false;
    }
    __builtin_unreachable();
}

int main() {
    doesDeMorganHold();
}