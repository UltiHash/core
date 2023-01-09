//
// Created by benjamin-elias on 17.06.22.
//
#ifndef CMAKE_BUILD_DEBUG_ULTIHASH_BLOCK_H
#define CMAKE_BUILD_DEBUG_ULTIHASH_BLOCK_H

#include "EnDecoder.h"

//TODO: a block should be derived from a BigInteger
//template<SequentialContainer T=std::vector<unsigned long long int>,std::enable_if_t<std::is_unsigned<boost::mp11::mp_first<T>>::value, bool> = true>
class Block{
    using innerType=boost::mp11::mp_first<std::vector<unsigned long long int>>;
private:
    std::string infoS;

public:
    [[nodiscard]] std::size_t size() const;

    std::string& get();

    template<class OutType>
    OutType encode() {
        EnDecoder coder{};
        return coder.encode<OutType>(infoS);
    }

    template<class InType>
    void decode(InType input) {
        EnDecoder coder{};
        infoS=coder.decoder<std::string>(input);
    }

    template<typename...Args>
    explicit Block(Args...input){
        if constexpr(sizeof...(input)==1){
            infoS=std::string(input...);
        }
    }
};
//Test of Level 3 Block types that could be processed in any form
//BOOST_CLASS_VERSION(Block, BOOST_CLASS_VERSION_NUM)
/*
BOOST_CLASS_VERSION(Block<std::vector<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::vector<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::vector<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<boost::container::vector<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::vector<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::vector<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::vector<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<boost::container::stable_vector<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::stable_vector<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::stable_vector<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::stable_vector<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<std::deque<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::deque<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::deque<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::deque<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<boost::container::deque<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::deque<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::deque<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::deque<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<boost::container::devector<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::devector<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::devector<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::devector<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<std::forward_list<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::forward_list<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::forward_list<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::forward_list<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<boost::container::slist<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::slist<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::slist<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::slist<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<std::list<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::list<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::list<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<std::list<unsigned char>>, BOOST_CLASS_VERSION_NUM)

BOOST_CLASS_VERSION(Block<boost::container::list<unsigned long long int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::list<unsigned int>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::list<unsigned short>>, BOOST_CLASS_VERSION_NUM)
BOOST_CLASS_VERSION(Block<boost::container::list<unsigned char>>, BOOST_CLASS_VERSION_NUM)
 */
#endif //CMAKE_BUILD_DEBUG_ULTIHASH_BLOCK_H
