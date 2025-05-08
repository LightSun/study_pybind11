#pragma once

#include <string>
#include "macro.h"
//#include "../util/h_atomic.hpp"

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)
#ifdef String
undef String
#endif

#ifdef Py_ssize_t
    using ssize_t = Py_ssize_t;
#else
using ssize_t = unsigned long long;
#endif
using size_t = std::size_t;

using String = std::string;
using CString = const std::string&;

typedef struct TypeObject TypeObject;
typedef struct IObject Object;

enum class CompareType{
    kEQ,kNE,
    kGE,kGT,
    kLT,kLE
};


struct IObject{
    _Object_HEAD_EXTRA
    volatile int ob_refcnt {1};
    TypeObject *ob_type {nullptr};
    //void* obj {nullptr};

    ~IObject(){}
    void incRef();
    void decRef();

    bool equals(IObject* other);

    static bool isNone(IObject* obj);
    //like PyEllipsis
    static bool isEllipsis(IObject* obj);
    static bool isInstance(IObject* obj, void* typeObj);
    static bool isStaticMethod(IObject* obj);

    static TypeObject* type(IObject* obj);
    static IObject* getNone();
    static IObject* getEllipsis();
    static IObject* getTrue();
    static IObject* getFalse();

    static Object* str(IObject* obj);  //Object_str
    static ssize_t hash(IObject* obj);
    static const char* getClassName(IObject* obj);

    //return 1 for true(expect).
    static int richCompare(const IObject* o1, const IObject* o2, int compareType);
    //from python
    static int lengthHint(IObject* obj, int defVal);
    //array or map
    static int length(IObject* obj);
     //from python
    static String repr(IObject* obj);

    static IObject* getItem(IObject* obj, IObject* key);
    static bool setItem(IObject* obj, IObject* key, IObject* value);

    static bool hasAttr(IObject* obj, IObject* attr);
    static bool hasAttrString(IObject* obj, const char* name);
    static bool delAttr(IObject* obj, IObject* attr);
    static bool delAttrString(IObject* obj, const char* name);
    static IObject* getAttr(IObject* obj, IObject* attr);
    static IObject* getAttrString(IObject* obj, const char* name);
    static bool setAttr(IObject* obj, IObject* attr, IObject* val);
    static bool setAttrString(IObject* obj, const char* name, IObject* val);

    static IObject* dict_getItemString(IObject* obj, const char *key);
    static IObject* dict_getItem(IObject* obj, IObject *key);
    static bool dict_Next(IObject* obj, int* pos, IObject** key, IObject** value);

    static IObject* getFunction(IObject* obj);
    static IObject** sequence_Fast_ITEMS(IObject* obj);

    static IObject* getIter(IObject* obj); //iterator
    static IObject* iter_Next(IObject* obj);


};

void hError_clear();
int hError_Occurred(); //PyErr_Occurred

int hUnicode_Check(IObject* obj);  //PyUnicode_Check
Object* hUnicode_FromStringAndSize(const char* str, ssize_t len);
Object* hUnicode_FromString(const char* str);
Object* hUnicode_AsUTF8String(Object*);

//get char data
int hBytes_AsStringAndSize(Object* src, char** buf, ssize_t* len);
Object* hBytes_FromStringAndSize(const char* buf, ssize_t len);
int hBytes_Check(Object* src);          //PyBytes_Check
Object* hBytes_FromString(const char*); //PyBytes_FromString

int hByteArray_Check(Object*);
Object* hByteArray_FromObject(Object*);
Object* hByteArray_FromStringAndSize(const char* buf, ssize_t len);
int hByteArray_Size(Object*);
/**
        char *buffer = PyByteArray_AS_STRING(m_ptr);
        ssize_t size = PyByteArray_GET_SIZE(m_ptr);
        return std::string(buffer, static_cast<size_t>(size));
 */
std::string hByteArray_to_rawStr(Object*);


int hBool_Check(Object*);
int hObject_IsTrue(Object*);
long long hLong_AsLong(Object*);
unsigned long hLong_AsUnsignedLong(Object*);
unsigned long long hLong_AsUnsignedLongLong(Object*);

//struct Object {
//    _Object_HEAD_EXTRA
//    volatile int ob_refcnt {1};
//    TypeObject *ob_type {nullptr};

//    void incRef(){
//        h7_atomic::h_atomic_add(&ob_refcnt, 1);
//    }
//    void decRef(){
//        if(h7_atomic::h_atomic_add(&ob_refcnt, -1) == 1){
//            delete this;
//        }
//    }
//};
#define Object_HEAD Object ob_base;

#define True_check(a) true

BIND11_NAMESPACE_END(BIND11_NAMESPACE)
