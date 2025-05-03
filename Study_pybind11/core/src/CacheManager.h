#pragma once

#include <vector>
#include <string>

namespace h7 {

using uint32 = unsigned int;
using uint64 = unsigned long long;
using String = std::string;
using CString = const std::string&;

    class CacheManager{
    public:
        enum{
          kFlag_COMPRESSED = 0x1
        };
       // using CString = const std::string&;
        CacheManager(uint64 maxFragSize):m_maxFragSize(maxFragSize){
        }
        void addItem(const std::string& name, const std::string& data){
            addItem(name, data.data(), data.length());
        }
        /**
         * @brief addItem: add item data
         * @param name : the name of this item, for latter call 'getItemData' or 'getItemDataUnCompressed'
         * @param data : the item data
         * @param len : the item data length
         * @param flags : the flags. default 0
         */
        void addItem(const std::string& name, const char* data, uint64 len,
                     uint32 flags = 0);

        void addFileItem(const std::string& name, const std::string& filePath);

        void addItemCompressed(const std::string& name, const char* data, uint64 len);

        void addItemCompressed(const std::string& name, const std::string& data){
            addItemCompressed(name, data.data(), data.length());
        }
        void addFileItemCompressed(const std::string& name, const std::string& filePath);

        void getItemData(const std::string& name, std::string& out);

        void getItemDataUnCompressed(const std::string& name, std::string& out);

        void getItemAt(unsigned int index, std::string& o_name, std::string& o_data);

        void removeItem(const std::string& name);

        uint32 getItemCount();
        std::vector<String> getItemNames();

        void saveTo(const std::string& dir,const std::string& recordName,
                    const std::string& dataName);

        void compressTo(const std::string& dir,const std::string& recordName,
                        const std::string& dataName);

        bool load(const std::string& dir,const std::string& recordName,
                       const std::string& dataName);

        void reset(){
            m_data.clear();
            m_items.clear();
        }
    private:
#define __CACHE_NAME_SIZE 124
        struct Item{
            char name[__CACHE_NAME_SIZE];
            uint32 flags {0};
            std::vector<uint32> frag_ids;
            uint64 frag_offset; //the first frag offset.
            uint64 raw_size;
        };
        uint64 m_maxFragSize;
        std::vector<std::vector<char>> m_data; //Fragmentation
        std::vector<Item> m_items;

        uint64 getLastFragUsedSize(){
            return !m_data.empty() ? m_data[m_data.size()-1].size() :0;
        }
        std::vector<char>& getLastBlock(uint64 newSize){
            auto& block = m_data[m_data.size()-1];
            block.resize(newSize);
            return block;
        }
        uint32 getLastBlockId(){
            return m_data.size() - 1;
        }
        std::vector<char>& newBlock(uint64 size){
            m_data.resize(m_data.size() + 1);
            std::vector<char>& block = m_data[m_data.size()-1];
            block.resize(size);
            return block;
        }

        int getItemByName(const std::string& name){
            for(int i = 0 ; i < (int)m_items.size() ; i++){
                if(name == m_items[i].name){
                    return i;
                }
            }
            return -1;
        }
        inline uint64 computeRecordSize();
        inline void writeRecordFile(const std::string& file, bool compressed);
        inline void getItemData0(int _idx, std::string& out);
    };
}
