//
// Created by ankit on 01.12.22.
//

#ifndef CLIENT_SHELL_RECOMP_TREE_H
#define CLIENT_SHELL_RECOMP_TREE_H
#include "logging/logging_boost.h"

// TODO: Test the tree structure
namespace uh::recomp {
    /**
     * @brief A treeNode represents a file/directory of the user on RAM for fast access in the recompilation file.
     * @tparam T Templated type for implementing the logic of a tree to any data type.
     *
     * treeNode is a tree data structure that is used for seeking into a specific position into the Recompilation file according to the path the user wants to
     * interact with.
     */
    template <typename T>
    struct treeNode {
    public:

        treeNode() = default;

        explicit treeNode(const T &data, const treeNode<T>* pr= nullptr) : m_data(data), m_parent(pr) {}

        virtual ~treeNode() = default;

        bool addChild(const T& t)
        {
            std::shared_ptr<treeNode<T>> newChildPtr = std::make_shared<treeNode<T>>(t, this);
            if (!newChildPtr.get())
            {
                return false;
            }
            else
            {
                auto result = m_children.insert(std::make_pair(t, newChildPtr));
                return result.second;
            }
        }

        bool removeChild(const T& t)
        {
            return m_children.erase(t);
        }

        void setData(const T& t)
        {
            this->m_data = t;
        }

        const T& getData() const
        {
            return this->m_data;
        }

        const std::unordered_map<T, std::shared_ptr<treeNode>>& getChildren() const
        {
            return this->m_children;
        }

        std::shared_ptr<treeNode<T>> getChild(const T& pathStr) const
        {
            auto tmpVar = this->m_children.find(pathStr);
            if (tmpVar == m_children.end())
            {
                return nullptr;
            }
            else
            {
                return tmpVar->second;
            }
        }

        const treeNode<T>* getParent() const
        {
            return this->m_parent;
        }

        void setParent(const treeNode<T>* shptr){
            m_parent = shptr;
        }

        friend std::ostream& operator<<(std::ostream& os, std::shared_ptr<treeNode<T>>& trNode)
        {
            os << "Parent: " << trNode->getParent() << " Data: " << trNode->getData() << " Children: ";
            std::for_each(trNode->getChildren().begin(),
                          trNode->getChildren().end(),
              [&os](const std::pair<T, std::shared_ptr<treeNode<T>>> &p)
              {
                  os << "{" << p.first << ": " << p.second << "} ";
              });
            os << "\n";
            return os;
        }

    protected:
        // TODO: benchmark map vs unordered_map implementation
        // TODO: make ordered_map and unordered_map also templated since benchmarking can be made easier
        // memory alignment
        T m_data;
        const treeNode<T>* m_parent;
        std::unordered_map<T, std::shared_ptr<treeNode<T>>> m_children;
    private:

    };
}

#endif //CLIENT_SHELL_RECOMP_TREE_H
