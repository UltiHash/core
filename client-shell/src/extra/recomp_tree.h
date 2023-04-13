//
// Created by ankit on 01.12.22.
//

#ifndef CLIENT_SHELL_RECOMP_TREE_H
#define CLIENT_SHELL_RECOMP_TREE_H

#include <filesystem>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

template <typename T, typename U>
struct treeNode : public std::enable_shared_from_this<treeNode<T,U>>
{
    public:

        treeNode()
            : m_data(),
              m_seek(0),
              m_nChild(0),
              m_parent(nullptr)
        {
        }

        explicit treeNode(const T &data, const U& seek = 0, treeNode<T,U>* pr= nullptr, std::size_t nChild = 0)
            : m_data(data),
              m_seek(seek),
              m_nChild(nChild),
              m_parent(pr)
        {
        }

        void setData(const T& t)
        {
            this->m_data = t;
        }

        void increChildN()
        {
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

        const T& getData() const
        {
            return this->m_data;
        }

        [[nodiscard]] const std::size_t& getNumChild() const
        {
            return this->m_nChild;
        }

        const U& getSeek() const
        {
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

        bool removeChild(const T& t)
        {
            return m_children.erase(t);
        }

        treeNode<T,U>* addChild(const T& t, const U& seek, const std::size_t& nChild=0)
        {
            std::shared_ptr<treeNode<T,U>> newChildPtr = std::make_shared<treeNode<T,U>>(t, seek, this, nChild);
            auto result = m_children.insert(std::make_pair(t, newChildPtr));
            if (result.second)
            {
                return newChildPtr.get();
            }
            else
            {
                return nullptr;
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
                else
                {
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

    private:
        T m_data;
        U m_seek;
        size_t m_nChild;
        treeNode<T,U>* m_parent;
        std::unordered_map<T, std::shared_ptr<treeNode<T,U>>> m_children;
};


} // namespace uh::client::serialization

#endif //CLIENT_SHELL_RECOMP_TREE_H
