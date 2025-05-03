#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>

namespace h7 {

struct ConfigItem;

using String = std::string;
using CString = const std::string&;
using Map = std::map<String, String>;

struct ConfigItemHolder{
    ConfigItem* ci;
    std::function<void(ConfigItem*)> del;

    ~ConfigItemHolder(){
        if(ci && del){
            del(ci);
            ci = nullptr;
        }
    }
    ConfigItemHolder(){ci = nullptr; del = nullptr;}
    ConfigItemHolder(ConfigItem* ci, std::function<void(ConfigItem*)> del = nullptr):
        ci(ci), del(del){
    }
    ConfigItemHolder(ConfigItemHolder&& ch){
        this->ci = ch.ci;
        this->del = ch.del;
        ch.ci = nullptr;
    }
    ConfigItemHolder(ConfigItemHolder const& _ch){
        auto& ch = const_cast<ConfigItemHolder&>(_ch);
        this->ci = ch.ci;
        this->del = ch.del;
        ch.ci = nullptr;
    }

    ConfigItemHolder& operator=(ConfigItemHolder&& ch){
        this->ci = ch.ci;
        this->del = ch.del;
        ch.ci = nullptr;
        return *this;
    }
    ConfigItemHolder& operator=(ConfigItemHolder const& _ch){
        auto& ch = const_cast<ConfigItemHolder&>(_ch);
        this->ci = ch.ci;
        this->del = ch.del;
        ch.ci = nullptr;
        return *this;
    }
    ConfigItemHolder ref(){
        return ConfigItemHolder(ci);
    }
};

struct ConfigItemCache{
    std::unordered_map<String, ConfigItemHolder> incMap;
    std::unordered_map<String, ConfigItemHolder> superMap;
};

struct IWrappedResolver;

struct IConfigResolver{
    virtual ~IConfigResolver(){}
    virtual ConfigItemHolder resolveInclude(String curDir, CString name, String& errorMsg) = 0;
    virtual ConfigItemHolder resolveSuper(String curDir, CString name, String& errorMsg) = 0;
    virtual String resolveValue(CString name, String& errorMsg) = 0;

    static bool getRealValue(String& val, IConfigResolver* reso, String& errorMsg);
    static std::shared_ptr<IConfigResolver> newTwoConfigResolver(
            IConfigResolver* reso1, IConfigResolver* reso2);
    static std::shared_ptr<IWrappedResolver> newWrappedConfigResolver(
            ConfigItemCache* cache,IConfigResolver* reso1);
    static std::shared_ptr<IConfigResolver> newRuntimeConfigResolver(
            ConfigItemCache* cache,ConfigItem* item, Map* prop);
    static std::shared_ptr<IConfigResolver> newRuntimeConfigResolver(
            ConfigItemCache* cache,ConfigItem* item, const Map& prop);
};

struct IWrappedResolver: public IConfigResolver{

    virtual ~IWrappedResolver(){}

    virtual String resolves(ConfigItem* parItem) = 0;
    virtual String resolveValues(ConfigItem* item) = 0;
};

}

