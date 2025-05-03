#include "IObject.h"

BIND11_NAMESPACE_BEGIN(BIND11_NAMESPACE)

void IObject::incRef(){

}
void IObject::decRef(){

}

bool IObject::equals(IObject* other){
    return false;
}
//-----------------------------------
bool IObject::isNone(IObject* obj){

}
int IObject::richCompare(const IObject* o1, const IObject* o2, int compareType){

}
int IObject::lengthHint(IObject* obj, int defVal){

}
int IObject::length(IObject* obj){

}
String IObject::repr(IObject* obj){

}

bool IObject::isInstance(IObject* obj, void* typeObj){
}
ssize_t IObject::hash(IObject* obj){

}

IObject* IObject::getItem(IObject* obj, IObject* key){

}
bool IObject::setItem(IObject* obj, IObject* key, IObject* value){

}

bool IObject::hasAttr(IObject* obj, IObject* attr){

}
bool IObject::hasAttrString(IObject* obj, const char* name){

}
bool IObject::delAttr(IObject* obj, IObject* attr){

}
bool IObject::delAttrString(IObject* obj, const char* name){

}
IObject* IObject::getAttr(IObject* obj, IObject* attr){

}
IObject* IObject::getAttrString(IObject* obj, const char* name){

}
bool IObject::setAttr(IObject* obj, IObject* attr, IObject* val){

}
bool IObject::setAttrString(IObject* obj, const char* name, IObject* val){

}
IObject* IObject::getFunction(IObject* obj){

}
IObject* IObject::dict_getItemString(IObject* obj, const char *key){

}
IObject* IObject::dict_getItem(IObject* obj, IObject *key){

}

BIND11_NAMESPACE_END(BIND11_NAMESPACE)
