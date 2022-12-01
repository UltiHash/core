//
// Created by ankit on 01.12.22.
//

#ifndef CLIENT_SHELL_RECOMP_TREE_H
#define CLIENT_SHELL_RECOMP_TREE_H
#include <conceptTypes/conceptTypes.h>

/**
 * @brief A recomp namespace for everything that is specific to the recompilation file.
 */
namespace uh::recomp {
    /**
     * @brief A treeNode represents a file/directory on RAM.
     * @tparam T Templated type for implementing the logic of a tree to any data ty[e.
     *
     * treeNode is a tree data structure that is used for seeking into a specific position into the Recompilation file according to the path the user wants to
     * interact with.
     */
    template <typename T>
    class treeNode {
    public:
        treeNode() = default;
        virtual ~treeNode() = default;
        void addChild(const T& t);
        void removeChild(const T& t);
        void setValue(const T& t) {
            this->m_data = t;
        }
        const T& getData() const{
            return this->m_data;
        }
        const std::vector<std::shared_ptr<treeNode>>& getChildren() const
        {
            return this->m_children;
        }
        const std::shared_ptr<treeNode> getParent() const
        {
            return this->m_parent;
        }
        void setParent(const std::shared_ptr<treeNode>& shptr){
            m_parent = shptr;
        }

        /**
         * @brief Overloading the "<<" operator for printing out the children of a node.
         * @param os Reference to the output stream.
         * @param treeNode Reference to the treeNode class.
         * @return Reference to the output stream
         */
        friend std::ostream& operator<<(std::ostream& os, const treeNode& treeNode);

    protected:
        // QN: Vector searches the path linearly
        T m_data;
        std::shared_ptr<treeNode> m_parent;
        std::vector<std::shared_ptr<treeNode>> m_children;
    private:
    };
}

#endif //CLIENT_SHELL_RECOMP_TREE_H
