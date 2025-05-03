#pragma once

#include <string>
#include "macro.h"
//#include "../util/h_atomic.hpp"

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)
#ifdef String
undef String
#endif

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
    ~IObject(){}
    void incRef();
    void decRef();

    bool equals(IObject* other);

    static bool isNone(IObject* obj);

    //return 1 for true(expect).
    static int richCompare(const IObject* o1, const IObject* o2, int compareType);
    //from python
    static int lengthHint(IObject* obj, int defVal);
    //array or map
    static int length(IObject* obj);
     //from python
    static String repr(IObject* obj);

    static IObject* iterator(IObject* obj);

    static bool isInstance(IObject* obj, void* typeObj);
    static ssize_t hash(IObject* obj);
    static const char* getClassName(IObject* obj);

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
    static IObject* getFunction(IObject* obj);
    static IObject* dict_getItemString(IObject* obj, const char *key);
    static IObject* dict_getItem(IObject* obj, IObject *key);
};


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
//#define Object_HEAD Object ob_base;

#define Object_HEAD Object* ob_base {nullptr};
#define True_check(a) true

BIND11_NAMESPACE_END(BIND11_NAMESPACE)
