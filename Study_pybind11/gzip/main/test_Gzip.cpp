#include "gzip/src/Gzip.h"

using namespace h7_gz;

namespace h7_gz {
struct TestItem{
    String in_dir;
    String out_dir;
    String out_file;
    GzipHelper::FUNC_Classify classify;
    bool debug {false};
};
using CTestItem = const TestItem&;
}

static void test_Gzip_impl(CTestItem);
static void test_Gzip11();
static void test_Gzip12();

void test_Gzip1(){
    //test_Gzip11();
    test_Gzip12();
}

void test_Gzip12(){
    TestItem ti;
    ti.in_dir = "/media/heaven7/Elements_SE/temp/test2";
    ti.out_dir = "/media/heaven7/Elements_SE/temp/test2_out";
    ti.out_file = "/media/heaven7/Elements_SE/temp/test2.hzip";
    ti.debug = true;
    ti.classify = [](const std::vector<ZipFileItem>& items, std::vector<GroupItem>& gi){
        GroupItem ginput;
        ginput.children = items;
        {
        auto item = ginput.filter("avi", true);
        if(!item.isEmpty()){
            gi.push_back(std::move(item));
        }
        }
        {
        auto item = ginput.filter("onnx_te_te", true);
        if(!item.isEmpty()){
            gi.push_back(std::move(item));
        }
        }
        if(!ginput.isEmpty()){
            gi.push_back(std::move(ginput));
        }
    };
    test_Gzip_impl(ti);
}

void test_Gzip11(){
    TestItem ti;
    ti.in_dir = "/media/heaven7/Elements_SE/temp/test1";
    ti.out_dir = "/media/heaven7/Elements_SE/temp/test1_out";
    ti.out_file = "/media/heaven7/Elements_SE/temp/test1.med";
    test_Gzip_impl(ti);
}

void test_Gzip_impl(CTestItem ti){
    String in_dir = ti.in_dir;
    String out_dir = ti.out_dir;
    String out_file = ti.out_file;
    GzipHelper gh;
    gh.setClassifier(ti.classify);
    gh.setDebug(ti.debug);
    {
        auto ret = gh.compressDir(in_dir, out_file);
        String str = ret ? "Success" : "Failed";
        printf("compressFile: %s, in_dir = '%s', outF = '%s'\n",
               str.data(), in_dir.data(), out_file.data());
        if(!ret){
            return;
        }
    }
    //
    auto decRet = gh.decompressFile(out_file, out_dir);
    String decStr = decRet ? "Success" : "Failed";
    printf("decompressFile: %s,in_dir = '%s', outF = '%s'\n",
           decStr.data(), in_dir.data(), out_file.data());
}
