#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "core/src/Value.hpp"
#include "core/src/string_utils.hpp"

#define __H7_KeyValue_G_LIST(func)\
    std::vector<String> vec;\
    getVector(key, vec);\
    out.resize(vec.size());\
    for(int i = 0 ; i < (int)vec.size() ; ++i){\
        out[i] = Value(vec[i]).func(def);\
    }

#define __H7_KeyValue_G_LIST0(func)\
    std::vector<String> vec;\
    getVector(key, vec);\
    out.resize(vec.size());\
    for(int i = 0 ; i < (int)vec.size() ; ++i){\
        out[i] = Value(vec[i]).func();\
    }

namespace h7 {

using String = std::string;
using CString = const std::string&;

struct IKeyValue{

    virtual ~IKeyValue(){};

    virtual String getString(const String& key, const String& def = "") = 0;
    virtual void putString(const String& key, const String& val) = 0;
    virtual std::set<String> keys() = 0;
    virtual void print(CString prefix) = 0;
};

//key value helper.
class KVHelper{
public:
    KVHelper(IKeyValue* kv):kv_(kv){}

    void print(CString prefix){
        kv_->print(prefix);
    }
    String getProperty(CString key){
       return kv_->getString(key);
   }
   bool hasProperty(CString key){
       return !getProperty(key).empty();
   }
   String getString(CString key, CString defVal = ""){
       return kv_->getString(key, defVal);
   }
   void putString(const String& key, const String& val){
       kv_->putString(key, val);
   }
   //with replace
   String getStringRep(CString key, CString defVal = ""){
       auto str = getString("REPLACE::" + key);
       if(str.empty()){
           return getString(key, defVal);
       }
       return str;
   }
   bool getBool(CString key, bool def = false){
       Value v(getString(key));
       return v.getBool(def);
   }
   int getInt(CString key,int defVal = 0){
       Value v(getString(key));
       return v.getInt(defVal);
   }
   long long getLong(CString key, long long defVal = 0){
       Value v(getString(key));
       return v.getLong(defVal);
   }
   unsigned int getUInt(CString key, unsigned int defVal = 0){
       Value v(getString(key));
       return v.getUInt(defVal);
   }
   float getFloat(CString key, float defVal = 0){
       Value v(getString(key));
       return v.getFloat(defVal);
   }
   double getDouble(CString key, double defVal = 0){
       Value v(getString(key));
       return v.getDouble(defVal);
   }

   void getVectorInt(CString key,std::vector<int>& out, int def = 0){
       __H7_KeyValue_G_LIST(getInt)
   }
   void getVectorUInt(CString key,std::vector<unsigned int>& out,
                        unsigned int def = 0){
       __H7_KeyValue_G_LIST(getUInt)
   }
   void getVectorLong(CString key,std::vector<long long>& out,
                        long long def = 0){
       __H7_KeyValue_G_LIST(getLong)
   }
   void getVectorULong(CString key,std::vector<unsigned long long>& out,
                        unsigned long long def = 0){
       __H7_KeyValue_G_LIST(getULong)
   }
   void getVectorFloat(CString key,std::vector<float>& out, float def = 0){
       __H7_KeyValue_G_LIST(getFloat)
   }
   void getVectorDouble(CString key,std::vector<double>& out, double def = 0){
       __H7_KeyValue_G_LIST(getDouble)
   }
   void getVectorBool(CString key,std::vector<bool>& out){
       __H7_KeyValue_G_LIST0(getBool)
   }

public:
   void getMap(CString key,std::map<String, String>& out){
        //x=a:b,c:d
        std::vector<String> vec;
        getVector(key, vec);
        for(int i = 0 ; i < (int)vec.size() ; ++i){
            auto& line = vec[i];
            if(line.data()[0] == '{'){
                line = line.substr(1, line.length() - 2);
            }
            int pos = line.find(":");
            if(pos > 0){
                auto key = line.substr(0, pos);
                auto value = line.substr(pos + 1);
                h7::utils::trim(key);
                h7::utils::trim(value);
                out[key] = value;
            }else{
                fprintf(stderr, "getMap >> error line: %s\n", line.data());
                abort();
            }
        }
   }
   void getVector(CString key,std::vector<String>& out){
       String str = getString(key);
       if(!str.empty()){
           out = h7::utils::split(",", str);
       }else{
           //check assign array len
           int len = getInt(key+ "-arr-len", 0);
           if(len > 0){
               out.reserve(len);
               for(int i = 0 ; i < len ; ++i){
                   out.push_back(getProperty(key + "[" + std::to_string(i) + "]"));
               }
               return;
           }
           //check sub arr
           String subs = getProperty(key + "-sub-arrs");
           if(!subs.empty()){
                auto list = h7::utils::split(",", subs);
                for(String& str: list){
                   getVector(str, out);
                }
                return;
           }
           //handle index format
           for(int i = 0 ; ; ++i){
               String prop = getProperty(key + "[" + std::to_string(i) + "]");
               if(prop.empty()){
                   break;
               }
               out.push_back(prop);
           }
       }
   }
private:
    IKeyValue* kv_;
};
}
