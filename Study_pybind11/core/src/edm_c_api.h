#ifndef EDM_C_API_H
#define EDM_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

struct EDM_buffer{
    char* data {nullptr};
    unsigned int len {0}; //valid_len + 1
};
typedef void* EDM_manager;

EDM_manager edm_manager_create();

void edm_manager_destroy(EDM_manager edm);

void edm_manager_load(EDM_manager edm, const char* encOutDesc);

//1 for success.
int edm_manager_getItem(EDM_manager m,const char* key, EDM_buffer* of);

void edm_manager_freeBufferData(struct EDM_buffer*);

#ifdef __cplusplus
}
#endif


#endif // EDM_C_API_H
