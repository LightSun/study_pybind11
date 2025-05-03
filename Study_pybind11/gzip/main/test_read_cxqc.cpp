#include "core/src/Prop.h"
#include "core/src/IKeyValue.h"
#include "core/src/string_utils.hpp"
#include "core/src/FileUtils.h"
#include "core/src/StringBuffer.h"
#include "gzip/src/Gzip.h"

using namespace h7_gz;

namespace h7_gz {
struct TestItem2{
    String in_dir;
    String out_dir;
    String out_file;
    String in_file;
    GzipHelper::FUNC_Classify classify;
    bool debug {false};
};
using CTestItem = const TestItem2&;

static std::vector<String> getPtFileNames(CString file);
}

void test_zip_cxqc(int argc, const char* argv[]){
//    String file = "/home/heaven7/heaven7/work/SE/MedQA/MN/us-qc";
//    getPtFileName(file + "/api0207_config.py");
//    return;
    med_qa::Prop prop;
    prop.load(argc, argv);
    h7::KVHelper helper(&prop);
    //
    TestItem2 ti;
    ti.in_dir = helper.getString("in_dir");
    ti.out_dir = helper.getString("out_dir");
    ti.in_file = helper.getString("in_file");
    ti.out_file = helper.getString("out_file");
    //
    auto pt_desc_file = helper.getString("pt_desc_file");
    if(!pt_desc_file.empty()){
        MED_ASSERT(h7::FileUtils::isFileExists(pt_desc_file));
        auto files = getPtFileNames(pt_desc_file);
        String prefix = "/home/ai999/project/us-qc/";
        for(auto& f : files){
            auto realF = prefix + f;
            String outF = ti.out_dir + "/" + f;
            MED_ASSERT_X(h7::FileUtils::isFileExists(realF), realF);
            auto dir = h7::FileUtils::getFileDir(outF);
            h7::FileUtils::mkdirs(dir);
            auto cs = h7::FileUtils::getFileContent(realF);
            h7::FileUtils::writeFile(outF, cs);
        }
        return;
    }
    ti.debug = true;
    ti.classify = [](const std::vector<ZipFileItem>& items, std::vector<GroupItem>& gi){
        std::vector<String> ignoreDirs = {
            "pipeline_history","__pycache__","pipeline_run_log",
            "PyEncDec", "ppocrmy", "rapid_ocr_onnx",
            "rapidoce_测试数据1108",
            ".git", ".idea", ".vscode",
        };
        std::vector<String> ignoreExts = {
            ".log", ".jpg", ".png", ".PNG", ".JPG",
            ".onnx", ".pt", ".whl"
        };
        std::map<String, GroupItem> newItems;
        for(auto& zi : items){
            bool shouldIgnore = false;
            for(auto& d : ignoreDirs){
                String name = "/" + d + "/";
                if(zi.name.find(name) != String::npos){
                    shouldIgnore = true;
                    break;;
                }
            }
            for(auto& ext : ignoreExts){
                if(zi.ext == ext){
                    shouldIgnore = true;
                    break;;
                }
            }
            if(shouldIgnore){
                continue;
            }
            //newItems.push_back(zi);
            int idx = zi.name.rfind(".");
            if(idx < 0){
                fprintf(stderr, "wrong file: '%s'\n", zi.name.data());
                continue;
            }
            auto& gn = newItems[zi.ext];
            if(gn.name.empty()){
                gn.name = zi.ext;
            }
            gn.children.push_back(zi);
        }
        {
            auto it = newItems.begin();
            for(; it != newItems.end(); ++it){
                fprintf(stderr, "ext: '%s'\n", it->first.data());
                gi.push_back(it->second);
            }
        }
    };
    //-----------
    GzipHelper gh;
    gh.setClassifier(ti.classify);
    gh.setDebug(ti.debug);
    if(!ti.in_dir.empty() && !ti.out_file.empty()){
        auto ret = gh.compressDir(ti.in_dir, ti.out_file);
        String str = ret ? "Success" : "Failed";
        printf("compressDir: %s, in_dir = '%s', outF = '%s'\n",
               str.data(), ti.in_dir.data(), ti.out_file.data());
    }else if(!ti.in_file.empty() && !ti.out_dir.empty()){
        auto decRet = gh.decompressFile(ti.in_file, ti.out_dir);
        String decStr = decRet ? "Success" : "Failed";
        printf("decompressFile: %s, in_file = '%s', outDir = '%s'\n",
               decStr.data(), ti.in_file.data(), ti.out_dir.data());
    }else{
        fprintf(stderr, "wrong args:...\n");
        prop.prints();
    }
}
;
std::vector<String> h7_gz::getPtFileNames(CString file){
    auto cs = h7::FileUtils::getFileContent(file);
    h7::StringBuffer buf(cs);
    String line;
    std::vector<String> retList;
    while (buf.readLine(line)) {
        h7::utils::trimLastR(line);
        h7::utils::trim(line);
        if(line.empty()){
            continue;
        }
        String startStr = "default='";
        int idx = line.find(startStr);
        if(idx < 0){
            continue;
        }
        int idx2 = line.rfind("'");
        auto sub = line.substr(idx + startStr.length(),
                               idx2 - (idx + startStr.length()));
        if(h7::utils::startsWith(sub, "/home/ai999/project/us-qc")){
            String str = "/home/ai999/project/us-qc/";
            sub = sub.substr(str.length());
        }
        //printf("sub = @%s@\n", sub.data());
        retList.push_back(sub);
    }
    return retList;
}
