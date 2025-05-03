#include <string.h>
#include "ZlibUtils.h"
#include "zlib.h"

#define CHECK_ERR(err, msg) { \
if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        return false; \
} \
}
#define CHUNK 16384

namespace h7_gz {

static void error(CString func, const char *msg) {
    fprintf(stderr, "%s: %s\n", func.data(), msg);
    exit(1);
}
struct GZDeleter
{
    constexpr GZDeleter() noexcept = default;

    void operator()(gzFile in) const
    {
        if (gzclose(in) != Z_OK) error("readGZ", "failed gzclose");
    }
};

struct ZlibInputImpl : public IZlibInput{
    String cs;
    bool haveNext_ {true};
    ZlibInputImpl(CString cs):cs(cs){}

    bool hasNext() override{
        return haveNext_;
    };
    //isFinish: true means no more data.
    //return valid len of buffer.
    size_t next(std::vector<char>& vec,bool& isFinish) override{
        isFinish = true;
        vec.resize(cs.length());
        memcpy(vec.data(), cs.data(), cs.length());
        return cs.length();
    }
};

struct ZlibOutputImpl : public IZlibOutput{
    String buffer0;
    String* buffer;

    ZlibOutputImpl(String* b){
        if(b){
            buffer = b;
        }else{
            buffer = &buffer0;
        }
    }

    bool write(std::vector<char>& buf, size_t len) override{
        buffer->append(String(buf.data(), len));
        return true;
    }
};

bool ZlibUtils::compress(IZlibInput* in, IZlibOutput* out){
    // std::shared_ptr<z_stream> sp_strm(&strm,[](z_stream* strm){
    //     (void)deflateEnd(strm);
    // });
    z_stream c_stream;
    int err;
    c_stream.zalloc = nullptr;
    c_stream.zfree = nullptr;
    c_stream.opaque = (voidpf)0;
    c_stream.next_in = nullptr;

    err = deflateInit(&c_stream, Z_BEST_SPEED);
    CHECK_ERR(err, "deflateInit");
    //
    std::vector<char> bufIn;
    std::vector<char> bufOut;
    bool isFinish = false;
    while (!isFinish && in->hasNext()) {
        auto len = in->next(bufIn, isFinish);
        if(bufOut.size() < bufIn.size()){
            bufOut.resize(bufIn.size());
        }
        const int flush = isFinish ? Z_FINISH : Z_NO_FLUSH;
        auto aviOut_len = bufOut.size();
        c_stream.next_in = (Bytef*)bufIn.data();
        c_stream.avail_in = (uInt)len;
        do{
            c_stream.next_out = (Bytef*)bufOut.data();
            c_stream.avail_out = aviOut_len;

            err = deflate(&c_stream, flush);
            if(err != Z_OK && err != Z_STREAM_END){
                fprintf(stderr, "deflate error: %d\n", err);
                deflateEnd(&c_stream);
                return false;
            }
            auto cmpSize = aviOut_len - c_stream.avail_out;
            if(cmpSize > 0 && !out->write(bufOut, cmpSize)){
                deflateEnd(&c_stream);
                return false;
            }
        }while(c_stream.avail_out == 0);
        if(c_stream.avail_in != 0){
            break;
        }
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
    return true;
}

//
bool ZlibUtils::decompress(IZlibInput* in, IZlibOutput* out){
    z_stream c_stream;

    c_stream.zalloc = Z_NULL;
    c_stream.zfree = Z_NULL;
    c_stream.opaque = Z_NULL;
    c_stream.avail_in = 0;
    c_stream.next_in = Z_NULL;
    auto err = inflateInit(&c_stream);
    CHECK_ERR(err, "inflateInit");
    //
    std::vector<char> bufIn;
    std::vector<char> bufOut;
    bool isFinish = false;
    while (!isFinish && in->hasNext()) {
        auto len = in->next(bufIn, isFinish);
        if(bufOut.size() < bufIn.size()){
            bufOut.resize(bufIn.size());
        }
        c_stream.next_in = (Bytef*)bufIn.data();
        c_stream.avail_in = (uInt)len;
        auto aviOut_len = bufOut.size();
        do{
            c_stream.next_out = (Bytef*)bufOut.data();
            c_stream.avail_out = aviOut_len;

            err = inflate(&c_stream, Z_NO_FLUSH);
            switch (err) {
            case Z_NEED_DICT:
                fprintf(stderr, "decompress >> Error Z_NEED_DICT.\n");
                //err = Z_DATA_ERROR; //as data-error
                (void)inflateEnd(&c_stream);
                return false;
            case Z_DATA_ERROR:
                fprintf(stderr, "decompress >> Error Z_DATA_ERROR.\n");
                (void)inflateEnd(&c_stream);
                return false;

            case Z_MEM_ERROR:
                fprintf(stderr, "decompress >> Error Z_MEM_ERROR.\n");
                (void)inflateEnd(&c_stream);
                return false;
            }
            auto cmpSize = aviOut_len - c_stream.avail_out;
            if(cmpSize > 0 && !out->write(bufOut, cmpSize)){
                inflateEnd(&c_stream);
                return false;
            }
        }while(c_stream.avail_out == 0);
    }
    (void)inflateEnd(&c_stream);
    //return err == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
    return err == Z_STREAM_END;
}

bool ZlibUtils::compress(CString str, String& out){
    ZlibInputImpl is(str);
    ZlibOutputImpl os(&out);
    return compress(&is, &os);
}
bool ZlibUtils::decompress(CString str, String& out){
    ZlibInputImpl is(str);
    ZlibOutputImpl os(&out);
    return decompress(&is, &os);
}

// bool readGZ(CString file, CString outDir){
//     std::unique_ptr<gzFile_s, GZDeleter> inPtr(gzopen(file.data(), "rb"));
//     char buf[CHUNK];
//     int len;
//     int err;
//     auto out = stdout;
//     for (;;) {
//         len = gzread(inPtr.get(), buf, sizeof(buf));
//         if (len < 0){
//             error("readGZ", gzerror(inPtr.get(), &err));
//             return false;
//         }
//         if (len == 0) break;

//         if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
//             error("readGZ", "failed fwrite");
//             return false;
//         }
//     }
//     if (fclose(out)) error("readGZ", "failed fclose");
//     return true;
// }

}
