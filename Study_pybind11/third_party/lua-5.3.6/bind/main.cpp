
#include <iostream>
#include "include/detail/common.h"

// 辅助函数，用于打印索引序列
template <std::size_t... Is>
void print_index_sequence(BIND11::detail::index_sequence<Is...>){
    ((std::cout << (Is == 0? "" : ", ") << Is), ...);
    std::cout << std::endl;
}

void test_bool_indices(){
    // 根据布尔值序列生成索引序列
    using Result = BIND11::detail::select_indices<true, false, true, false, true>;
    print_index_sequence(Result{}); //0, 2, 4

    using Result2 = BIND11::detail::select_indices<false, true, true, false, true>;
    print_index_sequence(Result2{}); //, 1, 2, 4
}
void test_bool_constant(){
    using TrueType = BIND11::detail::bool_constant<true>;
    using FalseType = BIND11::detail::bool_constant<false>;

    std::cout << std::boolalpha;
    std::cout << "TrueType::value: " << TrueType::value << std::endl; //true
    std::cout << "FalseType::value: " << FalseType::value << std::endl;//false
}

void test_all_any(){
    using namespace BIND11::detail;
    using TrueType = bool_constant<true>;
    using FalseType = bool_constant<false>;

    // 测试 all_of
    std::cout << std::boolalpha;
    std::cout << "all_of<TrueType, TrueType>::value: " << all_of<TrueType, TrueType>::value << std::endl;
    std::cout << "all_of<TrueType, FalseType>::value: " << all_of<TrueType, FalseType>::value << std::endl;

    // 测试 any_of
    std::cout << "any_of<FalseType, FalseType>::value: " << any_of<FalseType, FalseType>::value << std::endl;
    std::cout << "any_of<FalseType, TrueType>::value: " << any_of<FalseType, TrueType>::value << std::endl;
}

static void test_satisfies_all_any_none();
static void test_first_last();
static void test_pack_element();
static void test_extractly_one();
static void test_is_template_base_of();
static void test_function_signatur();

int main(int argc, const char* argv[]){
    test_bool_indices();
    test_bool_constant();
    test_all_any();
    test_satisfies_all_any_none();
    test_first_last();
    test_pack_element();
    test_extractly_one();
    test_is_template_base_of();
    test_function_signatur();
    return 0;
}
//-----------------------

template <typename T>
struct IsIntegral : BIND11::detail::bool_constant<std::is_integral<T>::value> {};
template <typename T>
struct IsFloatingPoint : BIND11::detail::bool_constant<std::is_floating_point<T>::value> {};

template <typename T>
struct IsInt : std::is_same<T, int> {};

void test_satisfies_all_any_none(){
    using namespace BIND11::detail;
    std::cout << std::boolalpha;

    // 测试 satisfies_all_of
    std::cout << "satisfies_all_of<int, IsIntegral>::value: "
              << satisfies_all_of<int, IsIntegral>::value << std::endl;
    std::cout << "satisfies_all_of<int, IsIntegral, IsFloatingPoint>::value: "
              << satisfies_all_of<int, IsIntegral, IsFloatingPoint>::value << std::endl;

    // 测试 satisfies_any_of
    std::cout << "satisfies_any_of<int, IsIntegral, IsFloatingPoint>::value: "
              << satisfies_any_of<int, IsIntegral, IsFloatingPoint>::value << std::endl;
    std::cout << "satisfies_any_of<double, IsIntegral, IsFloatingPoint>::value: "
              << satisfies_any_of<double, IsIntegral, IsFloatingPoint>::value << std::endl;

    // 测试 satisfies_none_of
    std::cout << "satisfies_none_of<int, IsFloatingPoint>::value: "
              << satisfies_none_of<int, IsFloatingPoint>::value << std::endl;
    std::cout << "satisfies_none_of<int, IsIntegral, IsFloatingPoint>::value: "
              << satisfies_none_of<int, IsIntegral, IsFloatingPoint>::value << std::endl;
}
void test_first_last(){
    using namespace BIND11::detail::constexpr_impl;
    // 查找第一个值为 true 的元素的索引
    constexpr int index = first(0, false, false, true, false);
    std::cout << "The index of the first true value is: " << index << std::endl;
    //查找最后一个值为 true 的元素的索引
    constexpr int index2 = last(0, -1, false, true, false, true);
    std::cout << "The index of the last true value is: " << index2 << std::endl;
}
void test_pack_element(){
    using namespace BIND11::detail;
    // 从类型参数包中提取索引为 2 的类型
    using ResultType = pack_element<2, int, double, char, float>::type;

    // 验证提取的类型是否正确
    std::cout << std::boolalpha;
    std::cout << "Is the result type char? " << std::is_same<ResultType, char>::value << std::endl;
}
void test_extractly_one(){
    using namespace BIND11::detail;
    using Result = exactly_one_t<IsInt, double, float, int, char>;
    std::cout << std::boolalpha;
    std::cout << "Is the result type int? " << std::is_same<Result, int>::value
              << std::endl;
}

// 定义 MyBaseTemplate 模板类
template <typename... Ts>
struct MyBaseTemplate {};

// 定义 intrinsic_t 模板类，让它继承自 MyBaseTemplate
template <typename T>
struct intrinsic_t2 : public MyBaseTemplate<T> {};

// 定义 is_template_base_of 模板别名
template <template <typename...> class Base, typename T>
using is_template_base_of2 = decltype(BIND11::detail::is_template_base_of_impl<Base>
                                      ::check(static_cast<Base<T>*>(std::declval<intrinsic_t2<T>*>())));


void test_is_template_base_of(){
    using namespace BIND11::detail;
    std::cout << std::boolalpha;

    // 测试 MyBaseTemplate 是否是 intrinsic_t<int> 的模板基类
    std::cout << "Is MyBaseTemplate a template base of intrinsic_t<int>? "
              << is_template_base_of<MyBaseTemplate, int>::value << std::endl;
    // 测试 MyBaseTemplate 是否是 intrinsic_t2<int> 的模板基类
    std::cout << "Is MyBaseTemplate a template base of intrinsic_t<int>? "
              << is_template_base_of2<MyBaseTemplate, int>::value << std::endl;

}
struct ExampleFunctor {
    void operator()(int a, double b) {}
};
void exampleFunction0(int a, double b) {}

using ExampleFunctionPtr = void (*)(int, double);

void test_function_signatur(){
    using namespace BIND11::detail;
    std::cout << std::boolalpha;
    // 测试函数类型 //compile error
//    using FunctionSig = function_signature_t<decltype(exampleFunction0)>;
//    std::cout << "Is FunctionSig a function type? " <<
//        std::is_function<FunctionSig>::value << std::endl;

    // 测试函数指针类型
    using FunctionPtrSig = function_signature_t<ExampleFunctionPtr>;
    std::cout << "Is FunctionPtrSig a function type? " <<
        std::is_function<FunctionPtrSig>::value << std::endl;

    // 测试函数对象类型
    using FunctorSig = function_signature_t<ExampleFunctor>;
    std::cout << "Is FunctorSig a function type? " <<
        std::is_function<FunctorSig>::value << std::endl;

}
