//
// Created by tariq on 5/10/22.
//

#ifndef SCHOOL_PROJECT_PREFIX_H
#define SCHOOL_PROJECT_PREFIX_H

#include "EnDecoder.h"

class Prefix{
protected:
    unsigned short status_bytes{};
public:
    std::filesystem::file_type ft{};
    std::filesystem::perms pe{};
    std::string object_name;
    struct stat64 advancedFileInfo{};
    struct timeval newTimes[2]{};
protected:

    template<typename T>
    std::ostringstream write_pod(T &t) {
        std::ostringstream out;
        out.write(reinterpret_cast<char*>(&t), sizeof(T));
        return out;
    }

    template<typename T>
    T read_pod(std::istringstream in) {
        T t{};
        in.read(reinterpret_cast<char*>(&t), sizeof(T));
        return t;
    }

public:

    template<class OutType>
    OutType encode() {
        OutType oss;
        //TODO: compress variable space since size is known
        std::string status_bytes_string=write_pod(status_bytes).str();
        std::for_each(status_bytes_string.begin(),status_bytes_string.end(),[&oss](char c){oss.push_back(c);});

        std::string file_type_string=write_pod(ft).str();
        std::for_each(file_type_string.begin(),file_type_string.end(),[&oss](char c){oss.push_back(c);});

        std::string permission_string=write_pod(pe).str();
        std::for_each(permission_string.begin(),permission_string.end(),[&oss](char c){oss.push_back(c);});

        EnDecoder coder{};
        SequentialContainer auto obj_name_vec=coder.encode<OutType>(object_name);
        oss.insert(oss.cend(),obj_name_vec.begin(),obj_name_vec.end());

        /*
        advancedFileInfoString = write_pod(advancedFileInfo).str();
        obj_name_vec=coder.encode<OutType>(advancedFileInfoString);
        oss.insert(oss.cend(),obj_name_vec.begin(),obj_name_vec.end());*/

        std::string aSec,aNSec,mSec,mNSec;

        aSec = write_pod(advancedFileInfo.st_atim.tv_sec).str();
        std::for_each(aSec.begin(),aSec.end(),[&oss](char c){oss.push_back(c);});

        aNSec = write_pod(advancedFileInfo.st_atim.tv_nsec).str();
        std::for_each(aNSec.begin(),aNSec.end(),[&oss](char c){oss.push_back(c);});

        mSec = write_pod(advancedFileInfo.st_mtim.tv_sec).str();
        std::for_each(mSec.begin(),mSec.end(),[&oss](char c){oss.push_back(c);});

        mNSec = write_pod(advancedFileInfo.st_mtim.tv_nsec).str();
        std::for_each(mNSec.begin(),mNSec.end(),[&oss](char c){oss.push_back(c);});

        newTimes[0].tv_sec = advancedFileInfo.st_atim.tv_sec;
        newTimes[0].tv_usec = advancedFileInfo.st_atim.tv_nsec;
        newTimes[1].tv_sec = advancedFileInfo.st_mtim.tv_sec;
        newTimes[1].tv_usec = advancedFileInfo.st_mtim.tv_nsec;

        EnDecoder coder2{};
        return coder2.encode<OutType>(oss);//TODO: encoder later on also contains parity checksum
    }

    template<class OutType>
    typename OutType::iterator
    decode(OutType &iss, typename OutType::iterator step) {
        EnDecoder coder{};
        auto internal=coder.decoder<OutType>(iss,step);
        step=std::get<1>(internal);

        auto beg=std::get<0>(internal).begin();
        status_bytes = read_pod<decltype(status_bytes)>(std::istringstream(std::string{beg,beg+sizeof(status_bytes)}));
        beg+=sizeof(status_bytes);

        ft = read_pod<decltype(ft)>(std::istringstream(std::string{beg,beg+sizeof(ft)}));
        beg+=sizeof(ft);

        pe = read_pod<decltype(pe)>(std::istringstream(std::string{beg,beg+sizeof(pe)}));
        beg+=sizeof(pe);

        EnDecoder coder2,coder3;
        auto obj_name_tup = coder2.decoder<std::string>(std::get<0>(internal),beg);
        object_name=std::get<0>(obj_name_tup);
        beg=std::get<1>(obj_name_tup);
        //read back mtime and atime

        //atime
        newTimes[0].tv_sec = read_pod<decltype(newTimes[0].tv_sec)>(std::istringstream(std::string{beg,beg+sizeof(newTimes[0].tv_sec)}));
        beg+=sizeof(newTimes[0].tv_sec);
        newTimes[0].tv_usec = read_pod<decltype(newTimes[0].tv_usec)>(std::istringstream(std::string{beg,beg+sizeof(newTimes[0].tv_usec)}));
        beg+=sizeof(newTimes[0].tv_usec);
        //mtime
        newTimes[1].tv_sec = read_pod<decltype(newTimes[1].tv_sec)>(std::istringstream(std::string{beg,beg+sizeof(newTimes[1].tv_sec)}));
        beg+=sizeof(newTimes[1].tv_sec);
        newTimes[1].tv_usec = read_pod<decltype(newTimes[0].tv_usec)>(std::istringstream(std::string{beg,beg+sizeof(newTimes[0].tv_usec)}));
        //beg+=sizeof(newTimes[1].tv_usec);

        return step;
    }

    [[nodiscard]] virtual bool emptyObject() const;

    [[nodiscard]] virtual unsigned short folderEnd() const;

    [[nodiscard]] virtual bool is_regular_file() const;

    Prefix(const std::string &in, unsigned short folderE);

    Prefix()=default;
};

//BOOST_CLASS_VERSION(Prefix, 1)
#endif //SCHOOL_PROJECT_PREFIX_H
