#pragma once

#include "IObject.h"
#include "exception.h"

#include <cstddef>
#include <cstring>
#include <exception>
#include <forward_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)

/// 用于将先前未知的 C++ 实例转换为 对应脚本对象的方法。
enum class return_value_policy : uint8_t {
    /**这是默认的返回值策略，当返回值是指针时，它会退而采用return_value_policy::take_ownership（
     * 返回值策略：获取所有权）策略。否则，对于右值引用，它会使用return_value::move（返回值：移动）策略，
     * 对于左值引用， 则会使用return_value::copy（返回值：复制）策略 */
    automatic = 0,

    /** 当返回值是指针时，使用return_value_policy::reference（返回值策略：引用）策略。
     * 当从 C++ 代码中手动调用 Python 函数（即通过handle::operator()）时，
     * 这是函数参数的默认转换策略。你可能不需要使用该策略。 */
    automatic_reference,

    /** 引用一个存在的对象 */
    take_ownership,
    copy,
    /**  std::move */
    move,
    reference,

    //通过一种引用关系将父对象的生命周期与子对象的生命周期关联起来，
    //以确保在 Python 仍在使用子对象时，父对象不会被垃圾回收
    reference_internal
};

BIND11_NAMESPACE_BEGIN(detail)

inline static constexpr int log2(size_t n, int k = 0) {
    return (n <= 1) ? k : log2(n >> 1, k + 1);
}

// Returns the size as a multiple of sizeof(void *), rounded up.
// 字节对齐(void*)。x64: 0~8 -> 1;  9-16 -> 2
inline static constexpr size_t size_in_ptrs(size_t s) {
    return 1 + ((s - 1) >> log2(sizeof(void *)));
}

/**
 * The space to allocate for simple layout instance holders (see below) in multiple of the size of
 * a pointer (e.g.  2 means 16 bytes on 64-bit architectures).  The default is the minimum required
 * to holder either a std::unique_ptr or std::shared_ptr (which is almost always
 * sizeof(std::shared_ptr<T>)).
 */
constexpr size_t instance_simple_holder_in_ptrs() {
    static_assert(sizeof(std::shared_ptr<int>) >= sizeof(std::unique_ptr<int>),
                  "bind assumes std::shared_ptrs are at least as big as std::unique_ptrs");
    return size_in_ptrs(sizeof(std::shared_ptr<int>));
}

// Forward declarations
struct type_info;
struct value_and_holder;

struct nonsimple_values_and_holders {
    void **values_and_holders; //容纳一系列值指针和持有者而分配的所需空间
    uint8_t *status; //指向 [bb...] 块起始位置的指针（但并非独立分配）。
};

/// The 'instance' type which needs to be standard layout (need to be able to use 'offsetof')
struct instance {
    Object_HEAD
        /// Storage for pointers and holder; see simple_layout, below, for a description
        union {
        void *simple_value_holder[1 + instance_simple_holder_in_ptrs()];
        nonsimple_values_and_holders nonsimple;
    };
    /// Weak ref
    Object *weakrefs;
    /// If true, the pointer is owned which means we're free to manage it with a holder.
    bool owned : 1;
    /**
     * An instance has two possible value/holder layouts.
     *
     * Simple layout (when this flag is true), means the `simple_value_holder` is set with a
     * pointer and the holder object governing that pointer, i.e. [val1*][holder].  This layout is
     * applied whenever there is no python-side multiple inheritance of bound C++ types *and* the
     * type's holder will fit in the default space (which is large enough to hold either a
     * std::unique_ptr or std::shared_ptr).
     *
     * Non-simple layout applies when using custom holders that require more space than
     * `shared_ptr` (which is typically the size of two pointers), or when multiple inheritance is
     * used on the python side.  Non-simple layout allocates the required amount of memory to have
     * multiple bound C++ classes as parents.  Under this layout, `nonsimple.values_and_holders` is
     * set to a pointer to allocated space of the required space to hold a sequence of value
     * pointers and holders followed `status`, a set of bit flags (1 byte each), i.e.
     * [val1*][holder1][val2*][holder2]...[bb...]  where each [block] is rounded up to a multiple
     * of `sizeof(void *)`.  `nonsimple.status` is, for convenience, a pointer to the beginning of
     * the [bb...] block (but not independently allocated).
     *
     * Status bits indicate whether the associated holder is constructed (&
     * status_holder_constructed) and whether the value pointer is registered (&
     * status_instance_registered) in `registered_instances`.
     */
    bool simple_layout : 1;
    /// For simple layout, tracks whether the holder has been constructed
    bool simple_holder_constructed : 1;
    /// For simple layout, tracks whether the instance is registered in `registered_instances`
    bool simple_instance_registered : 1;
    /// If true, get_internals().patients has an entry for this object
    bool has_patients : 1;

    /// Initializes all of the above type/values/holders data (but not the instance values
    /// themselves)
    void allocate_layout();

    /// Destroys/deallocates all of the above
    void deallocate_layout();

    /// Returns the value_and_holder wrapper for the given type (or the first, if `find_type`
    /// omitted).  Returns a default-constructed (with `.inst = nullptr`) object on failure if
    /// `throw_if_missing` is false.
    value_and_holder get_value_and_holder(const type_info *find_type = nullptr,
                                          bool throw_if_missing = true);

    /// Bit values for the non-simple status flags
    static constexpr uint8_t status_holder_constructed = 1;
    static constexpr uint8_t status_instance_registered = 2;
};

static_assert(std::is_standard_layout<instance>::value,
              "Internal error: `BIND11::detail::instance` is not standard layout!");

/// from __cpp_future__ import (convenient aliases from C++14/17)
#if defined(BIND11_CPP14)
/*
// 定义一个模板函数，根据条件选择返回类型. 比如下面这个，整数就用int, 否则用double
template <typename T>
auto get_value(T value) -> std::conditional_t<std::is_integral_v<T>, int, double> {
    if constexpr (std::is_integral_v<T>) {
        return static_cast<int>(value);
    } else {
        return static_cast<double>(value);
    }
}
*/
using std::conditional_t; //编译时依据一个布尔条件来选择不同的类型
using std::enable_if_t;
using std::remove_cv_t; //编译时移除类型的 const 和 volatile 限定符
using std::remove_reference_t;
#else
template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;
template <bool B, typename T, typename F>
using conditional_t = typename std::conditional<B, T, F>::type;
template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;
template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;
#endif

#if defined(BIND11_CPP20)
using std::remove_cvref;
using std::remove_cvref_t;
#else
template <class T>
struct remove_cvref {
    using type = remove_cv_t<remove_reference_t<T>>;
};
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;
#endif

/// Index sequences
#if defined(BIND11_CPP14)
/*
template <typename T, std::size_t N, std::size_t... Is>
void print_array(const T (&arr)[N], std::index_sequence<Is...>) {
    ((std::cout << (Is == 0? "" : ", ") << arr[Is]), ...);
    std::cout << std::endl;
}

template <typename T, std::size_t N>
void print_array(const T (&arr)[N]) {
    print_array(arr, std::make_index_sequence<N>{});
}

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    print_array(arr);
    return 0;
}
*/
using std::index_sequence; //编译时表示和操作整数序列
using std::make_index_sequence; //生成序列
#else
template <size_t...>
struct index_sequence {};
template <size_t N, size_t... S>
struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, S...> {};
template <size_t... S>
struct make_index_sequence_impl<0, S...> {
    using type = index_sequence<S...>;
};
template <size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;
#endif

/// Make an index sequence of the indices of true arguments
/// ISeq: 表示当前已经生成的索引序列，类型为 std::index_sequence
/// size_t: 用于记录当前处理的索引
template <typename ISeq, size_t, bool...>
struct select_indices_impl {
    using type = ISeq;
};
/*
size_t... IPrev：展开的已经生成的索引序列中的索引。
size_t I：       当前处理的索引。
bool B：         当前的布尔值，用于决定是否将索引 I 添加到序列中。
bool... Bs：     剩余的布尔值序列。
使用 std::conditional_t 根据当前布尔值 B 来决定是否将索引 I 添加到已有的索引序列
index_sequence<IPrev...> 中。如果 B 为 true，则新的索引序列为 index_sequence<IPrev..., I>；
                            如果 B 为 false，则新的索引序列保持不变，即 index_sequence<IPrev...>。
*/
template <size_t... IPrev, size_t I, bool B, bool... Bs>
struct select_indices_impl<index_sequence<IPrev...>, I, B, Bs...>
    : select_indices_impl<conditional_t<B, index_sequence<IPrev..., I>, index_sequence<IPrev...>>,
                          I + 1,
                          Bs...> {};
/*
根据这些布尔值来决定是否将对应的索引添加到最终的索引序列中。
如果某个布尔值为 true，则将对应的索引添加到结果序列；  如果为 false，则跳过该索引。
*/
template <bool... Bs>
using select_indices = typename select_indices_impl<index_sequence<>, 0, Bs...>::type;

/// Backports of std::bool_constant and std::negation to accommodate older compilers
template <bool B>
using bool_constant = std::integral_constant<bool, B>;
template <typename T>
struct negation : bool_constant<!T::value> {};

// PGI/Intel cannot detect operator delete with the "compatible" void_t impl, so
// using the new one (C++14 defect, so generally works on newer compilers, even
// if not in C++17 mode)
#if defined(__PGIC__) || defined(__INTEL_COMPILER)
template <typename...>
using void_t = void;
#else
template <typename...>
struct void_t_impl {
    using type = void;
};
template <typename... Ts>
using void_t = typename void_t_impl<Ts...>::type;
#endif

/// Compile-time all/any/none of that check the boolean value of all template types
#if defined(__cpp_fold_expressions) && !(defined(_MSC_VER) && (_MSC_VER < 1916))
template <class... Ts>
using all_of = bool_constant<(Ts::value && ...)>;
template <class... Ts>
using any_of = bool_constant<(Ts::value || ...)>;
#elif !defined(_MSC_VER)
template <bool...>
struct bools {};
template <class... Ts>
using all_of = std::is_same<bools<Ts::value..., true>, bools<true, Ts::value...>>;
template <class... Ts>
using any_of = negation<all_of<negation<Ts>...>>;
#else
// MSVC has trouble with the above, but supports std::conjunction, which we can use instead (albeit
// at a slight loss of compilation efficiency).
template <class... Ts>
using all_of = std::conjunction<Ts...>;
template <class... Ts>
using any_of = std::disjunction<Ts...>;
#endif

template <class... Ts>
using none_of = negation<any_of<Ts...>>;

template <class T, template <class> class... Predicates>
using satisfies_all_of = all_of<Predicates<T>...>;
template <class T, template <class> class... Predicates>
using satisfies_any_of = any_of<Predicates<T>...>;
template <class T, template <class> class... Predicates>
using satisfies_none_of = none_of<Predicates<T>...>;

/// Strip the class from a method type
template <typename T>
struct remove_class {};
template <typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...)> {
    using type = R(A...);
};
template <typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...) const> {
    using type = R(A...);
};

/// Helper template to strip away type modifiers
/// 提取最基本的类型。不管原始类型是数组,引用，指针，const等等。
template <typename T>
struct intrinsic_type {
    using type = T;
};
template <typename T>
struct intrinsic_type<const T> {
    using type = typename intrinsic_type<T>::type;
};
template <typename T>
struct intrinsic_type<T *> {
    using type = typename intrinsic_type<T>::type;
};
template <typename T>
struct intrinsic_type<T &> {
    using type = typename intrinsic_type<T>::type;
};
template <typename T>
struct intrinsic_type<T &&> {
    using type = typename intrinsic_type<T>::type;
};
template <typename T, size_t N>
struct intrinsic_type<const T[N]> {
    using type = typename intrinsic_type<T>::type;
};
template <typename T, size_t N>
struct intrinsic_type<T[N]> {
    using type = typename intrinsic_type<T>::type;
};
template <typename T>
using intrinsic_t = typename intrinsic_type<T>::type;

/// Helper type to replace 'void' in some expressions
struct void_type {};

/// Helper template which holds a list of types
template <typename...>
struct type_list {};

/// Compile-time integer sum
#ifdef __cpp_fold_expressions
template <typename... Ts>
constexpr size_t constexpr_sum(Ts... ns) {
    return (0 + ... + size_t{ns});
}
#else
constexpr size_t constexpr_sum() { return 0; }
//编译期： 递归求和
template <typename T, typename... Ts>
constexpr size_t constexpr_sum(T n, Ts... ns) {
    return size_t{n} + constexpr_sum(ns...);
}
#endif

BIND11_NAMESPACE_BEGIN(constexpr_impl)
/// Implementation details for constexpr functions
constexpr int first(int i) { return i; }

template <typename T, typename... Ts>
constexpr int first(int i, T v, Ts... vs) {
    return v ? i : first(i + 1, vs...);
}

constexpr int last(int /*i*/, int result) { return result; }
template <typename T, typename... Ts>
constexpr int last(int i, int result, T v, Ts... vs) {
    return last(i + 1, v ? i : result, vs...);
}
BIND11_NAMESPACE_END(constexpr_impl)

/// Return the index of the first type in Ts which satisfies Predicate<T>.
/// Returns sizeof...(Ts) if none match.
/// 查找第一个值为 true 的元素的索引
template <template <typename> class Predicate, typename... Ts>
constexpr int constexpr_first() {
    return constexpr_impl::first(0, Predicate<Ts>::value...);
}

/// Return the index of the last type in Ts which satisfies Predicate<T>, or -1 if none match.
/// 查找最后一个值为 true 的元素的索引
template <template <typename> class Predicate, typename... Ts>
constexpr int constexpr_last() {
    return constexpr_impl::last(0, -1, Predicate<Ts>::value...);
}

/// Return the Nth element from the parameter pack
/// 递归， 提取一组类型中，指定索引的类型
template <size_t N, typename T, typename... Ts>
struct pack_element {
    using type = typename pack_element<N - 1, Ts...>::type;
};
template <typename T, typename... Ts>
struct pack_element<0, T, Ts...> {
    using type = T;
};

template <template <typename> class Predicate, typename Default, typename... Ts>
struct exactly_one {
    static constexpr auto found = constexpr_sum(Predicate<Ts>::value...);
    static_assert(found <= 1, "Found more than one type matching the predicate");

    static constexpr auto index = found ? constexpr_first<Predicate, Ts...>() : 0;
    using type = conditional_t<found, typename pack_element<index, Ts...>::type, Default>;
};
template <template <typename> class P, typename Default>
struct exactly_one<P, Default> {
    using type = Default;
};
/// 提取 指定判断条件和一组类型时，最多有1个类型满足的(超过则触发断言)，该类型.
template <template <typename> class Predicate, typename Default, typename... Ts>
using exactly_one_t = typename exactly_one<Predicate, Default, Ts...>::type;

/// Defer the evaluation of type T until types Us are instantiated
template <typename T, typename... /*Us*/>
struct deferred_type {
    using type = T;
};
template <typename T, typename... Us>
using deferred_t = typename deferred_type<T, Us...>::type;

/// 当 Base 是 Derived 的基类，并且 Base 和 Derived 不是同一类型时，整个表达式的值才为 true。
template <typename Base, typename Derived>
using is_strict_base_of
    = bool_constant<std::is_base_of<Base, Derived>::value && !std::is_same<Base, Derived>::value>;

//判断 Base 类型是否是 Derived 类型的可访问基类
template <typename Base, typename Derived>
using is_accessible_base_of
    = bool_constant<(std::is_same<Base, Derived>::value || std::is_base_of<Base, Derived>::value)
                    && std::is_convertible<Derived *, Base *>::value>;

//测试模型基类. eg: 测试 std::vector 是否是 std::vector<int> 的模板基类
template <template <typename...> class Base>
struct is_template_base_of_impl {
    template <typename... Us>
    static std::true_type check(Base<Us...> *);
    static std::false_type check(...);
};

/// Check if a template is the base of a type. For example:
/// `is_template_base_of<Base, T>` is true if `struct T : Base<U> {}` where U can be anything
//检查模板 Base 是不是 intrinsic_t<T>的基类
template <template <typename...> class Base, typename T>
// Sadly, all MSVC versions incl. 2022 need the workaround, even in C++20 mode.
// See also: https://github.com/pybind/BIND11/pull/3741
#if !defined(_MSC_VER)
using is_template_base_of
    = decltype(is_template_base_of_impl<Base>::check((intrinsic_t<T> *) nullptr));
#else
struct is_template_base_of
    : decltype(is_template_base_of_impl<Base>::check((intrinsic_t<T> *) nullptr)) {
};
#endif

/// Check if T is an instantiation of the template `Class`. For example:
/// `is_instantiation<shared_ptr, T>` is true if `T == shared_ptr<U>` where U can be anything.
template <template <typename...> class Class, typename T>
struct is_instantiation : std::false_type {};
/*eg: // 判断 std::vector<int> 是否是 std::vector 的实例化类型
std::cout  << is_instantiation<std::vector, std::vector<int>>::value << std::endl;
*/
template <template <typename...> class Class, typename... Us>
struct is_instantiation<Class, Class<Us...>> : std::true_type {};

/// Check if T is std::shared_ptr<U> where U can be anything
template <typename T>
using is_shared_ptr = is_instantiation<std::shared_ptr, T>;

/// Check if T looks like an input iterator
template <typename T, typename = void>
struct is_input_iterator : std::false_type {};

//如果 T 类型支持解引用和递增操作，那么这个表达式的结果就是 void。
template <typename T>
struct is_input_iterator<T,
                         void_t<decltype(*std::declval<T &>()), decltype(++std::declval<T &>())>>
    : std::true_type {};

template <typename T>
using is_function_pointer
    = bool_constant<std::is_pointer<T>::value
                    && std::is_function<typename std::remove_pointer<T>::type>::value>;

template <typename F>
struct strip_function_object {
    //提取仿函数的指针，并且去除class的前缀。比如A::fun() -> func()
    using type = typename remove_class<decltype(&F::operator())>::type;
};

// 提取函数签名(function, function pointer or lambda)
template <typename Function, typename F = remove_reference_t<Function>>
using function_signature_t = conditional_t<
    std::is_function<F>::value,
    F,
    typename conditional_t<std::is_pointer<F>::value || std::is_member_pointer<F>::value,
                           std::remove_pointer<F>,
                           strip_function_object<F>>::type>;

/*
如果该类型看起来像一个 lambda 表达式，则返回 true，即它既不是函数、指针，也不是成员指针。
请注意，此方法也可能会误判其他各类类型；该方法旨在用于传递 lambda 表达式合理的场景中。
*/
template <typename T>
using is_lambda = satisfies_none_of<remove_reference_t<T>,
                                    std::is_function,
                                    std::is_pointer,
                                    std::is_member_pointer>;

// [workaround(intel)] Internal error on fold expression
/// Apply a function over each element of a parameter pack
#if defined(__cpp_fold_expressions) && !defined(__INTEL_COMPILER)
// Intel compiler produces an internal error on this fold expression (tested with ICC 19.0.2)
#    define BIND11_EXPAND_SIDE_EFFECTS(PATTERN) (((PATTERN), void()), ...)
#else
using expand_side_effects = bool[];
#    define BIND11_EXPAND_SIDE_EFFECTS(PATTERN)                                                 \
        (void) BIND11::detail::expand_side_effects { ((PATTERN), void(), false)..., false }
#endif

BIND11_NAMESPACE_END(detail)

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4275)
//     warning C4275: An exported class was derived from a class that wasn't exported.
//     Can be ignored when derived from a STL class.
#endif

class BIND11_EXPORT_EXCEPTION builtin_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    /// Set the error using the Python C API
    virtual void set_error() const = 0;
};
#if defined(_MSC_VER)
#    pragma warning(pop)
#endif

#define BIND11_RUNTIME_EXCEPTION(name, type)                                                    \
    class BIND11_EXPORT_EXCEPTION name : public builtin_exception {                             \
    public:                                                                                       \
        using builtin_exception::builtin_exception;                                               \
        name() : name("") {}                                                                      \
        void set_error() const override { /*PyErr_SetString(type, what());*/ }                        \
    };
/*
BIND11_RUNTIME_EXCEPTION(stop_iteration, PyExc_StopIteration)
BIND11_RUNTIME_EXCEPTION(index_error, PyExc_IndexError)
BIND11_RUNTIME_EXCEPTION(key_error, PyExc_KeyError)
BIND11_RUNTIME_EXCEPTION(value_error, PyExc_ValueError)
BIND11_RUNTIME_EXCEPTION(type_error, PyExc_TypeError)
BIND11_RUNTIME_EXCEPTION(buffer_error, PyExc_BufferError)
BIND11_RUNTIME_EXCEPTION(import_error, PyExc_ImportError)
BIND11_RUNTIME_EXCEPTION(attribute_error, PyExc_AttributeError)
BIND11_RUNTIME_EXCEPTION(cast_error, PyExc_RuntimeError)
/// Thrown when BIND11::cast or
/// handle::call fail due to a type
/// casting error
BIND11_RUNTIME_EXCEPTION(reference_cast_error, PyExc_RuntimeError) /// Used internally
*/

[[noreturn]] BIND11_NOINLINE void BIND11_fail(const char *reason) {
    //assert(!PyErr_Occurred());
    throw std::runtime_error(reason);
}
[[noreturn]] BIND11_NOINLINE void BIND11_fail(const std::string &reason) {
    //assert(!PyErr_Occurred());
    throw std::runtime_error(reason);
}

template <typename T, typename SFINAE = void>
struct format_descriptor {};

BIND11_NAMESPACE_BEGIN(detail)
/*
如果 T 是 bool 类型，index 为 0。
如果 T 是整数类型(index).
    int8 -> 0
    uint8 -> 1
    int16 -> 2
    uint16 -> 3
    int32 -> 4
    uint32 -> 5
    int64 -> 6
    uint64 -> 7
如果 T 是浮点数类型：
    double -> 8 + 1。
    long double -> 8 + 2。
    float ->  8。
*/
template <typename T, typename SFINAE = void>
struct is_fmt_numeric {
    static constexpr bool value = false;
};
template <typename T>
struct is_fmt_numeric<T, enable_if_t<std::is_arithmetic<T>::value>> {
    static constexpr bool value = true;
    static constexpr int index
        = std::is_same<T, bool>::value
              ? 0
              : 1
                    + (std::is_integral<T>::value
                           ? detail::log2(sizeof(T)) * 2 + std::is_unsigned<T>::value
                           : 8
                                 + (std::is_same<T, double>::value        ? 1
                                                                   : std::is_same<T, long double>::value ? 2
                                                                                                         : 0));
};
BIND11_NAMESPACE_END(detail)

template <typename T>
struct format_descriptor<T, detail::enable_if_t<std::is_arithmetic<T>::value>> {
    static constexpr const char c = "?bBhHiIqQfdg"[detail::is_fmt_numeric<T>::index];
    static constexpr const char value[2] = {c, '\0'};
    static std::string format() { return std::string(1, c); }
};

#if !defined(BIND11_CPP17)

template <typename T>
constexpr const char
    format_descriptor<T, detail::enable_if_t<std::is_arithmetic<T>::value>>::value[2];

#endif

/// RAII wrapper that temporarily clears any Python error state
//struct error_scope {
//    PyObject *type, *value, *trace;
//    error_scope() { PyErr_Fetch(&type, &value, &trace); }
//    error_scope(const error_scope &) = delete;
//    error_scope &operator=(const error_scope &) = delete;
//    ~error_scope() { PyErr_Restore(type, value, trace); }
//};

/// Dummy destructor wrapper that can be used to expose classes with a private destructor
struct nodelete {
    template <typename T>
    void operator()(T *) {}
};

BIND11_NAMESPACE_BEGIN(detail)
template <typename... Args>
struct overload_cast_impl {
    template <typename Return>
    constexpr auto operator()(Return (*pf)(Args...)) const noexcept -> decltype(pf) {
        return pf;
    }

    template <typename Return, typename Class>
    constexpr auto operator()(Return (Class::*pmf)(Args...), std::false_type = {}) const noexcept
        -> decltype(pmf) {
        return pmf;
    }

    template <typename Return, typename Class>
    constexpr auto operator()(Return (Class::*pmf)(Args...) const, std::true_type) const noexcept
        -> decltype(pmf) {
        return pmf;
    }
};
BIND11_NAMESPACE_END(detail)

// overload_cast requires variable templates: C++14
#if defined(BIND11_CPP14)
#    define BIND11_OVERLOAD_CAST 1
/// Syntax sugar for resolving overloaded function pointers:
///  - regular: static_cast<Return (Class::*)(Arg0, Arg1, Arg2)>(&Class::func)
///  - sweet:   overload_cast<Arg0, Arg1, Arg2>(&Class::func)
template <typename... Args>
#    if (defined(_MSC_VER) && _MSC_VER < 1920) /* MSVC 2017 */                                    \
        || (defined(__clang__) && __clang_major__ == 5)
static constexpr detail::overload_cast_impl<Args...> overload_cast = {};
#    else
static constexpr detail::overload_cast_impl<Args...> overload_cast;
#    endif
#endif

/// Const member function selector for overload_cast
///  - regular: static_cast<Return (Class::*)(Arg) const>(&Class::func)
///  - sweet:   overload_cast<Arg>(&Class::func, const_)
static constexpr auto const_ = std::true_type{};

#if !defined(BIND11_CPP14) // no overload_cast: providing something that static_assert-fails:
template <typename... Args>
struct overload_cast {
    static_assert(detail::deferred_t<std::false_type, Args...>::value,
                  "BIND11::overload_cast<...> requires compiling in C++14 mode");
};
#endif // overload_cast

BIND11_NAMESPACE_BEGIN(detail)

// Adaptor for converting arbitrary container arguments into a vector; implicitly convertible from
// any standard container (or C-style array) supporting std::begin/std::end, any singleton
// arithmetic type (if T is arithmetic), or explicitly constructible from an iterator pair.
template <typename T>
class any_container {
    std::vector<T> v;

public:
    any_container() = default;

    // Can construct from a pair of iterators
    template <typename It, typename = enable_if_t<is_input_iterator<It>::value>>
    any_container(It first, It last) : v(first, last) {}

    // Implicit conversion constructor from any arbitrary container type
    // with values convertible to T
    template <typename Container,
             typename = enable_if_t<
                 std::is_convertible<decltype(*std::begin(std::declval<const Container &>())),
                                     T>::value>>
    // NOLINTNEXTLINE(google-explicit-constructor)
    any_container(const Container &c) : any_container(std::begin(c), std::end(c)) {}

    // initializer_list's aren't deducible, so don't get matched by the above template;
    // we need this to explicitly allow implicit conversion from one:
    template <typename TIn, typename = enable_if_t<std::is_convertible<TIn, T>::value>>
    any_container(const std::initializer_list<TIn> &c) : any_container(c.begin(), c.end()) {}

    // Avoid copying if given an rvalue vector of the correct type.
    // NOLINTNEXTLINE(google-explicit-constructor)
    any_container(std::vector<T> &&v) : v(std::move(v)) {}

    // Moves the vector out of an rvalue any_container
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::vector<T> &&() && { return std::move(v); }

    // Dereferencing obtains a reference to the underlying vector
    std::vector<T> &operator*() { return v; }
    const std::vector<T> &operator*() const { return v; }

    // -> lets you call methods on the underlying vector
    std::vector<T> *operator->() { return &v; }
    const std::vector<T> *operator->() const { return &v; }
};

// Forward-declaration; see detail/class.h
std::string get_fully_qualified_tp_name(Object *);

template <typename T>
inline static std::shared_ptr<T>
try_get_shared_from_this(std::enable_shared_from_this<T> *holder_value_ptr) {
// Pre C++17, this code path exploits undefined behavior, but is known to work on many platforms.
// Use at your own risk!
// See also https://en.cppreference.com/w/cpp/memory/enable_shared_from_this, and in particular
// the `std::shared_ptr<Good> gp1 = not_so_good.getptr();` and `try`-`catch` parts of the example.
#if defined(__cpp_lib_enable_shared_from_this) && (!defined(_MSC_VER) || _MSC_VER >= 1912)
    return holder_value_ptr->weak_from_this().lock();
#else
    try {
        return holder_value_ptr->shared_from_this();
    } catch (const std::bad_weak_ptr &) {
        return nullptr;
    }
#endif
}

// For silencing "unused" compiler warnings in special situations.
template <typename... Args>
#if defined(_MSC_VER) && _MSC_VER < 1920 // MSVC 2017
constexpr
#endif
    inline void
    silence_unused_warnings(Args &&...) {
}

// MSVC warning C4100: Unreferenced formal parameter
#if defined(_MSC_VER) && _MSC_VER <= 1916
#    define BIND11_WORKAROUND_INCORRECT_MSVC_C4100(...)                                         \
        detail::silence_unused_warnings(__VA_ARGS__)
#else
#    define BIND11_WORKAROUND_INCORRECT_MSVC_C4100(...)
#endif

// GCC -Wunused-but-set-parameter  All GCC versions (as of July 2021).
#if defined(__GNUG__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
#    define BIND11_WORKAROUND_INCORRECT_GCC_UNUSED_BUT_SET_PARAMETER(...)                       \
        detail::silence_unused_warnings(__VA_ARGS__)
#else
#    define BIND11_WORKAROUND_INCORRECT_GCC_UNUSED_BUT_SET_PARAMETER(...)
#endif

#if defined(_MSC_VER) // All versions (as of July 2021).

// warning C4127: Conditional expression is constant
constexpr inline bool silence_msvc_c4127(bool cond) { return cond; }

#    define BIND11_SILENCE_MSVC_C4127(...) ::BIND11::detail::silence_msvc_c4127(__VA_ARGS__)

#else
#    define BIND11_SILENCE_MSVC_C4127(...) __VA_ARGS__
#endif

// Pybind offers detailed error messages by default for all builts that are debug (through the
// negation of ndebug). This can also be manually enabled by users, for any builds, through
// defining BIND11_DETAILED_ERROR_MESSAGES.
#if !defined(BIND11_DETAILED_ERROR_MESSAGES) && !defined(NDEBUG)
#    define BIND11_DETAILED_ERROR_MESSAGES
#endif

BIND11_NAMESPACE_END(detail)
BIND11_NAMESPACE_END(BIND11_NAMESPACE)
