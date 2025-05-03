#include <string.h>
#include "edm_c_api.h"
#include "EDManager.h"

using namespace med_qa;

EDM_manager edm_manager_create(){
    return new EDManager();
}

void edm_manager_destroy(EDM_manager m){
    EDManager* edm = (EDManager*)m;
    delete edm;
}

void edm_manager_load(EDM_manager m, const char* encOutDesc){
    EDManager* edm = (EDManager*)m;
    edm->load(encOutDesc);
}

int edm_manager_getItem(EDM_manager m,const char* key, EDM_buffer* of){
    EDManager* edm = (EDManager*)m;
    auto data = edm->getItem(key);
    if(!data.empty()){
        of->len = data.length() + 1;
        of->data = (char*)malloc(of->len);
        memcpy(of->data, data.data(), data.length());
        of->data[of->len-1] = '\0';
        return 1;
    }
    return 0;
}
void edm_manager_freeBufferData(struct EDM_buffer* buf){
    if(buf->data){
        free(buf->data);
        buf->data = nullptr;
    }
}

