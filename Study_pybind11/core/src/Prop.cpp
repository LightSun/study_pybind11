#include "Prop.h"
#include "ArgsParser.h"
#include "ConfigUtils.h"
#include "FileUtils.h"
#include "Properties.hpp"

using namespace med_qa;

namespace med_qa{
struct _PropCtx{
    h7::Properties prop;
};
}

Prop::Prop()
{
    m_ctx = new _PropCtx();
}

Prop::~Prop()
{
    if(m_ctx){
        delete m_ctx;
        m_ctx = nullptr;
    }
}
void Prop::prints(){
    auto& tmap = m_ctx->prop.m_map;
    auto it = tmap.begin();
    while (it != tmap.end()) {
        printf("[kv]: %s=%s\n", it->first.data(), it->second.data());
        it ++;
    }
}

void Prop::load(CString propFile){
    h7::ConfigUtils::loadProperties(propFile, m_ctx->prop.m_map);
    String dir = h7::FileUtils::getFileDir(propFile);
    h7::ConfigUtils::resolveProperties({dir}, m_ctx->prop.m_map);
}
void Prop::load(int argc, const char* argv[]){
    h7::ArgsParser ap(argc, (char**)argv);
    m_ctx->prop = ap.asProperty();
}
h7::Properties* Prop::getRawProp(){
    return &m_ctx->prop;
}
void Prop::copyFrom(std::map<String,String>& src){
    m_ctx->prop.m_map = src;
}
String Prop::getString(CString key, CString def){
    return m_ctx->prop.getString(key, def);
}
void Prop::print(CString prefix){
    m_ctx->prop.print(prefix);
}
void Prop::putString(CString key, CString val){
    m_ctx->prop.setProperty(key, val);
}
std::set<String> Prop::keys(){
    return m_ctx->prop.keys();
}
