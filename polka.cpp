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
    Combine(std::shared_ptr<Statement> l, std::shared_ptr<Statement> r)
        : Statement(l->get_arguments_count() + std::max(int(r->get_arguments_count()) - int(l->get_results_count()), 0),
                r->get_results_count() + std::max(int(l->get_results_count()) - int(r->get_arguments_count()), 0),
                l->is_pure() && r->is_pure()), l(l), r(r) {}    
    
    std::vector<int> apply(std::vector<int> in) const override{ 
        return r->apply(l->apply(in)); 
    } 
    inline std::shared_ptr<Statement> get_l() { return l; }
    inline std::shared_ptr<Statement> get_r() { return r; }

private:
    std::shared_ptr<Statement> l, r;
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

        return in;
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

std::shared_ptr<Statement> compile(std::string_view str) {
    if (str.empty() || str.find_first_not_of(" ") == std::string::npos) {
        return std::make_shared<BlankStr>();
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

    static const std::regex token_regex(R"(\S+)");
    static const std::regex number_regex(R"([-+]?\d+)");

    auto tokens_begin = std::regex_iterator<std::string_view::iterator>{str.begin(), str.end(), token_regex};
    auto tokens_end = std::regex_iterator<std::string_view::iterator>{};

    std::shared_ptr<Statement> ret;
    for (auto it = tokens_begin; it != tokens_end; ++it) {
        std::string token = it->str();

        if (std::regex_match(token, number_regex)) {
            auto const_op = std::make_shared<ConstOp>(std::stoi(token));
            ret = !ret ? const_op : (ret | const_op);
        } else {
            auto op_it = operator_mapping.find(token);

            if (op_it != operator_mapping.end()) {
                auto op_stmt = op_it->second();
                ret = !ret ? op_stmt : (ret | op_stmt);
            }
        }
    }

    return ret;

}


std::shared_ptr<Statement> optimize(std::shared_ptr<Statement> stmt) {
    if (auto combine_stmt = std::dynamic_pointer_cast<Combine>(stmt)) {
        auto left_optimized = optimize(combine_stmt->get_l());
        auto right_optimized = optimize(combine_stmt->get_r());
        
        if (auto left_const = std::dynamic_pointer_cast<ConstOp>(left_optimized)) {
            if (auto right_const = std::dynamic_pointer_cast<ConstOp>(right_optimized)) {
                auto combined_value = left_const->apply({})[0] + right_const->apply({})[0];
                return std::make_shared<ConstOp>(combined_value);
            }
        }

        return std::make_shared<Combine>(left_optimized, right_optimized);
    }
    return stmt;
}



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

    for (int i = 0; i < 100000; ++i) {
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

    auto const_5 = compile("5");
    assert(const_5->apply({}) == std::vector{5});

    // Тест сложения
    auto test_addition = compile("2 3 +");
    assert(test_addition->apply({}) == std::vector{5});

    // Тест вычитания
    auto test_subtraction = compile("5 3 -");
    assert(test_subtraction->apply({}) == std::vector{2});

    // Тест умножения
    auto test_multiplication = compile("3 4 *");
    assert(test_multiplication->apply({}) == std::vector{12});

    // Тест деления
    auto test_division = compile("8 4 /");
    assert(test_division->apply({}) == std::vector{2});

    // Тест остатка от деления
    auto test_modulus = compile("10 3 %");
    assert(test_modulus->apply({}) == std::vector{1});

    // Тест для abs
    auto test_abs = compile("-5 abs");
    assert(test_abs->apply({}) == std::vector{5});

    auto test_dup = compile("7 dup");
    assert(test_dup->apply({}) == std::vector<int>({7, 7}));

    // Тест сложной комбинации операций

    // Тест последовательных операций
    auto sequential_test = compile("5 1 + 2 * 3 -");
    assert(sequential_test->apply({}) == std::vector<int>({9}));

    // Тест с несколькими abs
    auto test_multiple_abs = compile("-3 abs 4 abs");
    assert(test_multiple_abs->apply({}) == std::vector<int>({3, 4}));

    auto test_blank = compile("  ");
    assert(test_blank->is_pure() && test_blank->get_arguments_count() == 0 && test_blank->get_results_count() == 0);


    // Тест сложной цепочки
    auto chain_test = compile("20 2 3 + 4 * dup 5 -");
    assert(chain_test->apply({}) == std::vector<int>({20, 20, 15}));

    auto test_spaces_and_sign = compile("    +                ");
    assert(test_spaces_and_sign->is_pure() && test_spaces_and_sign->get_arguments_count() == 2 && test_spaces_and_sign->get_results_count() == 1);

    auto test_negative_modulus = compile("-10 3 %");
    assert(test_negative_modulus->apply({}) == std::vector{-1});

    auto test_large_numbers = compile("1000000000 1000000000 +");
    assert(test_large_numbers->apply({}) == std::vector{2000000000});

    auto complex_expression_test = compile("3 5 8 * 7 + 2 - 4 /");
    assert(complex_expression_test->apply({}) == std::vector<int>({3, 11}));

    auto nested_operations_test = compile("2 3 + 5 * 6 2 / -");
    assert(nested_operations_test->apply({}) == std::vector<int>({22}));

    auto multiple_dup_abs_test = compile("4 dup dup abs -3 abs +");
    assert(multiple_dup_abs_test->apply({}) == std::vector<int>({4, 4, 7}));

    auto negative_modulus_test = compile("-10 -3 %");
    assert(negative_modulus_test->apply({}) == std::vector{-1});

    auto overflow_test = compile("2147483647 1 +");
    assert(overflow_test->apply({}) == std::vector<int>{-2147483648});

    std::shared_ptr<Statement> long_sequence_test = compile("1");
    for (int i = 0; i < 1000; ++i) {
        long_sequence_test = long_sequence_test | compile("1 +");
    }
    assert(long_sequence_test->apply({}) == std::vector<int>({1001}));

    std::cout << "Все тесты прошли!" << std::endl;

    print_elapsed_time(start_time);
}