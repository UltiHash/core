#include "Recompilation.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

Recompilation::Recompilation(const client_config& config, std::unique_ptr<uh::protocol::client>&& client) : m_config(config), m_client(std::move(client))
{
    if (m_config.m_option == options_chosen::integrate)
    {
        encode();
    }
    else if (m_config.m_option == options_chosen::retrieve)
    {
        decode();
    }
    else if (m_config.m_option == options_chosen::list)
    {
        ls();
    }
}

// ---------------------------------------------------------------------

Recompilation::~Recompilation()
{
    this->flush();
    this->close();
}

// ---------------------------------------------------------------------

void Recompilation::encode()
{
    for (const auto &input_Path : m_config.m_inputPaths)
    {
        this->open(m_config.m_outputPath, std::ios::out | std::ios::app | std::ios::binary);
        if (this->is_open())[[likely]]
        {
            INFO << "Successfully created and opened the recompilation file at " << m_config.m_outputPath << "."
                      << std::endl;
        } else[[unlikely]]
        {
            throw std::runtime_error("Could not create recompilation file! Aborting!");
        }
        std::string current_path = input_Path;
        auto root = std::filesystem::directory_iterator(current_path);
        auto fileIt = std::filesystem::begin(root);
        std::stack <std::tuple<decltype(root), decltype(fileIt)>> fileTreeStack;
        fileTreeStack.emplace(root, fileIt);

        bool is_dir = std::filesystem::is_directory(current_path);
        bool is_empty = std::filesystem::is_empty(current_path);

        //creating the tree structure to seek into a particular position in the recompilation file
        //tellp might throw an exception, and also tellp return a integer which is not uint64_t
        std::stringstream recomp_stream;
        rootShrPtr = std::make_shared < treeNode < std::string, std::uint64_t
                >> (current_path, recomp_stream.tellp(), nullptr);
        treeNode <std::string, std::uint64_t> *parentPtr = rootShrPtr.get();

        Data d_first(current_path, (is_empty and is_dir), m_client);
        SequentialContainer auto d_first_encode = d_first.encodeD < std::vector < unsigned char >> ();

        std::for_each(d_first_encode.cbegin(), d_first_encode.cend(),
                      [&recomp_stream](const unsigned char character)
                      {
                          recomp_stream << character;
                      });

        if (std::get<1>(fileTreeStack.top()) != std::filesystem::end(std::get<0>(fileTreeStack.top())))[[likely]]
        {
            do
            {
                current_path = std::get<1>(fileTreeStack.top())->path();
                is_dir = std::filesystem::is_directory(current_path);
                if (is_dir)
                {
                    parentPtr = parentPtr->addChild(std::get<1>(fileTreeStack.top())->path().filename().string(),
                                                    static_cast<uint64_t>(recomp_stream.tellp()));
                }
                else
                {
                    parentPtr->addChild(std::get<1>(fileTreeStack.top())->path().filename().string(),
                                        static_cast<uint64_t>(recomp_stream.tellp()));
                }
                if (parentPtr != nullptr)
                {
                    INFO << parentPtr->getData() << " added the child with path: " << current_path << " and seek: "
                         << recomp_stream.tellp();
                }
                else
                {
                    throw std::runtime_error("Failed Making the tree structure.");
                }
                std::get<1>(fileTreeStack.top())++;
                if (is_dir)[[unlikely]]
                {
                    root = std::filesystem::directory_iterator(current_path);
                    fileIt = std::filesystem::begin(root);
                    fileTreeStack.emplace(root, fileIt);
                }
                long int diff_go_up_levels = 0;
                while (not fileTreeStack.empty() and
                       std::get<1>(fileTreeStack.top()) == std::filesystem::end(std::get<0>(fileTreeStack.top())))
                {
                    diff_go_up_levels++;
                    fileTreeStack.pop();
                    if (parentPtr->getParent() != nullptr)
                        parentPtr = parentPtr->getParent();
                }
                Data d(current_path, diff_go_up_levels, m_client);
                SequentialContainer auto d_loop_encode = d.encodeD < std::vector < unsigned char >> ();
                std::for_each(d_loop_encode.cbegin(), d_loop_encode.cend(),
                              [&recomp_stream](const unsigned char character) { recomp_stream << character; });
            } while (!fileTreeStack.empty());
        }

        // wrap the whole recompilation data by using an encoder
        EnDecoder encoder;
        //TODO: optimize here since a copy of a stingstream buffer is made
        SequentialContainer auto recomp_stream_encoding =
                encoder.encode < std::vector < unsigned char >> (recomp_stream.str());
        std::for_each(recomp_stream_encoding.cbegin(), recomp_stream_encoding.cend(),
                      [this](const unsigned char character) { *this << character; });
        recomp_stream.flush();

        // encode the final tree structure that has the seek position of the recompilation file
        auto serializedReconpTree =
                BFSSerializer < std::vector < unsigned char >, std::string, std::uint64_t>(rootShrPtr.get());
        std::for_each(serializedReconpTree.cbegin(), serializedReconpTree.cend(),
                      [this](const unsigned char character) { *this << character; });

        this->flush();
        this->close();
    }
}

// ---------------------------------------------------------------------

void Recompilation::decode() {
    std::cout << "Start decoding..." << std::endl;
    for(const auto &input_Path : m_config.m_inputPaths){
        try{
            this->open(input_Path, std::ios::in | std::ios::binary);
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
            auto root_folder_current_it = std::make_tuple(m_config.m_outputPath,input.begin()+offset);
            std::filesystem::create_directories(std::get<0>(root_folder_current_it));

            Data d(m_client);
            do
            {
                root_folder_current_it = d.write_from_stream_vector(input,root_folder_current_it);
            } while(m_config.m_outputPath!=std::get<0>(root_folder_current_it));

            std::cout << "Successfully read existing recompilation file!"<<std::endl;
        }
        catch(const std::exception &e){
            FATAL << "Reading recompilation file failed due to this reason: " << e.what() << std::endl;
        }
    }
}

// ---------------------------------------------------------------------

void Recompilation::ls() {
    for (const auto &input_Path : m_config.m_inputPaths) {
        try {
            this->open(input_Path, std::ios::in | std::ios::binary);
            if (this->is_open())[[likely]] {
                std::cout << "Successfully opened existing recompilation file!\n" << std::endl;
            } else[[unlikely]] {
                std::cerr << "Could not open recompilation file! Aborting!" << std::endl;
                std::exit(EXIT_FAILURE);
            }
            std::vector<unsigned char> input;
            char reader;
            while (this->get(reader)) {
                input.push_back(reader);
            }
            this->flush();
            this->close();

            // take the entire recompilation data that is wrapped with the encoding excluding the serialized tree data
            EnDecoder coder{};
            auto step = input.begin();
            //TODO: can be optimized here since a copy is being made
            auto internal = coder.decoder < std::vector < unsigned char >> (input, step);
            step = std::get<1>(internal);
            unsigned char offset = step - input.begin() - std::get<0>(internal).size();

            // deserialize the tree structure in the recompilation file
            BFSDeSerializer(rootShrPtr, input, step, offset);
            auto childFound = rootShrPtr->getChildByRPath(m_config.m_outputPath);
            std::cout << m_config.m_outputPath.string() << "/\n" << childFound;

        } catch (const std::exception &e) {
            FATAL << "Ls failed due to this reason: " << e.what() << std::endl;
        }
    };
}

// ---------------------------------------------------------------------

} // namespace uh::client
