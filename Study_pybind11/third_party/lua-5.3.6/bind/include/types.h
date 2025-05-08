/*
    BIND11/pytypes.h: Convenience wrapper classes for basic Python types

    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "detail/common.h"
#include "buffer_info.h"

#include <type_traits>
#include <utility>

#if defined(BIND11_HAS_OPTIONAL)
#    include <optional>
#endif

#ifdef BIND11_HAS_STRING_VIEW
#    include <string_view>
#endif

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)

/* A few forward declarations */
class handle;
class object;
class str;
class iterator;
class type;
struct arg;
struct arg_v;

BIND11_NAMESPACE_BEGIN(detail)
class args_proxy;
bool isinstance_generic(handle obj, const std::type_info &tp);

// Accessor forward declarations
template <typename Policy>
class accessor;
namespace accessor_policies {
struct obj_attr;
struct str_attr;
struct generic_item;
struct sequence_item;
struct list_item;
struct tuple_item;
} // namespace accessor_policies
using obj_attr_accessor = accessor<accessor_policies::obj_attr>;
using str_attr_accessor = accessor<accessor_policies::str_attr>;
using item_accessor = accessor<accessor_policies::generic_item>;
using sequence_accessor = accessor<accessor_policies::sequence_item>;
using list_accessor = accessor<accessor_policies::list_item>;
using tuple_accessor = accessor<accessor_policies::tuple_item>;

/// Tag and check to identify a class which implements the Python object API
class Object_tag {};
template <typename T>
using is_Object = std::is_base_of<Object_tag, remove_reference_t<T>>;

/** \rst
    A mixin class which adds common functions to `handle`, `object` and various accessors.
    The only requirement for `Derived` is to implement ``Object *Derived::ptr() const``.
\endrst */
template <typename Derived>
class object_api : public Object_tag {
    const Derived &derived() const { return static_cast<const Derived &>(*this); }

public:

    iterator begin() const;
    iterator end() const;

    item_accessor operator[](handle key) const;
    item_accessor operator[](const char *key) const;

    obj_attr_accessor attr(handle key) const;
    str_attr_accessor attr(const char *key) const;

    args_proxy operator*() const;

    template <typename T>
    bool contains(T &&item) const;

    /** \rst
        Assuming the Python object is a function or implements the ``__call__``
        protocol, ``operator()`` invokes the underlying function, passing an
        arbitrary set of parameters. The result is returned as a `object` and
        may need to be converted back into a Python object using `handle::cast()`.

        When some of the arguments cannot be converted to Python objects, the
        function will throw a `cast_error` exception. When the Python function
        call fails, a `error_already_set` exception is thrown.
    \endrst */
    template <return_value_policy policy = return_value_policy::automatic_reference,
              typename... Args>
    object operator()(Args &&...args) const;
    template <return_value_policy policy = return_value_policy::automatic_reference,
              typename... Args>
    BIND11_DEPRECATED("call(...) was deprecated in favor of operator()(...)")
    object call(Args &&...args) const;

    /// Equivalent to ``obj is other`` in Python.
    bool is(object_api const &other) const {
        return derived().ptr() == other.derived().ptr();
    }
    /// Equivalent to ``obj is None`` in Python.
    bool is_none() const {
        //return derived().ptr() == Py_None;
    }
    /// Equivalent to obj == other in Python
    bool equal(object_api const &other) const {
        return rich_compare(other, CompareType::kEQ);
    }
    bool not_equal(object_api const &other) const {
        return rich_compare(other, CompareType::kNE); }
    bool operator<(object_api const &other) const {
        return rich_compare(other, CompareType::kLT); }
    bool operator<=(object_api const &other) const {
        return rich_compare(other, CompareType::kLE); }
    bool operator>(object_api const &other) const {
        return rich_compare(other, CompareType::kGT); }
    bool operator>=(object_api const &other) const {
        return rich_compare(other, CompareType::kGE); }

    object operator-() const;
    object operator~() const;
    object operator+(object_api const &other) const;
    object operator+=(object_api const &other) const;
    object operator-(object_api const &other) const;
    object operator-=(object_api const &other) const;
    object operator*(object_api const &other) const;
    object operator*=(object_api const &other) const;
    object operator/(object_api const &other) const;
    object operator/=(object_api const &other) const;
    object operator|(object_api const &other) const;
    object operator|=(object_api const &other) const;
    object operator&(object_api const &other) const;
    object operator&=(object_api const &other) const;
    object operator^(object_api const &other) const;
    object operator^=(object_api const &other) const;
    object operator<<(object_api const &other) const;
    object operator<<=(object_api const &other) const;
    object operator>>(object_api const &other) const;
    object operator>>=(object_api const &other) const;

    BIND11_DEPRECATED("Use py::str(obj) instead")
    BIND11::str str() const;

    str_attr_accessor doc() const;

    int ref_count() const { return static_cast<int>(Py_REFCNT(derived().ptr())); }

    handle get_type() const;

private:
    bool rich_compare(object_api const &other, int value) const;
};

BIND11_NAMESPACE_END(detail)

//no reference counting
class handle : public detail::object_api<handle> {
public:
    handle() = default;

    handle(Object *ptr) : m_ptr(ptr) {}

    /// Return the underlying ``Object *`` pointer
    Object *ptr() const { return m_ptr; }
    Object *&ptr() { return m_ptr; }

    //often used by 'object' class. and auto call inc_ref with dec_ref
    const handle &inc_ref() const & {
        //TODO Py_XINCREF(m_ptr);
        m_ptr->incRef();
        return *this;
    }
    const handle &dec_ref() const & {
        //TODO Py_XDECREF(m_ptr);
        m_ptr->decRef();
        return *this;
    }

    //cast may throw cast_error
    template <typename T>
    T cast() const;

    explicit operator bool() const { return m_ptr != nullptr; }
    //like: obj1 is obj2
    BIND11_DEPRECATED("Use obj1.is(obj2) instead")
    bool operator==(const handle &h) const { return m_ptr == h.m_ptr; }
    BIND11_DEPRECATED("Use !obj1.is(obj2) instead")
    bool operator!=(const handle &h) const { return m_ptr != h.m_ptr; }
    BIND11_DEPRECATED("Use handle::operator bool() instead")
    bool check() const { return m_ptr != nullptr; }

protected:
    Object *m_ptr = nullptr;
};

//with reference counting
class object : public handle {
public:
    object() = default;
    //BIND11_DEPRECATED("Use reinterpret_borrow<object>() or reinterpret_steal<object>()")
    object(handle h, bool is_borrowed) : handle(h) {
        if (is_borrowed) {
            inc_ref();
        }
    }
    object(const object &o) : handle(o) { inc_ref(); }
    /// Move constructor;
    object(object &&other) noexcept : handle(other) { other.m_ptr = nullptr; }
    ~object() { dec_ref(); }

    //without dec-inf
    handle release() {
        Object *tmp = m_ptr;
        m_ptr = nullptr;
        return handle(tmp);
    }

    object &operator=(const object &other) {
        other.inc_ref();
        // Use temporary variable to ensure `*this` remains valid while
        // `Py_XDECREF` executes, in case `*this` is accessible from Python.
        handle temp(m_ptr);
        m_ptr = other.m_ptr;
        temp.dec_ref();
        return *this;
    }

    object &operator=(object &&other) noexcept {
        if (this != &other) {
            handle temp(m_ptr);
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
            temp.dec_ref();
        }
        return *this;
    }

    // Calling cast() on an object lvalue just copies (via handle::cast)
    template <typename T>
    T cast() const &;
    // Calling on an object rvalue does a move, if needed and/or possible
    template <typename T>
    T cast() &&;

protected:
    // Tags for choosing constructors from raw Object *
    struct borrowed_t {};
    struct stolen_t {};

    /// @cond BROKEN
    template <typename T>
    friend T reinterpret_borrow(handle);
    template <typename T>
    friend T reinterpret_steal(handle);
    /// @endcond

public:
    // Only accessible from derived classes and the reinterpret_* functions
    object(handle h, borrowed_t) : handle(h) { inc_ref(); }
    object(handle h, stolen_t) : handle(h) {}
};

template <typename T>
T reinterpret_borrow(handle h) {
    return {h, object::borrowed_t{}};
}

template <typename T>
T reinterpret_steal(handle h) {
    return {h, object::stolen_t{}};
}

BIND11_NAMESPACE_BEGIN(detail)
std::string error_string();
BIND11_NAMESPACE_END(detail)

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4275 4251)
//     warning C4275: An exported class was derived from a class that wasn't exported.
//     Can be ignored when derived from a STL class.
#endif
/// Fetch and hold an error which was already set in Python.  An instance of this is typically
/// thrown to propagate python-side errors back through C++ which can either be caught manually or
/// else falls back to the function dispatcher (which then raises the captured error back to
/// python).
class BIND11_EXPORT_EXCEPTION error_already_set : public std::runtime_error {
public:
    /// Constructs a new exception from the current Python error indicator, if any.  The current
    /// Python error indicator will be cleared.
    error_already_set() : std::runtime_error(detail::error_string()) {
        //PyErr_Fetch(&m_type.ptr(), &m_value.ptr(), &m_trace.ptr());
    }

    error_already_set(const error_already_set &) = default;
    error_already_set(error_already_set &&) = default;

    inline ~error_already_set() override;

    /// Give the currently-held error back to Python, if any.  If there is currently a Python error
    /// already set it is cleared first.  After this call, the current object no longer stores the
    /// error variables (but the `.what()` string is still available).
    void restore() {
        //PyErr_Restore(m_type.release().ptr(), m_value.release().ptr(), m_trace.release().ptr());
    }

    /// If it is impossible to raise the currently-held error, such as in a destructor, we can
    /// write it out using Python's unraisable hook (`sys.unraisablehook`). The error context
    /// should be some object whose `repr()` helps identify the location of the error. Python
    /// already knows the type and value of the error, so there is no need to repeat that. After
    /// this call, the current object no longer stores the error variables, and neither does
    /// Python.
    void discard_as_unraisable(object err_context) {
        restore();
        //PyErr_WriteUnraisable(err_context.ptr());
    }
    /// An alternate version of `discard_as_unraisable()`, where a string provides information on
    /// the location of the error. For example, `__func__` could be helpful.
    void discard_as_unraisable(const char *err_context) {
        //discard_as_unraisable(reinterpret_steal<object>(BIND11_FROM_STRING(err_context)));
    }

    // Does nothing; provided for backwards compatibility.
    BIND11_DEPRECATED("Use of error_already_set.clear() is deprecated")
    void clear() {}

    /// Check if the currently trapped error type matches the given Python exception class (or a
    /// subclass thereof).  May also be passed a tuple to search for any exception class matches in
    /// the given tuple.
    bool matches(handle exc) const {
        //TODO return (PyErr_GivenExceptionMatches(m_type.ptr(), exc.ptr()) != 0);
        return false;
    }

    const object &type() const { return m_type; }
    const object &value() const { return m_value; }
    const object &trace() const { return m_trace; }

private:
    object m_type, m_value, m_trace;
};
#if defined(_MSC_VER)
#    pragma warning(pop)
#endif

inline void raise_from(Object *type, const char *message) {
    //more see pybind11
}

/// Sets the current Python error indicator with the chosen error, performing a 'raise from'
/// from the error contained in error_already_set to indicate that the chosen error was
/// caused by the original error. After this function is called error_already_set will
/// no longer contain an error.
inline void raise_from(error_already_set &err, Object *type, const char *message) {
    err.restore();
    raise_from(type, message);
}

/** \defgroup python_builtins const_name
    Unless stated otherwise, the following C++ functions behave the same
    as their Python counterparts.
 */

/** \ingroup python_builtins
    \rst
    Return true if ``obj`` is an instance of ``T``. Type ``T`` must be a subclass of
    `object` or a class which was exposed to Python as ``py::class_<T>``.
\endrst */
template <typename T, detail::enable_if_t<std::is_base_of<object, T>::value, int> = 0>
bool isinstance(handle obj) {
    return T::check_(obj);
}

template <typename T, detail::enable_if_t<!std::is_base_of<object, T>::value, int> = 0>
bool isinstance(handle obj) {
    return detail::isinstance_generic(obj, typeid(T));
}

template <>
inline bool isinstance<handle>(handle) = delete;
template <>
inline bool isinstance<object>(handle obj) {
    return obj.ptr() != nullptr;
}

/// \ingroup python_builtins
/// Return true if ``obj`` is an instance of the ``type``.
inline bool isinstance(handle obj, handle type) {
    //const auto result = Object_IsInstance(obj.ptr(), type.ptr());
//    if (result == -1) {
//        throw error_already_set();
//    }
//    return result != 0;
    return IObject::isInstance(obj.ptr(), type.ptr());
}

/// \addtogroup python_builtins
/// @{
inline bool hasattr(handle obj, handle name) {
    return IObject::hasAttr(obj.ptr(), name.ptr());
}

inline bool hasattr(handle obj, const char *name) {
    //return Object_HasAttrString(obj.ptr(), name) == 1;
    return IObject::hasAttrString(obj.ptr(), name);
}

inline void delattr(handle obj, handle name) {
//    if (Object_DelAttr(obj.ptr(), name.ptr()) != 0) {
//        throw error_already_set();
//    }
    IObject::delAttr(obj.ptr(), name.ptr());
}

inline void delattr(handle obj, const char *name) {
//    if (Object_DelAttrString(obj.ptr(), name) != 0) {
//        throw error_already_set();
//    }
    IObject::delAttrString(obj.ptr(), name);
}

inline object getattr(handle obj, handle name) {
//    Object *result = Object_GetAttr(obj.ptr(), name.ptr());
    auto result = IObject::getAttr(obj.ptr(), name.ptr());
    if(!result){
         throw error_already_set();
    }
    return reinterpret_steal<object>(result);
}

inline object getattr(handle obj, const char *name) {
    //Object *result = Object_GetAttrString(obj.ptr(), name);
    Object *result = IObject::getAttrString(obj.ptr(), name);
    if (!result) {
        throw error_already_set();
    }
    return reinterpret_steal<object>(result);
}

inline object getattr(handle obj, handle name, handle default_) {
    if (Object *result = IObject::getAttr(obj.ptr(), name.ptr())) {
        return reinterpret_steal<object>(result);
    }
    //PyErr_Clear();
    return reinterpret_borrow<object>(default_);
}

inline object getattr(handle obj, const char *name, handle default_) {
    if (Object *result = IObject::getAttrString(obj.ptr(), name)) {
        return reinterpret_steal<object>(result);
    }
    //PyErr_Clear();
    return reinterpret_borrow<object>(default_);
}

inline void setattr(handle obj, handle name, handle value) {
//    if (Object_SetAttr(obj.ptr(), name.ptr(), value.ptr()) != 0) {
//        throw error_already_set();
//    }
    if(!IObject::setAttr(obj.ptr(), name.ptr(), value.ptr())){
        throw error_already_set();
    }
}

inline void setattr(handle obj, const char *name, handle value) {
    if(!IObject::setAttrString(obj.ptr(), name, value.ptr())){
        throw error_already_set();
    }
}

inline ssize_t hash(handle obj) {
    auto h = IObject::hash(obj.ptr());
    if (h == -1) {
        throw error_already_set();
    }
    return h;
}

/// @} python_builtins

BIND11_NAMESPACE_BEGIN(detail)
inline handle get_function(handle value) {
    if (value) {
        return IObject::getFunction(value.ptr());
//        if (PyInstanceMethod_Check(value.ptr())) {
//            value = PyInstanceMethod_GET_FUNCTION(value.ptr());
//        } else if (PyMethod_Check(value.ptr())) {
//            value = PyMethod_GET_FUNCTION(value.ptr());
//        }
    }
    return value;
}

// Reimplementation of python's dict helper functions to ensure that exceptions
// aren't swallowed (see #2862)

// copied from cpython _PyDict_GetItemStringWithError
inline Object *dict_getitemstring(Object *v, const char *key) {
//    Object *kv = nullptr, *rv = nullptr;
//    kv = PyUnicode_FromString(key);
//    if (kv == nullptr) {
//        throw error_already_set();
//    }

//    rv = PyDict_GetItemWithError(v, kv);
//    Py_DECREF(kv);
//    if (rv == nullptr && PyErr_Occurred()) {
//        throw error_already_set();
//    }
   // return rv;
    return IObject::dict_getItemString(v, key);
}

inline Object *dict_getitem(Object *v, Object *key) {
//    Object *rv = PyDict_GetItemWithError(v, key);
//    if (rv == nullptr && PyErr_Occurred()) {
//        throw error_already_set();
//    }
//    return rv;
    return IObject::dict_getItem(v, key);
}

// Helper aliases/functions to support implicit casting of values given to python
// accessors/methods. When given a Object, this simply returns the Object as-is; for other C++
// type, the value goes through BIND11::cast(obj) to convert it to an `object`.
template <typename T, enable_if_t<is_Object<T>::value, int> = 0>
auto object_or_cast(T &&o) -> decltype(std::forward<T>(o)) {
    return std::forward<T>(o);
}
// The following casting version is implemented in cast.h:
template <typename T, enable_if_t<!is_Object<T>::value, int> = 0>
object object_or_cast(T &&o);
// Match a Object*, which we want to convert directly to handle via its converting constructor
inline handle object_or_cast(Object *ptr) { return ptr; }

#if defined(_MSC_VER) && _MSC_VER < 1920
#    pragma warning(push)
#    pragma warning(disable : 4522) // warning C4522: multiple assignment operators specified
#endif
template <typename Policy>
class accessor : public object_api<accessor<Policy>> {
    using key_type = typename Policy::key_type;

public:
    accessor(handle obj, key_type key) : obj(obj), key(std::move(key)) {}
    accessor(const accessor &) = default;
    accessor(accessor &&) noexcept = default;

    // accessor overload required to override default assignment operator (templates are not
    // allowed to replace default compiler-generated assignments).
    void operator=(const accessor &a) && { std::move(*this).operator=(handle(a)); }
    void operator=(const accessor &a) & { operator=(handle(a)); }

    template <typename T>
    void operator=(T &&value) && {
        Policy::set(obj, key, object_or_cast(std::forward<T>(value)));
    }
    template <typename T>
    void operator=(T &&value) & {
        get_cache() = reinterpret_borrow<object>(object_or_cast(std::forward<T>(value)));
    }

    template <typename T = Policy>
    BIND11_DEPRECATED(
        "Use of obj.attr(...) as bool is deprecated in favor of BIND11::hasattr(obj, ...)")
    explicit
    operator enable_if_t<std::is_same<T, accessor_policies::str_attr>::value
                             || std::is_same<T, accessor_policies::obj_attr>::value,
                         bool>() const {
        return hasattr(obj, key);
    }
    template <typename T = Policy>
    BIND11_DEPRECATED("Use of obj[key] as bool is deprecated in favor of obj.contains(key)")
    explicit
    operator enable_if_t<std::is_same<T, accessor_policies::generic_item>::value, bool>() const {
        return obj.contains(key);
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator object() const { return get_cache(); }
    Object *ptr() const { return get_cache().ptr(); }
    template <typename T>
    T cast() const {
        return get_cache().template cast<T>();
    }

private:
    object &get_cache() const {
        if (!cache) {
            cache = Policy::get(obj, key);
        }
        return cache;
    }

private:
    handle obj;
    key_type key;
    mutable object cache;
};
#if defined(_MSC_VER) && _MSC_VER < 1920
#    pragma warning(pop)
#endif

BIND11_NAMESPACE_BEGIN(accessor_policies)
struct obj_attr {
    using key_type = object;
    static object get(handle obj, handle key) { return getattr(obj, key); }
    static void set(handle obj, handle key, handle val) { setattr(obj, key, val); }
};

struct str_attr {
    using key_type = const char *;
    static object get(handle obj, const char *key) { return getattr(obj, key); }
    static void set(handle obj, const char *key, handle val) { setattr(obj, key, val); }
};

struct generic_item {
    using key_type = object;

    //may be field, method, array/map
    static object get(handle obj, handle key) {
        //Object *result = Object_GetItem(obj.ptr(), key.ptr());
        Object *result = IObject::getItem(obj.ptr(), key.ptr());
        if (!result) {
            throw error_already_set();
        }
        return reinterpret_steal<object>(result);
    }

    static void set(handle obj, handle key, handle val) {
        //if (Object_SetItem(obj.ptr(), key.ptr(), val.ptr()) != 0) {
        if (IObject::setItem(obj.ptr(), key.ptr(), val.ptr())) {
            throw error_already_set();
        }
    }
};

struct sequence_item {
    using key_type = size_t;

    template <typename IdxType, detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    static object get(handle obj, const IdxType &index) {
        Object *result = PySequence_GetItem(obj.ptr(), ssize_t_cast(index));
        if (!result) {
            throw error_already_set();
        }
        return reinterpret_steal<object>(result);
    }

    template <typename IdxType, detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    static void set(handle obj, const IdxType &index, handle val) {
        // PySequence_SetItem does not steal a reference to 'val'
        if (PySequence_SetItem(obj.ptr(), ssize_t_cast(index), val.ptr()) != 0) {
            throw error_already_set();
        }
    }
};

struct list_item {
    using key_type = size_t;

    template <typename IdxType, detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    static object get(handle obj, const IdxType &index) {
        Object *result = PyList_GetItem(obj.ptr(), ssize_t_cast(index));
        if (!result) {
            throw error_already_set();
        }
        return reinterpret_borrow<object>(result);
    }

    template <typename IdxType, detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    static void set(handle obj, const IdxType &index, handle val) {
        // PyList_SetItem steals a reference to 'val'
        if (PyList_SetItem(obj.ptr(), ssize_t_cast(index), val.inc_ref().ptr()) != 0) {
            throw error_already_set();
        }
    }
};

struct tuple_item {
    using key_type = size_t;

    template <typename IdxType, detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    static object get(handle obj, const IdxType &index) {
        Object *result = PyTuple_GetItem(obj.ptr(), ssize_t_cast(index));
        if (!result) {
            throw error_already_set();
        }
        return reinterpret_borrow<object>(result);
    }

    template <typename IdxType, detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    static void set(handle obj, const IdxType &index, handle val) {
        // PyTuple_SetItem steals a reference to 'val'
        if (PyTuple_SetItem(obj.ptr(), ssize_t_cast(index), val.inc_ref().ptr()) != 0) {
            throw error_already_set();
        }
    }
};
BIND11_NAMESPACE_END(accessor_policies)

/// STL iterator template used for tuple, list, sequence and dict
template <typename Policy>
class generic_iterator : public Policy {
    using It = generic_iterator;

public:
    using difference_type = ssize_t;
    using iterator_category = typename Policy::iterator_category;
    using value_type = typename Policy::value_type;
    using reference = typename Policy::reference;
    using pointer = typename Policy::pointer;

    generic_iterator() = default;
    generic_iterator(handle seq, ssize_t index) : Policy(seq, index) {}

    // NOLINTNEXTLINE(readability-const-return-type) // PR #3263
    reference operator*() const { return Policy::dereference(); }
    // NOLINTNEXTLINE(readability-const-return-type) // PR #3263
    reference operator[](difference_type n) const { return *(*this + n); }
    pointer operator->() const { return **this; }

    It &operator++() {
        Policy::increment();
        return *this;
    }
    It operator++(int) {
        auto copy = *this;
        Policy::increment();
        return copy;
    }
    It &operator--() {
        Policy::decrement();
        return *this;
    }
    It operator--(int) {
        auto copy = *this;
        Policy::decrement();
        return copy;
    }
    It &operator+=(difference_type n) {
        Policy::advance(n);
        return *this;
    }
    It &operator-=(difference_type n) {
        Policy::advance(-n);
        return *this;
    }

    friend It operator+(const It &a, difference_type n) {
        auto copy = a;
        return copy += n;
    }
    friend It operator+(difference_type n, const It &b) { return b + n; }
    friend It operator-(const It &a, difference_type n) {
        auto copy = a;
        return copy -= n;
    }
    friend difference_type operator-(const It &a, const It &b) { return a.distance_to(b); }

    friend bool operator==(const It &a, const It &b) { return a.equal(b); }
    friend bool operator!=(const It &a, const It &b) { return !(a == b); }
    friend bool operator<(const It &a, const It &b) { return b - a > 0; }
    friend bool operator>(const It &a, const It &b) { return b < a; }
    friend bool operator>=(const It &a, const It &b) { return !(a < b); }
    friend bool operator<=(const It &a, const It &b) { return !(a > b); }
};

BIND11_NAMESPACE_BEGIN(iterator_policies)
/// Quick proxy class needed to implement ``operator->`` for iterators which can't return pointers
template <typename T>
struct arrow_proxy {
    T value;

    // NOLINTNEXTLINE(google-explicit-constructor)
    arrow_proxy(T &&value) noexcept : value(std::move(value)) {}
    T *operator->() const { return &value; }
};

/// Lightweight iterator policy using just a simple pointer: see ``PySequence_Fast_ITEMS``
class sequence_fast_readonly {
protected:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = handle;
    using reference = const handle; // PR #3263
    using pointer = arrow_proxy<const handle>;

    //PySequence_Fast_ITEMS(p)： ret 指针数组
    sequence_fast_readonly(handle obj, ssize_t n) :
       // ptr(PySequence_Fast_ITEMS(obj.ptr()) + n)
        ptr(IObject::sequence_Fast_ITEMS(obj.ptr()) + n)
    {}

    // NOLINTNEXTLINE(readability-const-return-type) // PR #3263
    reference dereference() const { return *ptr; }
    void increment() { ++ptr; }
    void decrement() { --ptr; }
    void advance(ssize_t n) { ptr += n; }
    bool equal(const sequence_fast_readonly &b) const { return ptr == b.ptr; }
    ssize_t distance_to(const sequence_fast_readonly &b) const { return ptr - b.ptr; }

private:
    Object **ptr;
};

/// Full read and write access using the sequence protocol: see ``detail::sequence_accessor``
class sequence_slow_readwrite {
protected:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = object;
    using reference = sequence_accessor;
    using pointer = arrow_proxy<const sequence_accessor>;

    sequence_slow_readwrite(handle obj, ssize_t index) : obj(obj), index(index) {}

    reference dereference() const { return {obj, static_cast<size_t>(index)}; }
    void increment() { ++index; }
    void decrement() { --index; }
    void advance(ssize_t n) { index += n; }
    bool equal(const sequence_slow_readwrite &b) const { return index == b.index; }
    ssize_t distance_to(const sequence_slow_readwrite &b) const { return index - b.index; }

private:
    handle obj;
    ssize_t index;
};

/// Python's dictionary protocol permits this to be a forward iterator
class dict_readonly {
protected:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<handle, handle>;
    using reference = const value_type; // PR #3263
    using pointer = arrow_proxy<const value_type>;

    dict_readonly() = default;
    dict_readonly(handle obj, ssize_t pos) : obj(obj), pos(pos) { increment(); }

    // NOLINTNEXTLINE(readability-const-return-type) // PR #3263
    reference dereference() const { return {key, value}; }
    void increment() {
        if (IObject::dict_Next(obj.ptr(), &pos, &key, &value) == 0) {
            pos = -1;
        }
    }
    bool equal(const dict_readonly &b) const { return pos == b.pos; }

private:
    handle obj;
    Object *key {nullptr};
    Object *value {nullptr};
    ssize_t pos = -1;
};
BIND11_NAMESPACE_END(iterator_policies)

#if !defined(PYPY_VERSION)
using tuple_iterator = generic_iterator<iterator_policies::sequence_fast_readonly>;
using list_iterator = generic_iterator<iterator_policies::sequence_fast_readonly>;
#else
using tuple_iterator = generic_iterator<iterator_policies::sequence_slow_readwrite>;
using list_iterator = generic_iterator<iterator_policies::sequence_slow_readwrite>;
#endif

using sequence_iterator = generic_iterator<iterator_policies::sequence_slow_readwrite>;
using dict_iterator = generic_iterator<iterator_policies::dict_readonly>;

inline bool PyIterable_Check(Object *obj) {
    Object *iter = IObject::getIter(obj);
    if (iter) {
        iter->decRef();
        return true;
    }
    hError_clear();
    return false;
}

inline bool PyNone_Check(Object *o) { return IObject::isNone(o); }
inline bool PyEllipsis_Check(Object *o) { return IObject::isEllipsis(o); }

#ifdef BIND11_STR_LEGACY_PERMISSIVE
inline bool PyUnicode_Check_Permissive(Object *o) {
    return PyUnicode_Check(o) || BIND11_BYTES_CHECK(o);
}
#    define BIND11_STR_CHECK_FUN detail::PyUnicode_Check_Permissive
#else
#    define BIND11_STR_CHECK_FUN PyUnicode_Check
#endif

inline bool PyStaticMethod_Check(Object *o) { return IObject::isStaticMethod(o); }

class kwargs_proxy : public handle {
public:
    explicit kwargs_proxy(handle h) : handle(h) {}
};

class args_proxy : public handle {
public:
    explicit args_proxy(handle h) : handle(h) {}
    kwargs_proxy operator*() const { return kwargs_proxy(*this); }
};

/// Python argument categories (using PEP 448 terms)
template <typename T>
using is_keyword = std::is_base_of<arg, T>;
template <typename T>
using is_s_unpacking = std::is_same<args_proxy, T>; // * unpacking
template <typename T>
using is_ds_unpacking = std::is_same<kwargs_proxy, T>; // ** unpacking
template <typename T>
using is_positional = satisfies_none_of<T, is_keyword, is_s_unpacking, is_ds_unpacking>;
template <typename T>
using is_keyword_or_ds = satisfies_any_of<T, is_keyword, is_ds_unpacking>;

// Call argument collector forward declarations
template <return_value_policy policy = return_value_policy::automatic_reference>
class simple_collector;
template <return_value_policy policy = return_value_policy::automatic_reference>
class unpacking_collector;

BIND11_NAMESPACE_END(detail)

// TODO: After the deprecated constructors are removed, this macro can be simplified by
//       inheriting ctors: `using Parent::Parent`. It's not an option right now because
//       the `using` statement triggers the parent deprecation warning even if the ctor
//       isn't even used.
#define BIND11_OBJECT_COMMON(Name, Parent, CheckFun)                                            \
public:                                                                                           \
    BIND11_DEPRECATED("Use reinterpret_borrow<" #Name ">() or reinterpret_steal<" #Name ">()")  \
    Name(handle h, bool is_borrowed)                                                              \
        : Parent(is_borrowed ? Parent(h, borrowed_t{}) : Parent(h, stolen_t{})) {}                \
    Name(handle h, borrowed_t) : Parent(h, borrowed_t{}) {}                                       \
    Name(handle h, stolen_t) : Parent(h, stolen_t{}) {}                                           \
    BIND11_DEPRECATED("Use py::isinstance<py::python_type>(obj) instead")                       \
    bool check() const { return m_ptr != nullptr && (CheckFun(m_ptr) != 0); }                     \
    static bool check_(handle h) { return h.ptr() != nullptr && CheckFun(h.ptr()); }              \
    template <typename Policy_> /* NOLINTNEXTLINE(google-explicit-constructor) */                 \
    Name(const ::BIND11::detail::accessor<Policy_> &a) : Name(object(a)) {}

#define BIND11_OBJECT_CVT(Name, Parent, CheckFun, ConvertFun)                                   \
    BIND11_OBJECT_COMMON(Name, Parent, CheckFun)                                                \
    /* This is deliberately not 'explicit' to allow implicit conversion from object: */           \
    /* NOLINTNEXTLINE(google-explicit-constructor) */                                             \
    Name(const object &o)                                                                         \
        : Parent(check_(o) ? o.inc_ref().ptr() : ConvertFun(o.ptr()), stolen_t{}) {               \
        if (!m_ptr)                                                                               \
            throw ::BIND11::error_already_set();                                                \
    }                                                                                             \
    /* NOLINTNEXTLINE(google-explicit-constructor) */                                             \
    Name(object &&o) : Parent(check_(o) ? o.release().ptr() : ConvertFun(o.ptr()), stolen_t{}) {  \
        if (!m_ptr)                                                                               \
            throw ::BIND11::error_already_set();                                                \
    }

#define BIND11_OBJECT_CVT_DEFAULT(Name, Parent, CheckFun, ConvertFun)                           \
    BIND11_OBJECT_CVT(Name, Parent, CheckFun, ConvertFun)                                       \
    Name() : Parent() {}

#define BIND11_OBJECT_CHECK_FAILED(Name, o_ptr)                                                 \
    ::BIND11::type_error("Object of type '"                                                     \
                           + ::BIND11::detail::get_fully_qualified_tp_name(o_ptr)      \
                           + "' is not an instance of '" #Name "'")

#define BIND11_OBJECT(Name, Parent, CheckFun)                                                   \
    BIND11_OBJECT_COMMON(Name, Parent, CheckFun)                                                \
    /* This is deliberately not 'explicit' to allow implicit conversion from object: */           \
    /* NOLINTNEXTLINE(google-explicit-constructor) */                                             \
    Name(const object &o) : Parent(o) {                                                           \
        if (m_ptr && !check_(m_ptr))                                                              \
            throw BIND11_OBJECT_CHECK_FAILED(Name, m_ptr);                                      \
    }                                                                                             \
    /* NOLINTNEXTLINE(google-explicit-constructor) */                                             \
    Name(object &&o) : Parent(std::move(o)) {                                                     \
        if (m_ptr && !check_(m_ptr))                                                              \
            throw BIND11_OBJECT_CHECK_FAILED(Name, m_ptr);                                      \
    }

#define BIND11_OBJECT_DEFAULT(Name, Parent, CheckFun)                                           \
    BIND11_OBJECT(Name, Parent, CheckFun)                                                       \
    Name() : Parent() {}

/// \addtogroup pytypes
/// @{

/** \rst
    Wraps a Python iterator so that it can also be used as a C++ input iterator

    Caveat: copying an iterator does not (and cannot) clone the internal
    state of the Python iterable. This also applies to the post-increment
    operator. This iterator should only be used to retrieve the current
    value using ``operator*()``.
\endrst */
class iterator : public object {
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = ssize_t;
    using value_type = handle;
    using reference = const handle; // PR #3263
    using pointer = const handle *;

    //BIND11_OBJECT_DEFAULT(iterator, object, PyIter_Check)
    BIND11_OBJECT_DEFAULT(iterator, object, True_check)

    iterator &operator++() {
        advance();
        return *this;
    }

    iterator operator++(int) {
        auto rv = *this;
        advance();
        return rv;
    }

    // NOLINTNEXTLINE(readability-const-return-type) // PR #3263
    reference operator*() const {
        if (m_ptr && !value.ptr()) {
            auto &self = const_cast<iterator &>(*this);
            self.advance();
        }
        return value;
    }

    pointer operator->() const {
        operator*();
        return &value;
    }

    /** \rst
         The value which marks the end of the iteration. ``it == iterator::sentinel()``
         is equivalent to catching ``StopIteration`` in Python.

         .. code-block:: cpp

             void foo(py::iterator it) {
                 while (it != py::iterator::sentinel()) {
                    // use `*it`
                    ++it;
                 }
             }
    \endrst */
    static iterator sentinel() { return {}; }

    friend bool operator==(const iterator &a, const iterator &b) {
        return a->ptr() == b->ptr(); }
    friend bool operator!=(const iterator &a, const iterator &b) {
        return a->ptr() != b->ptr(); }

private:
    void advance() {
        value = reinterpret_steal<object>(IObject::iter_Next(m_ptr));
        if (hError_Occurred()) {
            throw error_already_set();
        }
    }

private:
    object value = {};
};

class type : public object {
public:
    BIND11_OBJECT(type, object, True_check)

    /// Return a type handle from a handle or an object
    static handle handle_of(handle h) { return handle((Object *) IObject::type(h.ptr())); }

    /// Return a type object from a handle or an object
    static type of(handle h) { return type(type::handle_of(h), borrowed_t{}); }

    // Defined in BIND11/cast.h
    /// Convert C++ type to handle if previously registered. Does not convert
    /// standard types, like int, float. etc. yet.
    /// See https://github.com/pybind/BIND11/issues/2486
    template <typename T>
    static handle handle_of();

    /// Convert C++ type to type if previously registered. Does not convert
    /// standard types, like int, float. etc. yet.
    /// See https://github.com/pybind/BIND11/issues/2486
    template <typename T>
    static type of() {
        return type(type::handle_of<T>(), borrowed_t{});
    }
};

class iterable : public object {
public:
    BIND11_OBJECT_DEFAULT(iterable, object, detail::PyIterable_Check)
};

class bytes;

class str : public object {
public:
    //BIND11_OBJECT_CVT(str, object, BIND11_STR_CHECK_FUN, raw_str)
    BIND11_OBJECT_CVT(str, object, hUnicode_Check, raw_str)

    template <typename SzType, detail::enable_if_t<std::is_integral<SzType>::value, int> = 0>
    str(const char *c, const SzType &n)
        : object(hUnicode_FromStringAndSize(c, ssize_t_cast(n)), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate string object!");
        }
    }

    // 'explicit' is explicitly omitted from the following constructors to allow implicit
    // conversion to py::str from C++ string-like objects
    // NOLINTNEXTLINE(google-explicit-constructor)
    str(const char *c = "") : object(hUnicode_FromString(c), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate string object!");
        }
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    str(const std::string &s) : str(s.data(), s.size()) {}

#ifdef BIND11_HAS_STRING_VIEW
    // enable_if is needed to avoid "ambiguous conversion" errors (see PR #3521).
    template <typename T, detail::enable_if_t<std::is_same<T, std::string_view>::value, int> = 0>
    // NOLINTNEXTLINE(google-explicit-constructor)
    str(T s) : str(s.data(), s.size()) {}

#    ifdef BIND11_HAS_U8STRING
    // reinterpret_cast here is safe (C++20 guarantees char8_t has the same size/alignment as char)
    // NOLINTNEXTLINE(google-explicit-constructor)
    str(std::u8string_view s) : str(reinterpret_cast<const char *>(s.data()), s.size()) {}
#    endif

#endif

    explicit str(const bytes &b);

    /** \rst
        Return a string representation of the object. This is analogous to
        the ``str()`` function in Python.
    \endrst */
    explicit str(handle h) : object(raw_str(h.ptr()), stolen_t{}) {
        if (!m_ptr) {
            throw error_already_set();
        }
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string() const {
        object temp = *this;
        if (hUnicode_Check(m_ptr)) {
            temp = reinterpret_steal<object>(hUnicode_AsUTF8String(m_ptr));
            if (!temp) {
                throw error_already_set();
            }
        }
        char *buffer = nullptr;
        ssize_t length = 0;
        if (hBytes_AsStringAndSize(temp.ptr(), &buffer, &length) != 0) {
            throw error_already_set();
        }
        return std::string(buffer, (size_t) length);
    }

    template <typename... Args>
    str format(Args &&...args) const {
        return attr("format")(std::forward<Args>(args)...);
    }

private:
    /// Return string representation -- always returns a new reference, even if already a str
    static Object *raw_str(Object *op) {
        Object *str_value = IObject::str(op);
        return str_value;
    }
};
/// @} pytypes

inline namespace literals {
/** \rst
    String literal version of `str`
 \endrst */
inline str operator"" _s(const char *s, size_t size) { return {s, size}; }
} // namespace literals

/// \addtogroup pytypes
/// @{
class bytes : public object {
public:
    BIND11_OBJECT(bytes, object, hBytes_Check)

    // Allow implicit conversion:
    // NOLINTNEXTLINE(google-explicit-constructor)
    bytes(const char *c = "") : object(hBytes_FromString(c), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate bytes object!");
        }
    }

    template <typename SzType, detail::enable_if_t<std::is_integral<SzType>::value, int> = 0>
    bytes(const char *c, const SzType &n)
        : object(hBytes_FromStringAndSize(c, ssize_t_cast(n)), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate bytes object!");
        }
    }

    // Allow implicit conversion:
    // NOLINTNEXTLINE(google-explicit-constructor)
    bytes(const std::string &s) : bytes(s.data(), s.size()) {}

    explicit bytes(const BIND11::str &s);

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string() const { return string_op<std::string>(); }

#ifdef BIND11_HAS_STRING_VIEW
    // enable_if is needed to avoid "ambiguous conversion" errors (see PR #3521).
    template <typename T, detail::enable_if_t<std::is_same<T, std::string_view>::value, int> = 0>
    // NOLINTNEXTLINE(google-explicit-constructor)
    bytes(T s) : bytes(s.data(), s.size()) {}

    // Obtain a string view that views the current `bytes` buffer value.  Note that this is only
    // valid so long as the `bytes` instance remains alive and so generally should not outlive the
    // lifetime of the `bytes` instance.
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::string_view() const { return string_op<std::string_view>(); }
#endif
private:
    template <typename T>
    T string_op() const {
        char *buffer = nullptr;
        ssize_t length = 0;
        if (hBytes_AsStringAndSize(m_ptr, &buffer, &length) != 0) {
            throw error_already_set();
        }
        return {buffer, static_cast<size_t>(length)};
    }
};
// Note: breathe >= 4.17.0 will fail to build docs if the below two constructors
// are included in the doxygen group; close here and reopen after as a workaround
/// @} pytypes

inline bytes::bytes(const BIND11::str &s) {
    object temp = s;
    if (hUnicode_Check(s.ptr())) {
        temp = reinterpret_steal<object>(hUnicode_AsUTF8String(s.ptr()));
        if (!temp) {
            throw error_already_set();
        }
    }
    char *buffer = nullptr;
    ssize_t length = 0;
    if (hBytes_AsStringAndSize(temp.ptr(), &buffer, &length) != 0) {
        throw error_already_set();
    }
    auto obj = reinterpret_steal<object>(hBytes_FromStringAndSize(buffer, length));
    if (!obj) {
        BIND11_fail("Could not allocate bytes object!");
    }
    m_ptr = obj.release().ptr();
}

inline str::str(const bytes &b) {
    char *buffer = nullptr;
    ssize_t length = 0;
    if (hBytes_AsStringAndSize(b.ptr(), &buffer, &length) != 0) {
        throw error_already_set();
    }
    auto obj = reinterpret_steal<object>(hUnicode_FromStringAndSize(buffer, length));
    if (!obj) {
        BIND11_fail("Could not allocate string object!");
    }
    m_ptr = obj.release().ptr();
}

/// \addtogroup pytypes
/// @{
class bytearray : public object {
public:
    BIND11_OBJECT_CVT(bytearray, object, hByteArray_Check, hByteArray_FromObject)

    template <typename SzType, detail::enable_if_t<std::is_integral<SzType>::value, int> = 0>
    bytearray(const char *c, const SzType &n)
        : object(hByteArray_FromStringAndSize(c, ssize_t_cast(n)), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate bytearray object!");
        }
    }

    bytearray() : bytearray("", 0) {}

    explicit bytearray(const std::string &s) : bytearray(s.data(), s.size()) {}

    size_t size() const { return static_cast<size_t>(hByteArray_Size(m_ptr)); }

    explicit operator std::string() const {
        return hByteArray_to_rawStr(m_ptr);
    }
};
// Note: breathe >= 4.17.0 will fail to build docs if the below two constructors
// are included in the doxygen group; close here and reopen after as a workaround
/// @} pytypes

/// \addtogroup pytypes
/// @{
class none : public object {
public:
    BIND11_OBJECT(none, object, detail::PyNone_Check)
    none() : object(IObject::getNone(), borrowed_t{}) {}
};

class ellipsis : public object {
public:
    BIND11_OBJECT(ellipsis, object, detail::PyEllipsis_Check)
    ellipsis() : object(IObject::getEllipsis(), borrowed_t{}) {}
};

class bool_ : public object {
public:
    BIND11_OBJECT_CVT(bool_, object, hBool_Check, raw_bool)
    bool_() : object(IObject::getFalse(), borrowed_t{}) {}
    // Allow implicit conversion from and to `bool`:
    // NOLINTNEXTLINE(google-explicit-constructor)
    bool_(bool value) : object(value ? IObject::getTrue() : IObject::getFalse(), borrowed_t{}) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() const { return (m_ptr != nullptr) && hLong_AsLong(m_ptr) != 0; }

private:
    /// Return the truth value of an object -- always returns a new reference
    static Object *raw_bool(Object *op) {
        const auto value = hObject_IsTrue(op);
        if (value == -1) {
            return nullptr;
        }
        return handle(value != 0 ? IObject::getTrue() : IObject::getFalse()).inc_ref().ptr();
    }
};

BIND11_NAMESPACE_BEGIN(detail)
// Converts a value to the given unsigned type.  If an error occurs, you get back (Unsigned) -1;
// otherwise you get back the unsigned long or unsigned long long value cast to (Unsigned).
// (The distinction is critically important when casting a returned -1 error value to some other
// unsigned type: (A)-1 != (B)-1 when A and B are unsigned types of different sizes).
template <typename Unsigned>
Unsigned as_unsigned(Object *o) {
    if (BIND11_SILENCE_MSVC_C4127(sizeof(Unsigned) <= sizeof(unsigned long))) {
        unsigned long v = hLong_AsUnsignedLong(o);
        return v == (unsigned long) -1 && hError_Occurred() ? (Unsigned) -1 : (Unsigned) v;
    }
    unsigned long long v = hLong_AsUnsignedLongLong(o);
    return v == (unsigned long long) -1 && hError_Occurred() ? (Unsigned) -1 : (Unsigned) v;
}
BIND11_NAMESPACE_END(detail)

class int_ : public object {
public:
    BIND11_OBJECT_CVT(int_, object, BIND11_LONG_CHECK, PyNumber_Long)
    int_() : object(PyLong_FromLong(0), stolen_t{}) {}
    // Allow implicit conversion from C++ integral types:
    template <typename T, detail::enable_if_t<std::is_integral<T>::value, int> = 0>
    // NOLINTNEXTLINE(google-explicit-constructor)
    int_(T value) {
        if (BIND11_SILENCE_MSVC_C4127(sizeof(T) <= sizeof(long))) {
            if (std::is_signed<T>::value) {
                m_ptr = PyLong_FromLong((long) value);
            } else {
                m_ptr = PyLong_FromUnsignedLong((unsigned long) value);
            }
        } else {
            if (std::is_signed<T>::value) {
                m_ptr = PyLong_FromLongLong((long long) value);
            } else {
                m_ptr = PyLong_FromUnsignedLongLong((unsigned long long) value);
            }
        }
        if (!m_ptr) {
            BIND11_fail("Could not allocate int object!");
        }
    }

    template <typename T, detail::enable_if_t<std::is_integral<T>::value, int> = 0>
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator T() const {
        return std::is_unsigned<T>::value  ? detail::as_unsigned<T>(m_ptr)
               : sizeof(T) <= sizeof(long) ? (T) PyLong_AsLong(m_ptr)
                                           : (T) BIND11_LONG_AS_LONGLONG(m_ptr);
    }
};

class float_ : public object {
public:
    BIND11_OBJECT_CVT(float_, object, PyFloat_Check, PyNumber_Float)
    // Allow implicit conversion from float/double:
    // NOLINTNEXTLINE(google-explicit-constructor)
    float_(float value) : object(PyFloat_FromDouble((double) value), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate float object!");
        }
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    float_(double value = .0) : object(PyFloat_FromDouble((double) value), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate float object!");
        }
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator float() const { return (float) PyFloat_AsDouble(m_ptr); }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator double() const { return (double) PyFloat_AsDouble(m_ptr); }
};

class weakref : public object {
public:
    BIND11_OBJECT_CVT_DEFAULT(weakref, object, PyWeakref_Check, raw_weakref)
    explicit weakref(handle obj, handle callback = {})
        : object(PyWeakref_NewRef(obj.ptr(), callback.ptr()), stolen_t{}) {
        if (!m_ptr) {
            if (PyErr_Occurred()) {
                throw error_already_set();
            }
            BIND11_fail("Could not allocate weak reference!");
        }
    }

private:
    static Object *raw_weakref(Object *o) { return PyWeakref_NewRef(o, nullptr); }
};

class slice : public object {
public:
    BIND11_OBJECT_DEFAULT(slice, object, PySlice_Check)
    slice(handle start, handle stop, handle step)
        : object(PySlice_New(start.ptr(), stop.ptr(), step.ptr()), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate slice object!");
        }
    }

#ifdef BIND11_HAS_OPTIONAL
    slice(std::optional<ssize_t> start, std::optional<ssize_t> stop, std::optional<ssize_t> step)
        : slice(index_to_object(start), index_to_object(stop), index_to_object(step)) {}
#else
    slice(ssize_t start_, ssize_t stop_, ssize_t step_)
        : slice(int_(start_), int_(stop_), int_(step_)) {}
#endif

    bool
    compute(size_t length, size_t *start, size_t *stop, size_t *step, size_t *slicelength) const {
        return PySlice_GetIndicesEx((BIND11_SLICE_OBJECT *) m_ptr,
                                    (ssize_t) length,
                                    (ssize_t *) start,
                                    (ssize_t *) stop,
                                    (ssize_t *) step,
                                    (ssize_t *) slicelength)
               == 0;
    }
    bool compute(
        ssize_t length, ssize_t *start, ssize_t *stop, ssize_t *step, ssize_t *slicelength) const {
        return PySlice_GetIndicesEx(
                   (BIND11_SLICE_OBJECT *) m_ptr, length, start, stop, step, slicelength)
               == 0;
    }

private:
    template <typename T>
    static object index_to_object(T index) {
        return index ? object(int_(*index)) : object(none());
    }
};

class capsule : public object {
public:
    BIND11_OBJECT_DEFAULT(capsule, object, PyCapsule_CheckExact)
    BIND11_DEPRECATED("Use reinterpret_borrow<capsule>() or reinterpret_steal<capsule>()")
    capsule(Object *ptr, bool is_borrowed)
        : object(is_borrowed ? object(ptr, borrowed_t{}) : object(ptr, stolen_t{})) {}

    explicit capsule(const void *value,
                     const char *name = nullptr,
                     void (*destructor)(Object *) = nullptr)
        : object(PyCapsule_New(const_cast<void *>(value), name, destructor), stolen_t{}) {
        if (!m_ptr) {
            throw error_already_set();
        }
    }

    BIND11_DEPRECATED("Please pass a destructor that takes a void pointer as input")
    capsule(const void *value, void (*destruct)(Object *))
        : object(PyCapsule_New(const_cast<void *>(value), nullptr, destruct), stolen_t{}) {
        if (!m_ptr) {
            throw error_already_set();
        }
    }

    capsule(const void *value, void (*destructor)(void *)) {
        m_ptr = PyCapsule_New(const_cast<void *>(value), nullptr, [](Object *o) {
            auto destructor = reinterpret_cast<void (*)(void *)>(PyCapsule_GetContext(o));
            if (destructor == nullptr) {
                if (PyErr_Occurred()) {
                    throw error_already_set();
                }
                BIND11_fail("Unable to get capsule context");
            }
            const char *name = get_name_in_error_scope(o);
            void *ptr = PyCapsule_GetPointer(o, name);
            if (ptr == nullptr) {
                throw error_already_set();
            }
            destructor(ptr);
        });

        if (!m_ptr || PyCapsule_SetContext(m_ptr, (void *) destructor) != 0) {
            throw error_already_set();
        }
    }

    explicit capsule(void (*destructor)()) {
        m_ptr = PyCapsule_New(reinterpret_cast<void *>(destructor), nullptr, [](Object *o) {
            const char *name = get_name_in_error_scope(o);
            auto destructor = reinterpret_cast<void (*)()>(PyCapsule_GetPointer(o, name));
            if (destructor == nullptr) {
                throw error_already_set();
            }
            destructor();
        });

        if (!m_ptr) {
            throw error_already_set();
        }
    }

    template <typename T>
    operator T *() const { // NOLINT(google-explicit-constructor)
        return get_pointer<T>();
    }

    /// Get the pointer the capsule holds.
    template <typename T = void>
    T *get_pointer() const {
        const auto *name = this->name();
        T *result = static_cast<T *>(PyCapsule_GetPointer(m_ptr, name));
        if (!result) {
            throw error_already_set();
        }
        return result;
    }

    /// Replaces a capsule's pointer *without* calling the destructor on the existing one.
    void set_pointer(const void *value) {
        if (PyCapsule_SetPointer(m_ptr, const_cast<void *>(value)) != 0) {
            throw error_already_set();
        }
    }

    const char *name() const {
        const char *name = PyCapsule_GetName(m_ptr);
        if ((name == nullptr) && PyErr_Occurred()) {
            throw error_already_set();
        }
        return name;
    }

    /// Replaces a capsule's name *without* calling the destructor on the existing one.
    void set_name(const char *new_name) {
        if (PyCapsule_SetName(m_ptr, new_name) != 0) {
            throw error_already_set();
        }
    }

private:
    static const char *get_name_in_error_scope(Object *o) {
        error_scope error_guard;

        const char *name = PyCapsule_GetName(o);
        if ((name == nullptr) && PyErr_Occurred()) {
            // write out and consume error raised by call to PyCapsule_GetName
            PyErr_WriteUnraisable(o);
        }

        return name;
    }
};

class tuple : public object {
public:
    BIND11_OBJECT_CVT(tuple, object, PyTuple_Check, PySequence_Tuple)
    template <typename SzType = ssize_t,
              detail::enable_if_t<std::is_integral<SzType>::value, int> = 0>
    // Some compilers generate link errors when using `const SzType &` here:
    explicit tuple(SzType size = 0) : object(PyTuple_New(ssize_t_cast(size)), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate tuple object!");
        }
    }
    size_t size() const { return (size_t) PyTuple_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    detail::tuple_accessor operator[](size_t index) const { return {*this, index}; }
    detail::item_accessor operator[](handle h) const { return object::operator[](h); }
    detail::tuple_iterator begin() const { return {*this, 0}; }
    detail::tuple_iterator end() const { return {*this, PyTuple_GET_SIZE(m_ptr)}; }
};

// We need to put this into a separate function because the Intel compiler
// fails to compile enable_if_t<all_of<is_keyword_or_ds<Args>...>::value> part below
// (tested with ICC 2021.1 Beta 20200827).
template <typename... Args>
constexpr bool args_are_all_keyword_or_ds() {
    return detail::all_of<detail::is_keyword_or_ds<Args>...>::value;
}

class dict : public object {
public:
    BIND11_OBJECT_CVT(dict, object, PyDict_Check, raw_dict)
    dict() : object(PyDict_New(), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate dict object!");
        }
    }
    template <typename... Args,
              typename = detail::enable_if_t<args_are_all_keyword_or_ds<Args...>()>,
              // MSVC workaround: it can't compile an out-of-line definition, so defer the
              // collector
              typename collector = detail::deferred_t<detail::unpacking_collector<>, Args...>>
    explicit dict(Args &&...args) : dict(collector(std::forward<Args>(args)...).kwargs()) {}

    size_t size() const { return (size_t) PyDict_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    detail::dict_iterator begin() const { return {*this, 0}; }
    detail::dict_iterator end() const { return {}; }
    void clear() /* py-non-const */ { PyDict_Clear(ptr()); }
    template <typename T>
    bool contains(T &&key) const {
        return PyDict_Contains(m_ptr, detail::object_or_cast(std::forward<T>(key)).ptr()) == 1;
    }

private:
    /// Call the `dict` Python type -- always returns a new reference
    static Object *raw_dict(Object *op) {
        if (PyDict_Check(op)) {
            return handle(op).inc_ref().ptr();
        }
        return Object_CallFunctionObjArgs((Object *) &PyDict_Type, op, nullptr);
    }
};

class sequence : public object {
public:
    BIND11_OBJECT_DEFAULT(sequence, object, PySequence_Check)
    size_t size() const {
        ssize_t result = PySequence_Size(m_ptr);
        if (result == -1) {
            throw error_already_set();
        }
        return (size_t) result;
    }
    bool empty() const { return size() == 0; }
    detail::sequence_accessor operator[](size_t index) const { return {*this, index}; }
    detail::item_accessor operator[](handle h) const { return object::operator[](h); }
    detail::sequence_iterator begin() const { return {*this, 0}; }
    detail::sequence_iterator end() const { return {*this, PySequence_Size(m_ptr)}; }
};

class list : public object {
public:
    BIND11_OBJECT_CVT(list, object, PyList_Check, PySequence_List)
    template <typename SzType = ssize_t,
              detail::enable_if_t<std::is_integral<SzType>::value, int> = 0>
    // Some compilers generate link errors when using `const SzType &` here:
    explicit list(SzType size = 0) : object(PyList_New(ssize_t_cast(size)), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate list object!");
        }
    }
    size_t size() const { return (size_t) PyList_Size(m_ptr); }
    bool empty() const { return size() == 0; }
    detail::list_accessor operator[](size_t index) const { return {*this, index}; }
    detail::item_accessor operator[](handle h) const { return object::operator[](h); }
    detail::list_iterator begin() const { return {*this, 0}; }
    detail::list_iterator end() const { return {*this, PyList_GET_SIZE(m_ptr)}; }
    template <typename T>
    void append(T &&val) /* py-non-const */ {
        PyList_Append(m_ptr, detail::object_or_cast(std::forward<T>(val)).ptr());
    }
    template <typename IdxType,
              typename ValType,
              detail::enable_if_t<std::is_integral<IdxType>::value, int> = 0>
    void insert(const IdxType &index, ValType &&val) /* py-non-const */ {
        PyList_Insert(
            m_ptr, ssize_t_cast(index), detail::object_or_cast(std::forward<ValType>(val)).ptr());
    }
};

class args : public tuple {
    BIND11_OBJECT_DEFAULT(args, tuple, PyTuple_Check)
};
class kwargs : public dict {
    BIND11_OBJECT_DEFAULT(kwargs, dict, PyDict_Check)
};

class anyset : public object {
public:
    BIND11_OBJECT(anyset, object, PyAnySet_Check)
    size_t size() const { return static_cast<size_t>(PySet_Size(m_ptr)); }
    bool empty() const { return size() == 0; }
    template <typename T>
    bool contains(T &&val) const {
        return PySet_Contains(m_ptr, detail::object_or_cast(std::forward<T>(val)).ptr()) == 1;
    }
};

class set : public anyset {
public:
    BIND11_OBJECT_CVT(set, anyset, PySet_Check, PySet_New)
    set() : anyset(PySet_New(nullptr), stolen_t{}) {
        if (!m_ptr) {
            BIND11_fail("Could not allocate set object!");
        }
    }
    template <typename T>
    bool add(T &&val) /* py-non-const */ {
        return PySet_Add(m_ptr, detail::object_or_cast(std::forward<T>(val)).ptr()) == 0;
    }
    void clear() /* py-non-const */ { PySet_Clear(m_ptr); }
};

class frozenset : public anyset {
public:
    BIND11_OBJECT_CVT(frozenset, anyset, PyFrozenSet_Check, PyFrozenSet_New)
};

class function : public object {
public:
    BIND11_OBJECT_DEFAULT(function, object, PyCallable_Check)
    handle cpp_function() const {
        handle fun = detail::get_function(m_ptr);
        if (fun && PyCFunction_Check(fun.ptr())) {
            return fun;
        }
        return handle();
    }
    bool is_cpp_function() const { return (bool) cpp_function(); }
};

class staticmethod : public object {
public:
    BIND11_OBJECT_CVT(staticmethod, object, detail::PyStaticMethod_Check, PyStaticMethod_New)
};

class buffer : public object {
public:
    BIND11_OBJECT_DEFAULT(buffer, object, Object_CheckBuffer)

    buffer_info request(bool writable = false) const {
        int flags = PyBUF_STRIDES | PyBUF_FORMAT;
        if (writable) {
            flags |= PyBUF_WRITABLE;
        }
        auto *view = new Py_buffer();
        if (Object_GetBuffer(m_ptr, view, flags) != 0) {
            delete view;
            throw error_already_set();
        }
        return buffer_info(view);
    }
};

class memoryview : public object {
public:
    BIND11_OBJECT_CVT(memoryview, object, PyMemoryView_Check, PyMemoryView_FromObject)

    /** \rst
        Creates ``memoryview`` from ``buffer_info``.

        ``buffer_info`` must be created from ``buffer::request()``. Otherwise
        throws an exception.

        For creating a ``memoryview`` from objects that support buffer protocol,
        use ``memoryview(const object& obj)`` instead of this constructor.
     \endrst */
    explicit memoryview(const buffer_info &info) {
        if (!info.view()) {
            BIND11_fail("Prohibited to create memoryview without Py_buffer");
        }
        // Note: PyMemoryView_FromBuffer never increments obj reference.
        m_ptr = (info.view()->obj) ? PyMemoryView_FromObject(info.view()->obj)
                                   : PyMemoryView_FromBuffer(info.view());
        if (!m_ptr) {
            BIND11_fail("Unable to create memoryview from buffer descriptor");
        }
    }

    /** \rst
        Creates ``memoryview`` from static buffer.

        This method is meant for providing a ``memoryview`` for C/C++ buffer not
        managed by Python. The caller is responsible for managing the lifetime
        of ``ptr`` and ``format``, which MUST outlive the memoryview constructed
        here.

        See also: Python C API documentation for `PyMemoryView_FromBuffer`_.

        .. _PyMemoryView_FromBuffer:
           https://docs.python.org/c-api/memoryview.html#c.PyMemoryView_FromBuffer

        :param ptr: Pointer to the buffer.
        :param itemsize: Byte size of an element.
        :param format: Pointer to the null-terminated format string. For
            homogeneous Buffers, this should be set to
            ``format_descriptor<T>::value``.
        :param shape: Shape of the tensor (1 entry per dimension).
        :param strides: Number of bytes between adjacent entries (for each
            per dimension).
        :param readonly: Flag to indicate if the underlying storage may be
            written to.
     \endrst */
    static memoryview from_buffer(void *ptr,
                                  ssize_t itemsize,
                                  const char *format,
                                  detail::any_container<ssize_t> shape,
                                  detail::any_container<ssize_t> strides,
                                  bool readonly = false);

    static memoryview from_buffer(const void *ptr,
                                  ssize_t itemsize,
                                  const char *format,
                                  detail::any_container<ssize_t> shape,
                                  detail::any_container<ssize_t> strides) {
        return memoryview::from_buffer(
            const_cast<void *>(ptr), itemsize, format, std::move(shape), std::move(strides), true);
    }

    template <typename T>
    static memoryview from_buffer(T *ptr,
                                  detail::any_container<ssize_t> shape,
                                  detail::any_container<ssize_t> strides,
                                  bool readonly = false) {
        return memoryview::from_buffer(reinterpret_cast<void *>(ptr),
                                       sizeof(T),
                                       format_descriptor<T>::value,
                                       std::move(shape),
                                       std::move(strides),
                                       readonly);
    }

    template <typename T>
    static memoryview from_buffer(const T *ptr,
                                  detail::any_container<ssize_t> shape,
                                  detail::any_container<ssize_t> strides) {
        return memoryview::from_buffer(
            const_cast<T *>(ptr), std::move(shape), std::move(strides), true);
    }

    /** \rst
        Creates ``memoryview`` from static memory.

        This method is meant for providing a ``memoryview`` for C/C++ buffer not
        managed by Python. The caller is responsible for managing the lifetime
        of ``mem``, which MUST outlive the memoryview constructed here.

        See also: Python C API documentation for `PyMemoryView_FromBuffer`_.

        .. _PyMemoryView_FromMemory:
           https://docs.python.org/c-api/memoryview.html#c.PyMemoryView_FromMemory
     \endrst */
    static memoryview from_memory(void *mem, ssize_t size, bool readonly = false) {
        Object *ptr = PyMemoryView_FromMemory(
            reinterpret_cast<char *>(mem), size, (readonly) ? PyBUF_READ : PyBUF_WRITE);
        if (!ptr) {
            BIND11_fail("Could not allocate memoryview object!");
        }
        return memoryview(object(ptr, stolen_t{}));
    }

    static memoryview from_memory(const void *mem, ssize_t size) {
        return memoryview::from_memory(const_cast<void *>(mem), size, true);
    }

#ifdef BIND11_HAS_STRING_VIEW
    static memoryview from_memory(std::string_view mem) {
        return from_memory(const_cast<char *>(mem.data()), static_cast<ssize_t>(mem.size()), true);
    }
#endif
};

/// @cond DUPLICATE
inline memoryview memoryview::from_buffer(void *ptr,
                                          ssize_t itemsize,
                                          const char *format,
                                          detail::any_container<ssize_t> shape,
                                          detail::any_container<ssize_t> strides,
                                          bool readonly) {
    size_t ndim = shape->size();
    if (ndim != strides->size()) {
        BIND11_fail("memoryview: shape length doesn't match strides length");
    }
    ssize_t size = ndim != 0u ? 1 : 0;
    for (size_t i = 0; i < ndim; ++i) {
        size *= (*shape)[i];
    }
    Py_buffer view;
    view.buf = ptr;
    view.obj = nullptr;
    view.len = size * itemsize;
    view.readonly = static_cast<int>(readonly);
    view.itemsize = itemsize;
    view.format = const_cast<char *>(format);
    view.ndim = static_cast<int>(ndim);
    view.shape = shape->data();
    view.strides = strides->data();
    view.suboffsets = nullptr;
    view.internal = nullptr;
    Object *obj = PyMemoryView_FromBuffer(&view);
    if (!obj) {
        throw error_already_set();
    }
    return memoryview(object(obj, stolen_t{}));
}
/// @endcond
/// @} pytypes

/// \addtogroup python_builtins
/// @{

/// Get the length of a Python object.
inline size_t len(handle h) {
    ssize_t result = IObject::length(h.ptr());
    if (result < 0) {
        throw error_already_set();
    }
    return (size_t) result;
}

/// Get the length hint of a Python object.
/// Returns 0 when this cannot be determined.
inline size_t len_hint(handle h) {
    ssize_t result = IObject::lengthHint(h.ptr(), 0);
    if (result < 0) {
        // Sometimes a length can't be determined at all (eg generators)
        // In which case simply return 0
        //PyErr_Clear();
        return 0;
    }
    return (size_t) result;
}

inline str repr(handle h) {
//    Object *str_value = Object_Repr(h.ptr());
//    if (!str_value) {
//        throw error_already_set();
//    }
//    return reinterpret_steal<str>(str_value);
    return IObject::repr(h.ptr());
}

inline iterator iter(handle obj) {
    //Object *result = Object_GetIter(obj.ptr());
    Object *result = IObject::getIter(obj.ptr());
    if (!result) {
        throw error_already_set();
    }
    return reinterpret_steal<iterator>(result);
}
/// @} python_builtins

BIND11_NAMESPACE_BEGIN(detail)
template <typename D>
iterator object_api<D>::begin() const {
    return iter(derived());
}
template <typename D>
iterator object_api<D>::end() const {
    return iterator::sentinel();
}
template <typename D>
item_accessor object_api<D>::operator[](handle key) const {
    return {derived(), reinterpret_borrow<object>(key)};
}
template <typename D>
item_accessor object_api<D>::operator[](const char *key) const {
    return {derived(), BIND11::str(key)};
}
template <typename D>
obj_attr_accessor object_api<D>::attr(handle key) const {
    return {derived(), reinterpret_borrow<object>(key)};
}
template <typename D>
str_attr_accessor object_api<D>::attr(const char *key) const {
    return {derived(), key};
}
template <typename D>
args_proxy object_api<D>::operator*() const {
    return args_proxy(derived().ptr());
}
template <typename D>
template <typename T>
bool object_api<D>::contains(T &&item) const {
    return attr("__contains__")(std::forward<T>(item)).template cast<bool>();
}

template <typename D>
BIND11::str object_api<D>::str() const {
    return BIND11::str(derived());
}

template <typename D>
str_attr_accessor object_api<D>::doc() const {
    return attr("__doc__");
}

template <typename D>
handle object_api<D>::get_type() const {
    return type::handle_of(derived());
}

template <typename D>
bool object_api<D>::rich_compare(object_api const &other, int value) const {
    //int rv = Object_RichCompareBool(derived().ptr(), other.derived().ptr(), value);
    int rv = IObject::richCompare(derived().ptr(), other.derived().ptr(), value);
    if (rv == -1) {
        throw error_already_set();
    }
    return rv == 1;
}

#define BIND11_MATH_OPERATOR_UNARY(op, fn)                                                      \
    template <typename D>                                                                         \
    object object_api<D>::op() const {                                                            \
        object result = reinterpret_steal<object>(fn(derived().ptr()));                           \
        if (!result.ptr())                                                                        \
            throw error_already_set();                                                            \
        return result;                                                                            \
    }

#define BIND11_MATH_OPERATOR_BINARY(op, fn)                                                     \
    template <typename D>                                                                         \
    object object_api<D>::op(object_api const &other) const {                                     \
        object result = reinterpret_steal<object>(fn(derived().ptr(), other.derived().ptr()));    \
        if (!result.ptr())                                                                        \
            throw error_already_set();                                                            \
        return result;                                                                            \
    }

BIND11_MATH_OPERATOR_UNARY(operator~, PyNumber_Invert)
BIND11_MATH_OPERATOR_UNARY(operator-, PyNumber_Negative)
BIND11_MATH_OPERATOR_BINARY(operator+, PyNumber_Add)
BIND11_MATH_OPERATOR_BINARY(operator+=, PyNumber_InPlaceAdd)
BIND11_MATH_OPERATOR_BINARY(operator-, PyNumber_Subtract)
BIND11_MATH_OPERATOR_BINARY(operator-=, PyNumber_InPlaceSubtract)
BIND11_MATH_OPERATOR_BINARY(operator*, PyNumber_Multiply)
BIND11_MATH_OPERATOR_BINARY(operator*=, PyNumber_InPlaceMultiply)
BIND11_MATH_OPERATOR_BINARY(operator/, PyNumber_TrueDivide)
BIND11_MATH_OPERATOR_BINARY(operator/=, PyNumber_InPlaceTrueDivide)
BIND11_MATH_OPERATOR_BINARY(operator|, PyNumber_Or)
BIND11_MATH_OPERATOR_BINARY(operator|=, PyNumber_InPlaceOr)
BIND11_MATH_OPERATOR_BINARY(operator&, PyNumber_And)
BIND11_MATH_OPERATOR_BINARY(operator&=, PyNumber_InPlaceAnd)
BIND11_MATH_OPERATOR_BINARY(operator^, PyNumber_Xor)
BIND11_MATH_OPERATOR_BINARY(operator^=, PyNumber_InPlaceXor)
BIND11_MATH_OPERATOR_BINARY(operator<<, PyNumber_Lshift)
BIND11_MATH_OPERATOR_BINARY(operator<<=, PyNumber_InPlaceLshift)
BIND11_MATH_OPERATOR_BINARY(operator>>, PyNumber_Rshift)
BIND11_MATH_OPERATOR_BINARY(operator>>=, PyNumber_InPlaceRshift)

#undef BIND11_MATH_OPERATOR_UNARY
#undef BIND11_MATH_OPERATOR_BINARY

BIND11_NAMESPACE_END(detail)
BIND11_NAMESPACE_END(BIND11_NAMESPACE)
