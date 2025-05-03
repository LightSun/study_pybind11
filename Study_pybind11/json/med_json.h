#pragma once

#include "json/json.hpp"

namespace h7 {

#define DEF_JSON_HEAD(type)\
    namespace json_impl {\
        void to_json(nlohmann::json& j, const type& p);\
        void from_json(const nlohmann::json& j, type& p);\
    }\
    inline void to_json(nlohmann::json& j, const type& p){\
        json_impl::to_json(j, p);\
    }\
    inline void from_json(const nlohmann::json& j, type& p){\
        json_impl::from_json(j, p);\
    }

//...: fields error
//#define DEF_JSON_IMPL(type,...)\
//    namespace json_impl{\
//    void to_json(nlohmann::json& j, const type& p);{\
//        PP_JSON_TO(##__VA_ARGS__);\
//    }\
//    void from_json(const nlohmann::json& j, type& p);{\
//        PP_JSON_FROM(##__VA_ARGS__);\
//    }}

#define VAL_TO_JSON(name)\
     j[#name] = p.name;
#define JSON_OPT_VAL(name)\
     p.name = j.value(#name, p.name)

#define PP_ARG_X(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,\
           a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z, \
           A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,XX,...) XX
#define PP_ARG_N(...) \
        PP_ARG_X("ignored", ##__VA_ARGS__, \
            Z,Y,X,W,V,U,T,S,R,Q,P,O,N,M,L,K,J,I,H,G,F,E,D,C,B,A, \
            z,y,x,w,v,u,t,s,r,q,p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a, \
                                            9,8,7,6,5,4,3,2,1,0)
#define PP_VA_NAME(prefix,...) \
        PP_CAT2(prefix,PP_ARG_N(__VA_ARGS__))
#define PP_CAT2(a,b)      PP_CAT2_1(a,b)
#define PP_CAT2_1(a,b)    a##b

#define PP_MAP_0(m)
#define PP_MAP_1(m,x) m(x);
#define PP_MAP_2(m,x,y) m(x); m(y);
#define PP_MAP_3(m,x,y,z) m(x); m(y); m(z);
#define PP_MAP_4(m,w,x,y,z) m(w); m(x); m(y); m(z);
#define PP_MAP_5(m,v,w,x,y,z) m(v);m(w); m(x); m(y); m(z);
#define PP_MAP_6(m,u,v,w,x,y,z) m(u); m(v);m(w); m(x); m(y); m(z);
#define PP_MAP_7(m,t,u,v,w,x,y,z) m(t); m(u); m(v);m(w); m(x); m(y); m(z);
#define PP_MAP_8(m,s,t,u,v,w,x,y,z) m(s); m(t); m(u); m(v);m(w); m(x); m(y); m(z);
#define PP_MAP_9(m,r,s,t,u,v,w,x,y,z) m(r);m(s);m(t); m(u); m(v);m(w); m(x); m(y); m(z);
#define PP_MAP_10(m,r,s,t,u,v,w,x,y,z,z1) m(r);m(s);m(t); m(u); m(v);m(w); \
    m(x); m(y); m(z); m(z1)
#define PP_MAP_11(m,r,s,t,u,v,w,x,y,z,z1,z2) m(r);m(s);m(t); m(u); m(v);m(w); \
    m(x); m(y); m(z); m(z1); m(z2);
#define PP_MAP_12(m,r,s,t,u,v,w,x,y,z,z1,z2,z3) m(r);m(s);m(t); m(u); m(v);m(w); \
    m(x); m(y); m(z); m(z1); m(z2); m(z3)

#define PP_JSON_OP(op, ...)\
    PP_VA_NAME(PP_MAP_,__VA_ARGS__)(op,##__VA_ARGS__)

#define PP_JSON_TO(...) PP_JSON_OP(VAL_TO_JSON, ##__VA_ARGS__)
#define PP_JSON_FROM(...) PP_JSON_OP(JSON_OPT_VAL, ##__VA_ARGS__)
}
