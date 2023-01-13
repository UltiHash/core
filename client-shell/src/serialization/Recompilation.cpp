//
// Created by tariq on 5/11/22.
//

#ifndef SERIALIZATION_RECOMPILATION_H
#define SERIALIZATION_RECOMPILATION_H
#include "Recompilation.h"

bool Recompilation::encode() {
    std::cout << "Start encoding..." << std::endl;
    for(const auto&c:input_commands){
        this->open(std::get<1>(c), std::ios::out | std::ios::app | std::ios::binary );
        if(this->is_open())[[likely]]{
            std::cout << "Successfully created and opened fresh recompilation file at "<<std::get<1>(c)<<" !"<<std::endl;
        }
        else[[unlikely]]{
            std::cerr << "Could not create recompilation file! Aborting!"<<std::endl;
            return false;
        }
        std::string current_path=std::get<0>(c);
        auto root = std::filesystem::directory_iterator(current_path);
        auto fileIt = std::filesystem::begin(root);
        std::stack<std::tuple<decltype(root), decltype(fileIt)>>fileTreeStack;
        fileTreeStack.emplace(root,fileIt);

        bool is_dir=std::filesystem::is_directory(current_path);
        bool is_empty=std::filesystem::is_empty(current_path);

        //creating the tree structure to seek into a particular position in the recompilation file
        //tellp might throw an exception, and also tellp return a integer which is not uint64_t
        std::stringstream recomp_stream;
        rootShrPtr=std::make_shared<treeNode<std::string, std::uint64_t>>(current_path, recomp_stream.tellp(), nullptr);
        treeNode<std::string,std::uint64_t>* parentPtr = rootShrPtr.get();

        Data d_first(current_path, (is_empty and is_dir), m_client);
        SequentialContainer auto d_first_encode=d_first.encodeD<std::vector<unsigned char>>();

        std::for_each(d_first_encode.cbegin(),d_first_encode.cend(),[&recomp_stream](const unsigned char character){
            recomp_stream<<character;
        });

        if(std::get<1>(fileTreeStack.top())!=std::filesystem::end(std::get<0>(fileTreeStack.top())))[[likely]]{
            do{
                current_path = std::get<1>(fileTreeStack.top())->path();
                is_dir=std::filesystem::is_directory(current_path);
                if (is_dir) {
                    parentPtr = parentPtr->addChild(std::get<1>(fileTreeStack.top())->path().filename().string(), static_cast<uint64_t>(recomp_stream.tellp()));
                } else {
                    parentPtr->addChild(std::get<1>(fileTreeStack.top())->path().filename().string(), static_cast<uint64_t>(recomp_stream.tellp()));
                }
                if (parentPtr != nullptr) {
                    INFO << parentPtr->getData() << " added the child with path: " << current_path << " and seek: " << recomp_stream.tellp();
                } else {
                    throw std::runtime_error("Failed Making the tree structure.");
                }
                std::get<1>(fileTreeStack.top())++;
                if(is_dir)[[unlikely]]{
                    root = std::filesystem::directory_iterator(current_path);
                    fileIt = std::filesystem::begin(root);
                    fileTreeStack.emplace(root,fileIt);
                }
                long int diff_go_up_levels=0;
                while(not fileTreeStack.empty() and std::get<1>(fileTreeStack.top())==std::filesystem::end(std::get<0>(fileTreeStack.top()))){
                    diff_go_up_levels++;
                    fileTreeStack.pop();
                    if (parentPtr->getParent()!= nullptr)
                        parentPtr = parentPtr->getParent();
                }
                Data d(current_path, diff_go_up_levels, m_client);
                m_upload_size += d.size();
                SequentialContainer auto d_loop_encode=d.encodeD<std::vector<unsigned char>>();
                std::for_each(d_loop_encode.cbegin(),d_loop_encode.cend(),[&recomp_stream](const unsigned char character){recomp_stream<<character;});
            }while(!fileTreeStack.empty());
        }

        // wrap the whole recompilation data by using an encoder
        EnDecoder encoder;
        //TODO: optimize here since a copy of a stingstream buffer is made
        SequentialContainer auto recomp_stream_encoding = encoder.encode<std::vector<unsigned char>>(recomp_stream.str());
        std::for_each(recomp_stream_encoding.cbegin(),recomp_stream_encoding.cend(),[this](const unsigned char character){*this<<character;});
        recomp_stream.flush();

        // encode the final tree structure that has the seek position of the recompilation file
        auto serializedReconpTree = BFSSerializer<std::vector<unsigned char>, std::string, std::uint64_t>(rootShrPtr.get());
        std::for_each(serializedReconpTree.cbegin(),serializedReconpTree.cend(),[this](const unsigned char character){*this<<character;});

        this->flush();
        this->close();
    }
    return valid();
}

// TODO: decoding has to be re-done since encoding has been changed
bool Recompilation::decode() {
    std::cout << "Start decoding..." << std::endl;
    for(const auto&c:input_commands){
        try{
            this->open(std::get<0>(c), std::ios::in | std::ios::binary);
            if (this->is_open())[[likely]] {
                std::cout << "Successfully opened existing recompilation file!" <<std::endl;
            } else[[unlikely]] {
                std::cerr << "Could not open recompilation file! Aborting!"<<std::endl;
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char>input;
            char reader;
            while(this->get(reader)){
                input.push_back(reader);
            }
            this->flush();
            this->close();

            // take the entire recompilation data that is wrapped with the encoding excluding the serialized tree data
            EnDecoder coder{};
            auto step=input.begin();
            //TODO: can be optimized here since a copy is being made
            auto internal= coder.decoder<std::vector<unsigned char>>(input,step);
            step=std::get<1>(internal);
            unsigned char offset = step - input.begin() - std::get<0>(internal).size();

            //start from root folder and in parallel step input vector
            auto root_folder_current_it = std::make_tuple(std::get<1>(c),input.begin()+offset);
            std::filesystem::create_directories(std::get<0>(root_folder_current_it));

            Data d(m_client);
            do{
                root_folder_current_it = d.write_from_stream_vector(input,root_folder_current_it);
            }while(std::get<1>(c)!=std::get<0>(root_folder_current_it));

            std::cout << "Successfully read existing recompilation file!"<<std::endl;
        }
        catch(const std::exception &e){
            FATAL << "Reading recompilation file failed due to this reason: " << e.what() << std::endl;
            return false;
        }
    }

    return true;
}

bool Recompilation::ls(){
    for(const auto&c:input_commands) {
        try {
            this->open(std::get<0>(c), std::ios::in | std::ios::binary);
            if (this->is_open())[[likely]] {
                std::cout << "Successfully opened existing recompilation file!\n" <<std::endl;
            } else[[unlikely]] {
                std::cerr << "Could not open recompilation file! Aborting!"<<std::endl;
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char>input;
            char reader;
            while(this->get(reader)){
                input.push_back(reader);
            }
            this->flush();
            this->close();

            // take the entire recompilation data that is wrapped with the encoding excluding the serialized tree data
            EnDecoder coder{};
            auto step=input.begin();
            //TODO: can be optimized here since a copy is being made
            auto internal= coder.decoder<std::vector<unsigned char>>(input,step);
            step=std::get<1>(internal);
            unsigned char offset = step - input.begin() - std::get<0>(internal).size();

            // deserialize the tree structure in the recompilation file
            BFSDeSerializer(rootShrPtr,input, step, offset);
            auto childFound = rootShrPtr->getChildByRPath(std::get<1>(c));
            std::cout  << std::get<1>(c).string() << "/\n" << childFound;

        } catch (const std::exception &e) {
            FATAL << "Ls failed due to this reason: " << e.what() << std::endl;
            return false;
        }
    };
    return true;
}
bool Recompilation::cd() {
    return true;
}

std::filesystem::path Recompilation::fileNameCount(std::filesystem::path filename) {
    std::filesystem::path noExtension = filename;
    std::string toTransformFileName(noExtension.replace_extension("").filename().string());
    auto backIt = toTransformFileName.end();
    std::string numCheck;
    bool correctNum = true;
    std::size_t remove_from_back = 0;
    while (backIt != toTransformFileName.begin()) {
        backIt--;
        if (*backIt == '_')[[unlikely]] {
            remove_from_back++;
            break;
        }
        if (std::isdigit(*backIt))[[unlikely]] {
            remove_from_back++;
            numCheck.insert(numCheck.cbegin(), *backIt);
        } else[[likely]] {
            correctNum = false;
            break;
        }
    }
    if (numCheck.empty())[[likely]] {
        correctNum = false;
    }
    std::filesystem::path newFilename;
    if (correctNum)[[unlikely]] {
        unsigned long long int curCount = std::stoull(numCheck) + 1;
        newFilename = toTransformFileName.substr(0,toTransformFileName.size()-remove_from_back) + "_" + std::to_string(curCount) + filename.extension().string();
    } else[[likely]] {
        newFilename = filename.replace_extension("").filename().string() + "_1" + filename.extension().string();
    }
    newFilename = (filename.parent_path()/=newFilename);
    return newFilename;
}

Recompilation::Recompilation(unsigned char in_type, std::list<std::string> input,
                             std::map<std::string, bool> flags, uh::protocol::client& client): m_client(client) {
    multimode=input.size()>=2;
    flag_check=std::move(flags);
    recomp_Mode = in_type;
    std::filesystem::path from, to;
    overwrite=false;//TODO: remove
    auto inputIt = input.begin();
    while(multimode?inputIt!=--input.end():inputIt!=input.end()) {
        from = std::filesystem::path(*inputIt).make_preferred();//get dirPath
        if (std::filesystem::is_directory(from))[[unlikely]] {
            treeTypes.push_back(directories);
        } else[[likely]] {
            if (std::filesystem::is_regular_file(from))[[likely]] {
                treeTypes.push_back(files);
            } else[[unlikely]] {
                treeTypes.push_back(unknown);
            }
        }
        if (input.size() == 1) {
            if (not std::filesystem::exists(from))[[unlikely]] {
                treeTypes.push_back(notexist);
            } else[[likely]] {
                to = from;
                treeTypes.push_back(treeTypes.at(0));
            }
        } else[[likely]] {
            std::filesystem::path tmp2p = input.back();
            if (not std::filesystem::exists(from))[[unlikely]] {
                treeTypes.push_back(notexist);
            }
            if (not std::filesystem::exists(to))[[unlikely]] {
                treeTypes.push_back(notexist);
                to = tmp2p.make_preferred();
            } else {
                to = std::filesystem::canonical(tmp2p.make_preferred());
                if (std::filesystem::is_directory(to))[[unlikely]] {
                    treeTypes.push_back(directories);
                } else[[likely]] {
                    if (std::filesystem::is_regular_file(to))[[likely]] {
                        treeTypes.push_back(files);
                    } else[[unlikely]] {
                        treeTypes.push_back(unknown);
                    }
                }
            }
        }
        //unknown for beta must be cancelled
        if (std::any_of(treeTypes.begin(), treeTypes.end(), [](auto state) { return state == unknown; })) {
            std::cerr << "There were unknown filetypes in the selected paths for this operation. Cancelling!" << std::endl;
            return;
        }
        if (recomp_Mode=='w') {
            //if the input is not valid nothing can be done to fix it
            if (treeTypes[0] == notexist) {
                std::cerr << "The input path " << from
                          << " does not exist! On this way also the target cannot be determined! Enter a file or directory!"
                          << std::endl;
                return;
            }

            if (treeTypes[1] == notexist) {
                //if the  target path does not exist check if a file or directory was intended, but in this case dont overwrite
                overwrite = false;//TODO: also request keep flag, better use an force flag instead and keep by default
                //if the target file ends with a slash it was intended to be a directory, else it should be a file
                if (to.string().ends_with('/') or to.string().ends_with('\\'))[[unlikely]] {
                    //path must be a directory, we check if we still have to create it
                    std::filesystem::create_directories(to);
                    treeTypes[1] = directories;
                    std::cout << "The path " << to
                              << " did not exist, but the directories were created recursively!"
                              << std::endl;
                } else {
                    treeTypes[1] = files;
                }
            }
            if (treeTypes[1] == directories) {
                //the target is definitely a directory and this means we need to generate a name for it, as it was not given!
                std::string newname = "recompilation";
                if (not(boost::algorithm::ends_with(to.string(), "/") or
                        boost::algorithm::ends_with(to.string(), "\\"))) {
                    while (to.has_extension()) {
                        to = to.replace_extension("");
                    }
                    newname = to.filename();
                    to = to.parent_path().make_preferred();
                }
                to /= newname;
                to = to.make_preferred();
                treeTypes[1] = files;
            }
            //if the extension is not .uh, append it
            if (not boost::algorithm::ends_with(to.extension().string(), ".uh")) {
                to += ".uh";
            }
            //make sure recompilation file is existent and empty
            while (std::filesystem::exists(to))[[likely]] {
                if (overwrite) {
                    //reset
                    std::filesystem::remove_all(to);
                } else {
                    //rename to path
                    std::string old = to.string();
                    to = fileNameCount(to);
                    std::cout << "Renaming output filename from " << old << "\n to "
                              << to << ", because the first file existed already.\n" << std::endl;

                }
            }
        } else if (recomp_Mode=='r' || recomp_Mode=='l'){
            //TODO: debug decoding from already fixed encoding
            //if the input is not valid nothing can be done to fix it
            if (treeTypes[0] == notexist)[[unlikely]] {
                std::cerr << "The input path " << from
                          << " does not exist! On this way also the target cannot be determined! Enter the correct path to the recompilation file!"
                          << std::endl;
                return;
            }
            //if the input is a directory nothing can be done to fix it
            if (treeTypes[0] == directories)[[unlikely]] {
                std::cerr << "The input path " << from
                          << " seems to be a directory instead of the recompilation file! Enter the correct path to the recompilation file!"
                          << std::endl;
                return;
            }
            //if the source is a file we should check if it is a true recompilation file
            if (treeTypes[0] == files)[[likely]] {
                if (not boost::algorithm::ends_with(to.extension().string(), ".uh")) {
                    std::cout << "Warning: From the given file at " << to << " no extension was found!"
                              << std::endl;
                }
                try {
                    this->open(from, std::ios::in);
                    this->close();
                }
                catch (std::exception &e) {
                    std::cerr << "The following error occurred while trying to open the recompilation file: "
                              << e.what() << std::endl;
                    return;
                }
                if (valid())[[likely]] {
                    std::cout << "The input recompilation file was opened and validated successfully!" << std::endl;
                } else[[unlikely]] {
                    std::cerr << "The recompilation file was not valid. Aborting. " << std::endl;
                }
            } else[[unlikely]] {
                std::cerr << "An unknown error occurred after all possibilities have already been checked!"
                          << std::endl;
                return;
            }
            if (recomp_Mode=='r') {
                //if the target is a directory
                if (treeTypes[1] == notexist) {
                    //if the target file ends with a slash it was intended to be a directory, else it should be a file
                    if (to.string().ends_with('/') or to.string().ends_with('\\'))[[unlikely]] {
                        //path must be a directory, we check if we still have to create it
                        std::filesystem::create_directories(to);
                        treeTypes[1] = directories;
                        std::cout << "The path " << to
                                  << " did not exist, but the directories were created recursively!"
                                  << std::endl;
                    } else {
                        treeTypes[1] = files;
                        std::cout
                                << "The output path did not end with / or \\ so the output was thought to be a file, which is illegal!"
                                << std::endl;
                    }
                }
                //if the target type is still a file from the kind of path construction, we will choose the parent folder instead and create the recompilation file name if it does not exist
                if (treeTypes[1] == files) {
                    to = std::filesystem::path(to.parent_path().string() + "/" + to.stem().string()).make_preferred();
                    treeTypes[1] = directories;
                    std::cout
                            << "The output path was a folder type or not declared. Since we can only put the output into a directory, we generated the following output folder: \n"
                            << to
                            << std::endl;
                    while (std::filesystem::exists(to))[[likely]] {
                        std::string old = to.string();
                        to = fileNameCount(to);
                        std::cout << "Renaming output filename from \n" << old << "\n to\n"
                                  << to << "\n, because the first file existed already.\n" << std::endl;
                    }
                }
                if (not std::filesystem::exists(to))[[likely]] {
                    std::cout << "The output is non existent so the target file tree is recursively created at: " << to
                              << std::endl;
                } else[[unlikely]] {
                    std::cout
                            << "The output is already existent we will try to merge into the file tree structure!"
                            << std::endl;
                }
            }
        }
        //actually work through after prepare
        std::cout << "INPUT: " << from << std::endl;
        std::cout << "OUTPUT " << to << "\n\n";
        input_commands.emplace_back(from,to);
        inputIt++;
    }
    if(recomp_Mode=='w'){
        if(encode()){
            std::cout << "Encoding was successful!" << std::endl;
        }
        else{
            std::cerr << "Encoding failed with unknown error!" << std::endl;
        }
    }
    else if (recomp_Mode=='r'){
        if(decode()){
            std::cout << "Decoding was successful!" << std::endl;
        }
        else{
            std::cerr << "Decoding failed with unknown error!" << std::endl;
        }
    } else if (recomp_Mode=='c') {
        if(cd()){
            std::cout << "Cd was successful!" << std::endl;
        }
        else{
            std::cerr << "Cd failed with unknown error!" << std::endl;
        }
    } else if(recomp_Mode=='l') {
        //TODO: Sanitize the output if it contained doubles slahes
        if(ls()){
            std::cout << '\n';
            std::cout << "Ls was successful!" << std::endl;
        }
        else{
            std::cerr << "Ls failed with unknown error!" << std::endl;
        }
    }
}

Recompilation::~Recompilation() {
    this->flush();
    this->close();
}

bool Recompilation::valid() {
    //TODO: validity check
    return true;
}

#endif
