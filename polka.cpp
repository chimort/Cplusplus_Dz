#include "statement.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <ranges>
#include <cassert>
#include <array>
#include <algorithm>
//#free_doorov

class Combine: public Statement {
public:
    Combine() = default;
    Combine(const Combine &c) = default;
    Combine(std::shared_ptr<Statement> l, std::shared_ptr<Statement> r) : Statement(), l(l), r(r) {
        int l_args = l->get_arguments_count();
        int l_results = l->get_results_count();
        int r_args = r->get_arguments_count();
        int r_results = r->get_results_count();
        arguments = std::max(l_args, r_args - l_results);
        results = l_results + r_results - std::min(l_results, r_args);
        pure = l->is_pure() && r->is_pure();
    }
    std::vector<int> apply(std::vector<int> v) const override {
        if (l) {
            v = l->apply(v);
        }
        if (r) {
            v = r->apply(v);
        }
        return v;
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
        if (v.size() < 2) {
            return v;
        }
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

class BlankStr : public Statement { 
public:
    BlankStr() : Statement(0, 0, true) {}

    std::vector<int> apply(std::vector<int> in) const override {
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
    if (str.empty()) {
        return std::make_shared<BlankStr>();
    }

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

    auto consume_space = [&]() {
        str.remove_prefix(std::ranges::find_if(str, [](char x) { return x != ' '; }) - str.begin());
    };

    auto parse_num = [&]() -> std::optional<int> {
        bool is_negative = false;
        if (!str.empty() && str[0] == '-') {
            if (str.size() > 1 && std::isdigit(str[1])) {
                is_negative = true;
                str.remove_prefix(1);
            }
        }

        int number = 0;
        int len = 0;
        for (auto i = str.begin(); i < str.end(); ++i) {
            if (std::isdigit(*i)) {
                number = number * 10 + (*i - '0');
                len++;
            } else {
                break;
            }
        }
        if (len > 0) {
            str.remove_prefix(len);
            return is_negative ? -number : number;
        }

        return std::nullopt;
    };

    std::shared_ptr<Statement> ret;
    while (!str.empty()) {
        consume_space();

        if (str.empty()) {
            break;
        }

        if (auto number = parse_num()) {
            ret = !ret ? std::make_shared<ConstOp>(*number) :
                std::move(ret) | std::make_shared<ConstOp>(*number);
            continue;
        }

        for (const auto& mapping : operator_mappings) {
            if (str.starts_with(mapping.token)) {
                str.remove_prefix(mapping.token.size());
                consume_space(); 
                ret = !ret ? mapping.create() : std::move(ret) | mapping.create();
                break;
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

    std::vector<int> stack = {1, 2, 3};
    auto test1 = compile("1 2 3 + -111 - * 10 %");
    auto sixs = test1 | test1 | compile("6");
    sixs = sixs | compile("dup");
    for (auto i: sixs->apply(stack)) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    assert(sixs->apply(stack) == std::vector<int>({1, 2, 3, 6, 6, 6, 6}));

    auto test2 = compile("-");
    assert(test2->apply(stack) == std::vector<int>({1, -1})); 

    return 0;
}