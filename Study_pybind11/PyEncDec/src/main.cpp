#include "pybind11/numpy.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eval.h>

#include <type_traits>
#include <random>
#include "gzip/src/Gzip.h"
#include "AES.h"
#include "../com/com.h"
#include "core/src/FileUtils.h"

namespace h7 {

namespace py = pybind11;
using namespace py::literals;

using String = std::string;
using CString = const std::string&;

//*指针-->numpy 1D
template<typename T>
py::array_t<T> _ptr_to_arrays_1d(const T* data, py::ssize_t col) {
    auto result = py::array_t<T>(col);//申请空间
    py::buffer_info buf = result.request();
    T* ptr = (T*)buf.ptr;

    for (auto i = 0; i < col; i++)
        ptr[i] = data[i];

    return result;
}
static std::string gen_random_str(int length) {
    std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.size() - 1);

    for (int i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    return result;
}

class PyEncDec{
public:
    PyEncDec(){
        String name = "kmed_qa_" + gen_random_str(8);
        m_tmpDir = "/tmp/" + name;
    }
    ~PyEncDec(){
        //printf("~PyEncDec: '%s'\n", m_tmpDir.data());
        removeCache();
    }
    void removeCache(){
        FileUtils::removeDirectory(m_tmpDir);
    }
    py::list decMulFiles(CString inFile){
        h7_gz::GzipHelper gh;
        gh.setUseSimpleEncDec();
        if(gh.decompressFile(inFile, m_tmpDir)){
            auto files = FileUtils::getFiles(m_tmpDir);
            py::list plist;
            for(auto& f : files){
                plist.append(py::bytes(f));
            }
            return plist;
        }else{
            fprintf(stderr, "dec failed.\n");
            return py::list();
        }
    }
    py::bytes decFile(CString inFile){
        String key1 = "@(**^$$%&)((&^^(";
        String key2 = "8g77d7fg7&%%(*^^";
        auto cs = EncDecPy::getFileContent0(inFile);
        if(cs.empty()){
            fprintf(stderr, "open input failed: %s\n", inFile.data());
            return {};
        }
        {
            med_ed::AES aes(8, m_blockSize);
            auto decCs = aes.cfb128(cs, key1, key2, false);
            if(decCs.empty()){
                fprintf(stderr, "dec failed, file = %s\n", inFile.data());
            }
//            std::vector<char> buffer;
//            buffer.resize(decCs.length());
//            memcpy(buffer.data(), decCs.data(), decCs.length());
//            return buffer;
            return py::bytes(decCs);
        }
    }
    void writeFile(CString f1, std::vector<char>& cs){
        std::ofstream fos;
        fos.open(f1, std::ios::binary);
        if(!fos.is_open()){
            fprintf(stderr, "open output failed: %s\n", f1.data());
            return;
        }
        fos.write(cs.data(), cs.size());
        fos.close();
    }
    void setBlockSize(size_t size){
        if(size > 1024){
            m_blockSize = size;
        }else{
            fprintf(stderr, "setBlockSize >> wrong arg = %lu\n", size);
        }
    }
    void setBlockSizeDesc(CString sizeDesc){
        size_t blockSize = 0;
        String sizeStr = sizeDesc.substr(0, sizeDesc.length()-1);
        if(sizeDesc.length() >= 2){
            size_t size = std::stoul(sizeStr);
            char lastChar = sizeDesc.data()[sizeDesc.size()-1];
            if(lastChar == 'M' || lastChar == 'm'){
                blockSize = size << 20;
            }else if(lastChar == 'G' || lastChar == 'g'){
                blockSize = size << 30;
            }else{
                fprintf(stderr, "wrong sizeDesc: '%s'\n", sizeDesc.data());
            }
        }else{
            fprintf(stderr, "wrong sizeDesc: '%s'\n", sizeDesc.data());
        }
        if(blockSize > 0){
            m_blockSize = blockSize;
        }
    }
    //---------------

private:
    size_t m_blockSize {2 << 20};
    String m_tmpDir;
};

void BindImpl(pybind11::module& m);

PYBIND11_MODULE(PyEncDec, m) {
  m.doc() =
      "Make programer easier to use dec-file";
  BindImpl(m);
}

void BindImpl(pybind11::module& m){
    pybind11::class_<h7::PyEncDec>(m, "PyEncDec")
            .def(pybind11::init<>())
            .def("decFile",[](PyEncDec& self, CString f1){
                return self.decFile(f1);
            })
            .def("writeFile",[](PyEncDec& self, CString f1, std::vector<char>& cs){
                self.writeFile(f1, cs);
            })
            .def("setBlockSize",[](PyEncDec& self, size_t size){
                self.setBlockSize(size);
            })
            .def("setBlockSizeDesc",[](PyEncDec& self, CString desc){
                self.setBlockSizeDesc(desc);
            })
            .def("decMulFiles",[](PyEncDec& self, CString f1){
                return self.decMulFiles(f1);
            })
            .def("removeCache",[](PyEncDec& self){
                 self.removeCache();
            })
        ;
}

}
