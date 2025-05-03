#pragma once

#include <string>
#include "core/edm_c_api.h"

namespace h7 {
using String = std::string;
using CString = const String&;
class EdmHelper{
public:
    EdmHelper(){
        m_ptr = edm_manager_create();
    }
    ~EdmHelper(){
        if(m_ptr){
            edm_manager_destroy(m_ptr);
            m_ptr = nullptr;
        }
    }
    void loadDir(CString dir){
        String desc = dir + ",record,data";
        loadDesc(desc);
    }
    void loadDesc(CString desc){
        edm_manager_load(m_ptr, desc.data());
    }
    String getItem(CString key){
        EDM_buffer buf;
        if(edm_manager_getItem(m_ptr, key.data(), &buf)){
            String data(buf.data, buf.len-1);
            edm_manager_freeBufferData(&buf);
            return data;
        }
        return "";
    }
    String getItemData(CString key){
        return getItem(key);
    }
private:
    EDM_manager m_ptr{nullptr};
};
}
