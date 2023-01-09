//
// Created by ankit on 01.12.22.
//

#ifndef CLIENT_SHELL_RECOMP_TREE_H
#define CLIENT_SHELL_RECOMP_TREE_H

#include <serialization/EnDecoder.h>

namespace uh::recomp {
    /**
     * @brief A treeNode represents a file/directory of the user on RAM for fast access in the recompilation file.
     * @tparam T Templated type for implementing the logic of a tree to any data type.
     *
     * treeNode is a tree data structure that is used for seeking into a specific position into the Recompilation file according to the path the user wants to
     * interact with.
     */
    template <typename T, typename U>
    struct treeNode : public std::enable_shared_from_this<treeNode<T,U>> {
    public:

        treeNode() : m_data() , m_parent(nullptr), m_seek(0), m_nChild(0) {};

        explicit treeNode(const T &data, const U& seek = 0, treeNode<T,U>* pr= nullptr, std::size_t nChild = 0) : m_data(data), m_seek(seek), m_parent(pr), m_nChild(nChild) {}

        virtual ~treeNode() = default;

        // BASIC SETTERS
        void setData(const T& t)
        {
            this->m_data = t;
        }

        void increChildN() {
            this->m_nChild++;
        }

        void setSeek(const U& seek)
        {
            this->m_seek = seek;
        }

        void setParent(const treeNode<T,U>* shptr)
        {
            m_parent = shptr;
        }

        // BASIC GETTERS
        std::shared_ptr<treeNode<T,U>> getSharedPtr() {
            return this->shared_from_this();
        }

        const T& getData() const
        {
            return this->m_data;
        }

        [[nodiscard]] const std::size_t& getNumChild() const {
            return this->m_nChild;
        }

        const U& getSeek() const {
            return this->m_seek;
        }

        const std::unordered_map<T, std::shared_ptr<treeNode>>& getChildren() const
        {
            return this->m_children;
        }

        treeNode<T,U>* getParent()
        {
            return m_parent;
        }

        // FUNCTIONS
        bool removeChild(const T& t)
        {
            return m_children.erase(t);
        }

        treeNode<T,U>* addChild(const T& t, const U& seek, const std::size_t& nChild=0)
        {
            std::shared_ptr<treeNode<T,U>> newChildPtr = std::make_shared<treeNode<T,U>>(t, seek, this, nChild);
            if (!newChildPtr.get())
            {
                return nullptr;
            }
            else
            {
                auto result = m_children.insert(std::make_pair(t, newChildPtr));
                if (result.second) {
                    return newChildPtr.get();
                } else {
                    return nullptr;
                }
            }
        }

        // get the direct child of the node by the key provided
        treeNode<T,U>* getChild(const T& keyName) const
        {
            auto tmpVal = m_children.find(keyName);
            if ( tmpVal == m_children.end())
            {
                return nullptr;
            }
            else
            {
                return tmpVal->second.get();
            }
        }

        // get the indirect child of the node by the relative path from the current node
        treeNode<T,U>* getChildByRPath(const T& relPath)
        {
            std::filesystem::path prelPath(relPath);
            treeNode<T,U>* currNode = this;
            // check if the path is in correct representation or not: path should be of the form path1/path2/path3
            for (const auto& elemName: prelPath)
            {
                auto tmpVar = currNode->m_children.find(elemName);
                if (tmpVar==m_children.end())
                {
                    return nullptr;
                }
                else {
                    currNode = tmpVar->second.get();
                }
            }
            return currNode;
        }

        friend std::ostream& operator<<(std::ostream& os, treeNode<T,U>* trNode)
        {
            std::for_each(trNode->getChildren().begin(),
                          trNode->getChildren().end(),
              [&os](const std::pair<T, std::shared_ptr<treeNode<T,U>>> &p)
              {
                  os  << p.first << "   ";
              });
            os << "\n";
            return os;
        }

    protected:
        // TODO: benchmark map vs unordered_map implementation
        // memory alignment
        // !! It is up to the programmer to ensure that whichever thread works on a node, they have the shared_ptr by copy constructor. This ensures the member variables to be
        // passed by reference.
        T m_data;
        U m_seek;
        size_t m_nChild;
        treeNode<T,U>* m_parent;
        std::unordered_map<T, std::shared_ptr<treeNode<T,U>>> m_children;
    private:

    };

    // a BFS algorithm that uses the serialization function to serialize the tree Node into the Recompilation file
    // TODO: string for now but has to return bool or throw exception for identifying whether serialization was able to be performed or not
    // catch error close to the client
    template <typename OutType, typename T, typename U>
    OutType BFSSerializer(treeNode<T,U>* rootNode)
    {
        OutType serializedOutput;
        EnDecoder encoder{};
        std::deque<treeNode<T, U>*> treeLevel;
        treeLevel.push_back(rootNode);
        while (!treeLevel.empty())
        {
            treeNode<T,U>* t_Parent = treeLevel.front();
            treeLevel.pop_front();

            // serialization part of the nodes
            SequentialContainer auto t_dataEnc = encoder.encode<OutType>(t_Parent->getData());
            serializedOutput.insert(serializedOutput.cend(),t_dataEnc.begin(),t_dataEnc.end());
            SequentialContainer auto t_seekEnc = encoder.encode<OutType>(t_Parent->getSeek());
            serializedOutput.insert(serializedOutput.cend(),t_seekEnc.begin(),t_seekEnc.end());
            SequentialContainer auto t_nChildEnc = encoder.encode<OutType>(t_Parent->getChildren().size());
            serializedOutput.insert(serializedOutput.cend(),t_nChildEnc.begin(),t_nChildEnc.end());

            for (const auto &t_Pair: t_Parent->getChildren())
            {
                treeLevel.push_back(t_Pair.second.get());
            }
        }
        return serializedOutput;
    }

    template <typename OutType, typename T, typename U, typename Iterator>
    bool BFSDeSerializer(std::shared_ptr<treeNode<T,U>>& rootNode, OutType& input, Iterator& step, unsigned char& offset)
    {
        try {
            std::deque<treeNode<T, U>*> treeLevel;
            treeNode<T, U> *t_Parent;
            std::size_t c_child = 0;

            // get the root node
            auto internal_decoder_func = [&step,&offset]<typename OutType2>(OutType2& input){
                EnDecoder encoder{};
                auto t_dataDec = encoder.decoder<std::string>(input, step);
                auto t_Data = std::get<0>(t_dataDec);
                step = std::get<1>(t_dataDec);
                auto t_seekEnc = encoder.decoder<std::uint64_t>(input, step);
                auto t_Seek = std::get<0>(t_seekEnc) + offset;
                step = std::get<1>(t_seekEnc);
                auto t_nChildEnc = encoder.decoder<std::size_t>(input, step);
                auto t_Children = std::get<0>(t_nChildEnc);
                step = std::get<1>(t_nChildEnc);

                return std::make_tuple(t_Data,t_Seek,t_Children);
            };

            auto decoded_tuple = internal_decoder_func(input);

            rootNode = std::make_shared<treeNode<T, U>>(std::get<0>(decoded_tuple), std::get<1>(decoded_tuple), nullptr, std::get<2>(decoded_tuple));
            treeLevel.push_back(rootNode.get());

            while (!treeLevel.empty()) {
                t_Parent = treeLevel.front();
                treeLevel.pop_front();
                // generate the tree Nodes in BFS style as it was encoded that way
                while (c_child != t_Parent->getNumChild()) {
                    decoded_tuple = internal_decoder_func(input);
                    auto somePtr = t_Parent->addChild(std::get<0>(decoded_tuple), std::get<1>(decoded_tuple), std::get<2>(decoded_tuple));
                    treeLevel.push_back(somePtr);
                    c_child++;
                }
                c_child=0;
            }
            return true;
        } catch (const std::exception& e) {
            ERROR << e.what();
            return false;
        }
    }
}

#endif //CLIENT_SHELL_RECOMP_TREE_H
