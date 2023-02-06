//
// Created by tariq on 5/10/22.
//

#include "Prefix.h"

bool Prefix::emptyObject() const {
    unsigned short tmp = status_bytes&0b0001000000000000;
    return static_cast<bool>(tmp >> 12);
}

unsigned short Prefix::folderEnd() const {
    //maximum Path length in linux is 4096 so the maximum depth is half of that because of writing '/', this fits on 12 bit
    return static_cast<unsigned short>(status_bytes&0b0000111111111111);
}

bool Prefix::is_regular_file() const {
    return ft==std::filesystem::file_type::regular;
}

Prefix::Prefix(const std::string &in, unsigned short folderE) {//data to info
    status_bytes=std::filesystem::is_empty(in);
    status_bytes<<=12;
    status_bytes+=folderE;
    std::filesystem::file_status fss=std::filesystem::status(in);
    ft=fss.type();
    pe=fss.permissions();
    std::filesystem::path tmpPath(std::filesystem::canonical(std::filesystem::path(in).make_preferred()));
    object_name=tmpPath.filename().string();

#if defined(BSD)
    if(stat(tmpPath.c_str(), &advancedFileInfo)) {
#else
    if(stat64(tmpPath.c_str(), &advancedFileInfo)) {
#endif
        DEBUG << "The file stat64 of \"" << tmpPath << "\" could not be read!" << std::endl;
    }
}
