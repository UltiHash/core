//
// Created by tariq on 5/10/22.
//

#ifndef SCHOOL_PROJECT_DATA_H
#define SCHOOL_PROJECT_DATA_H

#include "Prefix.h"
#include "Identifier.h"
#include "protocol/client.h"
#include <fstream>

class Data : virtual public Prefix{
    std::vector<std::string> hash_blocks;
    std::size_t numBlocks{};
    const std::unique_ptr<uh::protocol::client>& m_client;

    std::size_t m_size = 0u;

    std::size_t m_encoded_size = 0u;
    std::size_t m_decoded_size = 0u;

private:
    static std::string demo_perms(std::filesystem::perms p){
        std::stringstream out;
        out << ((p & std::filesystem::perms::owner_read) != std::filesystem::perms::none ? "r" : "-")
            << ((p & std::filesystem::perms::owner_write) != std::filesystem::perms::none ? "w" : "-")
            << ((p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ? "x" : "-")
            << ((p & std::filesystem::perms::group_read) != std::filesystem::perms::none ? "r" : "-")
            << ((p & std::filesystem::perms::group_write) != std::filesystem::perms::none ? "w" : "-")
            << ((p & std::filesystem::perms::group_exec) != std::filesystem::perms::none ? "x" : "-")
            << ((p & std::filesystem::perms::others_read) != std::filesystem::perms::none ? "r" : "-")
            << ((p & std::filesystem::perms::others_write) != std::filesystem::perms::none ? "w" : "-")
            << ((p & std::filesystem::perms::others_exec) != std::filesystem::perms::none ? "x" : "-");
        return out.str();
    }

public:
    template<class OutType>
    OutType encodeD() {
        OutType prefix_encode=Prefix::encode<OutType>();

        if(this->is_regular_file() and not this->emptyObject())[[likely]] {
            std::string numBlocks_string=this->write_pod(numBlocks).str();//TODO: also encode Blocks, compress
            std::for_each(numBlocks_string.begin(),numBlocks_string.end(),[&prefix_encode](const unsigned char c){prefix_encode.push_back(c);});
            try {
                INFO << "Sending blocks...";

                std::size_t totalBlockSize= 0;
                long totalNanoSec{};
                long totalMicroSec{};
                long totalMilliSec{};
                long totalSec{};
                std::uint64_t count=0;
                for (auto &i: hash_blocks) {
                    // time measurement statistics for reading blocks from the agency server
                    auto start = std::chrono::steady_clock::now();
                    auto hash = m_client->write_block(std::vector<char>(i.begin(),i.end()));
                    m_encoded_size += i.size();
                    auto end = std::chrono::steady_clock::now();

                    auto nanoSec = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    auto microSec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    auto milliSec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    auto Sec = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
                    auto blockSize = i.size();
                    totalNanoSec+=nanoSec;
                    totalMicroSec+=microSec;
                    totalMilliSec+=milliSec;
                    totalSec+=Sec;
                    totalBlockSize+=blockSize;
                    count++;
                    INFO << "Block-" << count << ": " << "Size " << blockSize << " Bytes - RTT " << nanoSec << " ns, "
                         << microSec << " µs, "
                         << milliSec << " ms, "
                         << Sec << " sec";

                    //write to database, can be optimized here
                    auto id_obj = Identifier(hash);
                    OutType tmpID_encode = id_obj.encode<OutType>();
                    prefix_encode.insert(prefix_encode.cend(), tmpID_encode.begin(), tmpID_encode.end());
                }

                // total block time
                INFO << "Total: " << "Size " << totalBlockSize << " Bytes " << static_cast<float>(totalBlockSize)/static_cast<float>(1000000000) <<  " GB - Total RTT " << totalNanoSec << " ns, "
                     << totalMicroSec << " µs, "
                     << totalMilliSec << " ms, "
                     << totalSec << " sec";

            } catch (const std::exception &exc) {
                ERROR << "Error occurred while sending/encoding the blocks." << exc.what();
                std::exit(EXIT_FAILURE);
            }
        }
        return prefix_encode;
    }

    template<class InType>
    typename InType::iterator decodeD(InType &iss, typename InType::iterator step) {
        step=Prefix::decode(iss, step);

        if(this->is_regular_file() and not this->emptyObject())[[likely]] {
            numBlocks=read_pod<decltype(numBlocks)>(std::istringstream(std::string{step,step+sizeof(numBlocks)}));
            step+=sizeof(numBlocks);

            hash_blocks.clear();
            try {
                INFO << "Reading Blocks...";

                std::size_t totalBlockSize= 0;
                long totalNanoSec{};
                long totalMicroSec{};
                long totalMilliSec{};
                long totalSec{};
                for(std::size_t i=0; i<numBlocks; i++)
                {
                    Identifier id;
                    step=id.decode(iss,step);
                    auto hashID = id.get();
                    std::vector<char> vecHash(hashID.begin(), hashID.end());

                    // time measurement statistics for reading blocks from the agency server
                    auto start = std::chrono::steady_clock::now();

                    auto data = m_client->read_block(vecHash);
                    auto buffer = uh::io::read_to_buffer(*data);
                    m_decoded_size += buffer.size();

                    auto end = std::chrono::steady_clock::now();

                    auto nanoSec = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    auto microSec = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    auto milliSec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    auto Sec = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

                    totalNanoSec+=nanoSec;
                    totalMicroSec+=microSec;
                    totalMilliSec+=milliSec;
                    totalSec+=Sec;
                    totalBlockSize+=buffer.size();
                    INFO << "Block-" << i+1  << ": " << "Size " << buffer.size() << " Bytes - RTT " << nanoSec << " ns, "
                         << microSec << " µs, "
                         << milliSec << " ms, "
                         << Sec << " sec";

                    hash_blocks.emplace_back(buffer.begin(), buffer.end());
                }

                // total block time
                INFO << "Total: " << "Size " << totalBlockSize << " Bytes, " << static_cast<float>(totalBlockSize)/static_cast<float>(1000000000) << " GB - Total RTT " << totalNanoSec << " ns, "
                     << totalMicroSec << " µs, "
                     << totalMilliSec << " ms, "
                     << totalSec << " sec";
            }
            catch (const std::exception &exc) {
                ERROR << "Error occurred while receiving/decoding the hash." << exc.what();
                std::exit(EXIT_FAILURE);
            }
        }

        return step;
    }

    [[nodiscard]] virtual std::size_t getNumBlocks() const;

    [[nodiscard]] bool emptyObject() const override;

    [[nodiscard]] unsigned short folderEnd() const override;

    [[nodiscard]] bool is_regular_file() const override;

    [[nodiscard]] std::size_t size() const { return m_size; }

    std::size_t encoded_size() const { return m_encoded_size; }
    std::size_t decoded_size() const { return m_decoded_size; }

    inline std::string getObjectName();

    template<class InType>
    std::tuple<std::filesystem::path,typename InType::iterator>
    write_from_stream_vector(InType &iss,std::tuple<std::filesystem::path,typename InType::iterator>stepper) {
        std::get<1>(stepper)=decodeD(iss,std::get<1>(stepper));
        std::get<0>(stepper)=std::filesystem::path(std::get<0>(stepper).string()+"/"+object_name).make_preferred();

        std::cout << "Read from recompilation file: " << std::get<0>(stepper) <<  "\n\n";
        bool is_file=false;
        if(not is_regular_file()){
            if(not std::filesystem::exists(std::get<0>(stepper)))[[likely]]{
                std::filesystem::create_directory(std::get<0>(stepper));
            }
        }
        else{
            is_file=true;
            if(not std::filesystem::exists(std::get<0>(stepper)) and not emptyObject())[[likely]]{
                std::ofstream of;
                of.open(std::get<0>(stepper), std::ios::out | std::ios::app );
                if(not of.is_open()){
                    ERROR << "Could not open and write to file path " << std::get<0>(stepper) << " " << std::endl;
                    std::exit(EXIT_FAILURE);
                }
                for(const auto &b:hash_blocks){
                    of<<b;
                }
                of.flush();
                of.close();
            }
        }
        std::filesystem::perms test_perms=std::filesystem::status(std::get<0>(stepper)).permissions();
        if(test_perms!=pe)[[unlikely]]{
            //updated permissions on request if the user has permission to do so
            std::string permission_text = "The permission of the written object "+
                    std::get<0>(stepper).string()+" is now different from the permission of the recompilation file!\n"
                                         "Written down permission settings: "+
                                         demo_perms(test_perms)+
                                         "\nOriginal object perms: "+
                    demo_perms(pe)+
                    "\nMake sure you have permission to influence the settings of the data object!\n";
            WARNING << permission_text;
            std::cout << permission_text;
            std::string input_text;

            while(not (input_text=="Y" or input_text =="y" or input_text=="N" or input_text =="n")){
                std::cout << "Do you want to update the permissions of this data object to the original settings? (Y/N)"<<std::endl;
                std::getline(std::cin,input_text);
            }
            if(input_text=="Y" or input_text=="y")[[unlikely]]{
                try{
                    std::filesystem::permissions(std::get<0>(stepper),pe);
                }
                catch(std::exception& e){
                    std::string perm_fail="The permission of the data object was not set due to this reason: ";
                    perm_fail+=e.what();
                    perm_fail+="\n";
                    ERROR<<perm_fail;
                    std::cerr<<perm_fail;
                }
            }
        }

        lutimes(std::get<0>(stepper).c_str(), (const struct timeval *)&newTimes);//update atime and mtime from Prefix decode
        //on every file get parent path after write onto stepper
        for(unsigned short i=0; static_cast<unsigned int>(i)<this->folderEnd()+static_cast<unsigned int>(is_file);i++){
            std::get<0>(stepper)=std::filesystem::path(std::get<0>(stepper).parent_path()).make_preferred();
        }

        return stepper;
    }

    Data(const std::string &in, unsigned short folderE, const std::unique_ptr<uh::protocol::client>& client);
    explicit Data(const std::unique_ptr<uh::protocol::client>& client);
};
//BOOST_CLASS_VERSION(Data, 1)
#endif //SCHOOL_PROJECT_DATA_H
