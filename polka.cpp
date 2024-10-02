#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <ranges>
#include <cassert>
#include <array>

class Statement {  
public:  
    virtual std::vector<int> apply(std::vector<int> in) const = 0;  

    Statement() = default;  
    Statement(unsigned arguments, unsigned results, bool pure): arguments(arguments), results(results), pure(pure) {}  

    virtual ~Statement() = default;  

    bool is_pure() const {  
        return pure;  
    }  

    unsigned get_arguments_count() const {  
        return arguments;  
    }  

    unsigned get_results_count() const {  
        return results;  
    }  

protected:  
    unsigned arguments;  
    unsigned results;  
    bool pure;  
};

class Combine: public Statement {
public:
    Combine() = default;
    Combine(const Combine &c) = default;
    Combine(std::shared_ptr<Statement> l, std::shared_ptr<Statement> r) : Statement(), l(l), r(r) {
        unsigned l_args = l->get_arguments_count();
        unsigned l_results = l->get_results_count();
        unsigned r_args = r->get_arguments_count();
        unsigned r_results = r->get_results_count();
        arguments = std::max(l_args, r_args - l_results);
        results = l_results + r_results - std::min(l_results, r_args);
        pure = l->is_pure() && r->is_pure();
    }
    std::vector<int> apply(std::vector<int> v) const override {
        return l->apply(r->apply(v));
    }

private:
    std::shared_ptr<Statement> l, r;
};

class ConstOp: public Statement {
public:
    ConstOp(int v) : Statement(0, 1, true), v(v) {
    }

    std::vector<int> apply(std::vector<int> in) const override {
        in.push_back(v);
        return in;
    }

private:
    int v;
};


template<auto func>
class BinaryOP: public Statement {
public:
    BinaryOP() : Statement(2, 1, true) {}

    std::vector<int> apply(std::vector<int> v) const override {
        int b = v.back();
        v.pop_back();
        int a = v.back();
        v.pop_back();
        v.push_back(func(a, b));
        return v;
    }
};

class Abs : public Statement { 
public:
    Abs() : Statement(0, 1, true) {}

    std::vector<int> apply(std::vector<int> in) const override {
        int b = in.back();
        in.pop_back();
        in.push_back(std::abs(b));
        return in;
    }

};

class Input : public Statement {
public:
    Input() : Statement(0, 1, true) {}

    std::vector<int> apply(std::vector<int> in) const override {
        int value;
        std::cin >> value;
        in.push_back(value);
        return in;
    }
};

class Dup : public Statement {
public:
    Dup() : Statement(1, 2, true) {}

    std::vector<int> apply(std::vector<int> in) const override {
        int a = in.back();
        in.push_back(a);
        return in;
    }
};

std::shared_ptr<Statement> operator|(std::shared_ptr<Statement> lhs, std::shared_ptr<Statement> rhs)
{
    return std::make_shared<Combine>(lhs, rhs);
}


struct OperatorMap {
    std::string token;
    std::function<std::shared_ptr<Statement>()> create;
};

std::shared_ptr<Statement> compile(std::string_view str) {
    const std::array<OperatorMap, 8> operator_mappings = {{
        {"+", []() { return std::make_shared<BinaryOP<std::plus<>{}>>(); }},
        {"-", []() { return std::make_shared<BinaryOP<std::minus<>{}>>(); }},
        {"/", []() { return std::make_shared<BinaryOP<std::divides<>{}>>(); }},
        {"*", []() { return std::make_shared<BinaryOP<std::multiplies<>{}>>(); }},
        {"%", []() { return std::make_shared<BinaryOP<std::modulus<>{}>>(); }},
        {"abs", []() { return std::make_shared<Abs>(); }},
        {"input", []() { return std::make_shared<Input>(); }},
        {"dup", []() { return std::make_shared<Dup>(); }}
    }};

    auto consume_token = [&](std::string_view p) {
        if (str == p) {
            str = {};
            return true;
        } else if (str.starts_with(p) && str[p.size()] == ' ') {
            str.remove_prefix(p.size() + 1);
            return true;
        } else {
            return false;
        }
    };

    std::shared_ptr<Statement> ret;
    while (!str.empty()) {
        if (str[0] == ' ') {
            str.remove_prefix(std::ranges::find_if(str, [](char x) {return x != ' '; }) - str.begin());
        } else {
            bool exs = false;
            for (const auto& mapping: operator_mappings) {
                if (consume_token(mapping.token)) {
                    ret = !ret ? mapping.create() : std::move(ret) | mapping.create();
                    exs = true;
                    break;
                }
            }
            if (!exs) {
                int number = 0;
                int len = 0;
                for (auto i = str.begin(); i < str.end(); i++) {
                    if (std::isdigit(*i)) {
                        number = number * 10 + (*i - '0');
                        len++;
                    } else { break; }
                }
                str.remove_prefix(len);
                ret = !ret ? std::make_shared<ConstOp>(number) :
                    std::move(ret) | std::make_shared<ConstOp>(number);
            }
        }
    }
    return ret;
}

std::shared_ptr<Statement> optimize(std::shared_ptr<Statement> stmt);

int main()
{
    auto plus = compile("+");
    auto minus = compile("-");
    auto inc = compile("1 +");

    std::cout << inc->get_arguments_count() << std::endl;

    assert(plus->is_pure() && plus->get_arguments_count() == 2 && plus->get_results_count() == 1);
    assert(inc->is_pure() && inc->get_arguments_count() == 1 && inc->get_results_count() == 1);

    assert(plus->apply({2, 2}) == std::vector{4});
    assert(minus->apply({1, 2, 3}) == std::vector({1, -1}));

    auto plus_4 = inc | inc | inc | inc;

    assert(plus_4->is_pure() && plus_4->get_arguments_count() == 1 && plus_4->get_results_count() == 1);
    assert(plus_4->apply({0}) == std::vector{4});
    assert(inc->apply({0}) == std::vector{1});

    auto dup = compile("dup");
    assert(dup->is_pure() && dup->get_arguments_count() == 1 && dup->get_results_count() == 2);
    auto sqr = dup | compile("*");
    auto ten = compile("6") | plus_4;
    assert((ten | sqr)->apply({}) == std::vector{100});
    auto complicated_zero = compile(" 1    4  3 4   5  6 + -      - 3    / % -    ");
    assert(complicated_zero->is_pure() && complicated_zero->get_arguments_count() == 0 && complicated_zero->get_results_count() == 1);
    assert(complicated_zero->apply({}) == std::vector{0});

    for (int i = 0; i < 1000; ++i) {
        auto i_str = std::to_string(i);
        auto plus_i = "+" + i_str;
        auto minus_i = "-" + i_str;

        int res1 = compile(i_str)->apply({})[0];
        int res2 = compile(plus_i)->apply({})[0];
        int res3 = compile(minus_i)->apply({})[0];

        assert(res1 == i);
        assert(res2 == i);
        assert(res3 == -i);
    }

    auto nop = compile("");
    assert(nop->is_pure() && nop->get_arguments_count() == 0 && nop->get_results_count() == 0);
    return 0;
}