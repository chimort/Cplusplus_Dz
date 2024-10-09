#include "statement.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <ranges>
#include <unordered_map>
#include <regex>
#include <algorithm>

#include <cassert>
#include <chrono>
#include <thread>

class Combine: public Statement {
public:
    Combine() = default;
    Combine(const Combine &c) = default;
    Combine(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r)
        : Statement(calculateArgumentsCount(l, r), calculateResultsCount(l, r), calculateIsPure(l, r)), l(l), r(r) {}    
    
    std::vector<int> apply(std::vector<int> in) const override{ 
        return r->apply(l->apply(std::move(in))); 
    } 

private:
    std::shared_ptr<Statement> l, r;
    static unsigned calculateArgumentsCount(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r) {
        return l->get_arguments_count() + std::max(static_cast<int>(r->get_arguments_count()) - static_cast<int>(l->get_results_count()), 0);
    }

    static unsigned calculateResultsCount(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r) {
        return r->get_results_count() + std::max(static_cast<int>(l->get_results_count()) - static_cast<int>(r->get_arguments_count()), 0);
    }

    static bool calculateIsPure(const std::shared_ptr<Statement>& l, const std::shared_ptr<Statement>& r) {
        return l->is_pure() && r->is_pure();
    }

};

class ConstOp: public Statement {
public:
    ConstOp(int v) : Statement(0, 1, true), v(v) {
    }

    inline std::vector<int> apply(std::vector<int> in) const override {
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

    inline std::vector<int> apply(std::vector<int> in) const override {
        int b = in.back();
        in.pop_back();
        int a = in.back();
        in.pop_back();
        in.push_back(func(a, b));

        return std::move(in);
    }
};

class Abs : public Statement { 
public:
    Abs() : Statement(1, 1, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int b = in.back();
        in.pop_back();
        in.push_back(std::abs(b));
        return in;
    }

};

class Input : public Statement {
public:
    Input() : Statement(0, 1, false) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int value;
        std::cin >> value;
        in.push_back(value);
        return in;
    }
};

class Dup : public Statement {
public:
    Dup() : Statement(1, 2, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        int a = in.back();
        in.push_back(a);
        return in;
    }
};

class BlankStr : public Statement { 
public:
    BlankStr() : Statement(0, 0, true) {}

    inline std::vector<int> apply(std::vector<int> in) const override {
        return in;
    }
};

std::shared_ptr<Statement> operator|(std::shared_ptr<Statement> lhs, std::shared_ptr<Statement> rhs)
{
    return std::make_shared<Combine>(lhs, rhs);
}

const std::unordered_map<std::string, std::function<std::shared_ptr<Statement>()>> operator_mapping = {
    {"+", []() { return std::make_shared<BinaryOP<std::plus<>{}>>(); }},
    {"-", []() { return std::make_shared<BinaryOP<std::minus<>{}>>(); }},
    {"/", []() { return std::make_shared<BinaryOP<std::divides<>{}>>(); }},
    {"*", []() { return std::make_shared<BinaryOP<std::multiplies<>{}>>(); }},
    {"%", []() { return std::make_shared<BinaryOP<std::modulus<>{}>>(); }},
    {"abs", []() { return std::make_shared<Abs>(); }},
    {"input", []() { return std::make_shared<Input>(); }},
    {"dup", []() { return std::make_shared<Dup>(); }}
};

std::shared_ptr<Statement> compile(std::string_view str) {
    if (str.empty() || str.find_first_not_of(" ") == std::string::npos) {
        return std::make_shared<BlankStr>();
    }

    static const std::regex token_regex(R"(\S+)");
    static const std::regex number_regex(R"([-+]?\d+)");

    auto tokens_begin = std::regex_iterator<std::string_view::iterator>{str.begin(), str.end(), token_regex};
    auto tokens_end = std::regex_iterator<std::string_view::iterator>{};

    std::shared_ptr<Statement> ret;
    for (auto it = tokens_begin; it != tokens_end; ++it) {
        std::string token = it->str();

        if (std::regex_match(token, number_regex)) {
            auto const_op = std::make_shared<ConstOp>(std::stoi(token));
            ret = !ret ? std::move(const_op) : (ret | std::move(const_op));
        } else {
            auto op_it = operator_mapping.find(token);

            if (op_it != operator_mapping.end()) {
                auto op_stmt = op_it->second();
                ret = !ret ? std::move(op_stmt) : (ret | std::move(op_stmt));
            }
        }
    }
    return ret;
}

std::shared_ptr<Statement> optimize(std::shared_ptr<Statement> stmt);


void start_timer(std::chrono::high_resolution_clock::time_point& start_time) {
    start_time = std::chrono::high_resolution_clock::now();
}

void print_elapsed_time(const std::chrono::high_resolution_clock::time_point& start_time) {
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "Elapsed time: " << duration.count() << " seconds\n";
}


int main() {
    std::chrono::high_resolution_clock::time_point start_time;
    start_timer(start_time);
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

    for (int i = 0; i < 1000000; ++i) {
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

    std::shared_ptr<Statement> long_sequence_test = compile("1");
    for (int i = 0; i < 10000; ++i) {
        long_sequence_test = long_sequence_test | compile("1 +");
    }
    assert(long_sequence_test->apply({}) == std::vector<int>({10001}));
    std::cout << "Все тесты прошли!" << std::endl;

    print_elapsed_time(start_time);
}