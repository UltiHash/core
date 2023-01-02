//
// Created by benjamin-elias on 25.08.22.
//
#ifndef CMAKE_BUILD_DEBUG_ULTIHASH_ENCODER_H
#define CMAKE_BUILD_DEBUG_ULTIHASH_ENCODER_H

#include <vector>
#include <tuple>
#include <deque>
#include <list>
#include <forward_list>
#include <string>
#include <cstring>
#include <bitset>
#include <boost/mp11.hpp>
#include <boost/container/vector.hpp>

#include <logging/logging_boost.h>


// Minimal set of concepts required for EnDecoder to work without rewriting
//concept detecting standard vectors with no special properties
template<typename T1>
struct is_vector_standard {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::vector<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::vector<decayed>, decayed>::value;
};

template<typename... T1>
concept VectorStandard = std::conjunction<is_vector_standard<T1>...>::value;


//concept detecting the special boost stable vector that has tree structure inside with stable flag
template<typename T1>
struct is_vector_stable {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<boost::container::stable_vector<decayed>, decayed>::value;
};

//concept detecting deques
template<typename T1>
struct is_deque {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::deque<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::deque<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::devector<decayed>, decayed>::value;
};

//concept detecting forward_lists
template<typename T1>
struct is_forward_list {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::forward_list<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::slist<decayed>, decayed>::value;
};

//concept detecting lists
template<typename T1>
struct is_list {
    using decayed = typename std::decay<T1>::type;
    [[maybe_unused]] static const bool value = boost::mp11::mp_similar<std::list<decayed>, decayed>::value or
                                               boost::mp11::mp_similar<boost::container::list<decayed>, decayed>::value;
};

//concept detecting if the type is any sequential container type
template<typename... Args>
struct is_sequential_container {
    static const bool value = std::conjunction<std::disjunction<is_vector_standard<Args>, is_vector_stable<Args>, is_deque<Args>, is_forward_list<Args>, is_list<Args>>...>::value;
};

template<typename... Args>
concept SequentialContainer = is_sequential_container<Args...>::value;

template<class...Args>
concept String = std::conjunction<std::is_convertible<Args, std::string_view>...>::value;

//concept detecting any arithmetic fundamental type that is signed/unsigned integer base types or floating point base types
template<typename... Args>
concept Arithmetic = std::conjunction<std::disjunction<std::is_arithmetic<Args>, std::conjunction<std::is_arithmetic<typename std::decay<Args>::type>, std::is_lvalue_reference<Args>>>...>::value;


//concept detecting any fundamental type
template<typename... Args>
concept ArithmeticFundamental = std::conjunction<std::conjunction<std::is_arithmetic<Args>>...>::value;

//static type reader, transforming a type infomatiotion into a string that can be processed
template<class T>
constexpr std::string_view
type_name() {
    using namespace std;
#ifdef __clang__
    string_view p = __PRETTY_FUNCTION__;
    return {p.data() + 34, p.size() - 34 - 1};
#elif defined(__GNUC__)
    string_view p = __PRETTY_FUNCTION__;
#if __cplusplus < 201402
    return string_view(p.data() + 36, p.size() - 36 - 1);
#else
    return string_view(p.data() + 49, p.find(';', 49) - 49);
#endif
#elif defined(_MSC_VER)
    string_view p = __FUNCSIG__;
    return string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}


class EnDecoder {
    enum signedness{unsigned_type,signed_type};
    //string type is actually char*
    enum fundamental{char_type,short_type,int_type,long_type,float_type,double_type,long_double_type,string_type};
    unsigned char* internalBuffer{};
    unsigned char type_byte{};
    std::size_t mem_size{};
    std::vector<unsigned char>size_representation{};

    //first bit of type_byte is signedness
    //2,3,4 bit are type of representation (one spare)
    //5,6,7 bit are size of content length
    //8 bit is extension verifier/spare


private:
    inline std::tuple<std::size_t,unsigned char*> encode_raw(){
        return std::make_tuple(mem_size,internalBuffer);
    }
public:

    ~EnDecoder(){
        if(string_type_set()){
            delete[] internalBuffer;
        }
    }

    EnDecoder()=default;

    inline std::vector<unsigned char> content(){
        std::vector<unsigned char> output{internalBuffer,internalBuffer+mem_size};
        return output;
    }

    std::size_t size(){
        bool stsb=string_type_set();
        std::size_t size_out=1+(stsb?size_representation.size():0)+mem_size;
        return size_out;
    }

    [[nodiscard]] inline std::size_t size_data() const{
        return mem_size;
    }

    [[nodiscard]] inline bool string_type_set() const{
        return get_type() == string_type;
    }

    template<SequentialContainer OutType,typename Args,Arithmetic...Size>
    OutType encode(Args input,Size...length){
        size_representation.clear();
        type_byte=0;
        if constexpr ((std::is_same_v<unsigned char*,Args> or std::is_same_v<char*,Args>) and sizeof...(Size)==0){
            BOOST_STATIC_ASSERT_MSG("Size not given to encoder on type "+ type_name<Args>() +" !");
            return std::vector<unsigned char>{};
        }
        if constexpr (sizeof...(Size)>1 or (sizeof...(Size)==1 and (not (std::is_same_v<unsigned char*,Args> or std::is_same_v<char*,Args>) or SequentialContainer<Args> or std::is_same_v<Args,std::istringstream> or std::is_same_v<Args,std::istream> or std::is_same_v<Args,std::stringstream> or std::is_same_v<Args,std::fstream>))) {
            BOOST_STATIC_ASSERT_MSG("Too many size variables given on type "+ type_name<Args>() +" !");
            return std::vector<unsigned char>{};
        }
        if constexpr (std::is_same_v<Args,std::istringstream> or std::is_same_v<Args,std::istream> or std::is_same_v<Args,std::stringstream> or std::is_same_v<Args,std::fstream>){
            char c;
            std::string tmp_to_work;
            while(input.get(c)){
                tmp_to_work+=c;
            }
            return encode<OutType>(tmp_to_work);
        }
        if constexpr (std::is_same_v<long double, Args>){
            auto* tmp_buf=reinterpret_cast<unsigned char*>(&input);
            std::vector<unsigned char>convert_buf{tmp_buf+2,tmp_buf};//save space on long double
            internalBuffer=convert_buf.data();
            mem_size=10;
        }
        else{
            if constexpr (SequentialContainer<Args> and not (std::is_same_v<Args,unsigned char*> or std::is_same_v<Args,char*> or std::is_same_v<Args,bool*>)){
                using innerType=boost::mp11::mp_first<Args>;
                if constexpr (!ArithmeticFundamental<innerType>){
                    BOOST_STATIC_ASSERT_MSG("The sequential input container to be encoded did not contain a fundamental arithmetic within on type "+ type_name<Args>() +" !");
                }
                if constexpr (VectorStandard<Args> and (std::is_same_v<innerType,unsigned char> or std::is_same_v<innerType,char> or std::is_same_v<innerType,bool>)){
                    return encode<OutType>(std::string{input.begin(),input.end()});
                }
                else{
                    std::vector<unsigned char> convert_tmp{};
                    const std::size_t inner_bytes=std::is_same_v<long double,innerType>?10:sizeof(innerType);
                    std::size_t total_bytes_size=inner_bytes*input.size();
                    //convert_tmp.reserve(total_bytes_size);
                    for(const auto &i:input){
                        auto* readBuf=reinterpret_cast<unsigned char*>(&i);
                        std::vector<unsigned char> singleBuf{readBuf+std::is_same_v<long double,innerType>?2:0,readBuf+sizeof(innerType)};
                        convert_tmp.insert(convert_tmp.cbegin(),singleBuf.begin(),convert_tmp.end());
                    }
                    auto* output=new unsigned char[total_bytes_size];
                    std::memcpy(output,convert_tmp.data(),total_bytes_size);
                    return encode<OutType>(output,total_bytes_size);
                }
            }
            else{
                //signed/unsigned
                if constexpr (std::is_signed_v<Args>){
                    type_byte|=signed_type;
                }
                else{
                    if constexpr (std::is_unsigned_v<Args>){
                        type_byte|=unsigned_type;
                    }
                }

                if constexpr ((std::is_same_v<bool,Args> or std::is_same_v<char,Args> or std::is_same_v<unsigned char,Args>)){
                    type_byte|=(char_type<<1);
                }
                else{
                    if constexpr (std::is_same_v<short,Args> or std::is_same_v<unsigned short,Args>){
                        type_byte|=(short_type<<1);
                    }
                    else{
                        if constexpr (std::is_same_v<int,Args> or std::is_same_v<unsigned int,Args>){
                            type_byte|=(int_type<<1);
                        }
                        else{
                            if constexpr (std::is_same_v<long int,Args> or std::is_same_v<unsigned long int,Args>){
                                type_byte|=(long_type<<1);
                            }
                            else{
                                if constexpr (std::is_same_v<float,Args>){
                                    type_byte|=(float_type<<1);
                                }
                                else{
                                    if constexpr (std::is_same_v<double,Args>){
                                        type_byte|=(double_type<<1);
                                    }
                                    else{
                                        if constexpr (std::is_same_v<long double,Args>){
                                            type_byte|=(long_double_type<<1);
                                        }
                                        else{
                                            if constexpr (String<Args>){
                                                delete[] internalBuffer;
                                                type_byte|=(string_type<<1);
                                                if constexpr (std::is_same_v<unsigned char*,Args>){
                                                    mem_size=static_cast<std::size_t>(std::forward<Size...>(length...));
                                                    internalBuffer=new unsigned char[mem_size];
                                                    std::memcpy(internalBuffer,input.data(),mem_size);
                                                }
                                                else{
                                                    if constexpr (std::is_same_v<char*,Args>){
                                                        mem_size=static_cast<std::size_t>(std::forward<Size...>(length...));
                                                        internalBuffer=new unsigned char[mem_size];
                                                        std::memcpy(internalBuffer,input.data(),mem_size);
                                                    }
                                                    else{
                                                        if constexpr (std::is_same_v<std::string,Args>){
                                                            mem_size=input.size();
                                                            internalBuffer=new unsigned char[mem_size];
                                                            std::memcpy(internalBuffer,input.data(),mem_size);
                                                        }
                                                        else{
                                                            BOOST_STATIC_ASSERT_MSG("Type "+ type_name<Args>() +" was unknown to encoder and decoder!");
                                                            return std::vector<unsigned char>{};
                                                        }
                                                    }
                                                }
                                            }
                                            //from sized string types calculate how many bytes are needed to represent length and write back to type_byte
                                            std::bitset<64> h_bit{mem_size};
                                            unsigned char total_bit_count{};
                                            //counting max bits
                                            while(h_bit!=0)[[likely]]{
                                                total_bit_count++;
                                                h_bit>>=1;
                                            }
                                            unsigned char byte_count=total_bit_count/8;
                                            if(total_bit_count%8 or byte_count==0)[[likely]]{
                                                byte_count++;
                                            }
                                            if(byte_count>8)[[unlikely]]{
                                                byte_count=8;
                                                DEBUG << "The size of the string was not supported!" << std::endl;
                                            }
                                            type_byte |= ((byte_count-1) << 4);
                                            //shorten memsize to size_representation
                                            auto mem_size_convert=std::array<unsigned char,sizeof(mem_size)>();
                                            std::memcpy(mem_size_convert.data(),reinterpret_cast<unsigned char*>(&mem_size),sizeof(mem_size));
                                            for(unsigned char i=0; i<byte_count;i++){
                                                size_representation.insert(size_representation.begin(),mem_size_convert[i]);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if constexpr (!(String<Args> or std::is_same_v<char*,Args> or std::is_same_v<unsigned char*,Args> or std::is_same_v<Args,std::istringstream> or std::is_same_v<Args,std::istream> or std::is_same_v<Args,std::stringstream> or std::is_same_v<Args,std::fstream> or SequentialContainer<Args>)){
                    mem_size=sizeof(Args);
                    internalBuffer=new unsigned char[mem_size];
                    internalBuffer=reinterpret_cast<unsigned char*>(&input);
                    size_representation.push_back(mem_size);
                }
            }
        }

        return get_encoding<OutType>();
    }

    template<SequentialContainer OutType>
    OutType get_encoding(){
        OutType output;
        if constexpr (not std::is_same_v<boost::mp11::mp_first<OutType>,unsigned char>){
            BOOST_STATIC_ASSERT("The inner input stream type was not unsigned char! Aborting!");
        }
        auto encode_tuple=encode_raw();
        //template<typename> typename InnerType,
        //not safe: output.reserve(size());
        output.push_back(static_cast<unsigned char>(type_byte));
        if(string_type_set())[[likely]]{
            output.insert(output.end(),size_representation.begin(),size_representation.end());
        }
        output.insert(output.end(),std::get<1>(encode_tuple),std::get<1>(encode_tuple)+std::get<0>(encode_tuple));
        return output;
    }

    [[nodiscard]] inline fundamental get_type() const{
        return static_cast<fundamental>(0b00000111 & (type_byte>>1));
    }

    [[nodiscard]] inline signedness get_sign() const{
        return static_cast<signedness>(0b00000001 & (type_byte));
    }

    //either outputs std::tuple<streamtype> or std::tuple<containertype<unsigned char>,containertype::iterator>
    //from input (stream) or (containertype,containertype::iterator)
    template<typename OutType,class SomeStream,typename...Args>
    std::tuple<OutType,Args...> decoder(SomeStream &in, Args...args){
        if constexpr (not (SequentialContainer<SomeStream> or std::is_same_v<SomeStream,std::istringstream> or std::is_same_v<SomeStream,std::istream> or std::is_same_v<SomeStream,std::stringstream> or std::is_same_v<SomeStream,std::fstream>)){
            BOOST_STATIC_ASSERT_MSG("The decoder was not provided by any kind of input stream!");
        }
        if constexpr (SequentialContainer<SomeStream> and sizeof...(Args) != 1){
            BOOST_STATIC_ASSERT_MSG("Wrong number of step iterators given due to inserting some sequential container!");
        }
        if constexpr (SequentialContainer<SomeStream> and not std::is_same_v<boost::mp11::mp_first<SomeStream>,unsigned char>){
            BOOST_STATIC_ASSERT_MSG("The inner type of input stream container was not unsigned char!");
        }

        char c;
        bool state_counter=true;
        bool stsb{};
        fundamental foundType{};
        unsigned char bytes{},count_bytes{};
        unsigned long long count_read{};
        mem_size=0;
        size_representation.clear();
        bool loop=true;

        auto input_tup=std::make_tuple(&in,args...);

        while (loop) {
            unsigned char first_c;
            if constexpr (SequentialContainer<SomeStream>){
                first_c=*std::get<1>(input_tup);
            }
            else{
                if(in.get(c)){
                    loop=true;
                }
                else{
                    loop=false;
                }
                auto* actual_c=reinterpret_cast<unsigned char*>(&c);
                first_c=static_cast<unsigned char>(actual_c[0]);
            }

            if(!loop)[[unlikely]]{
                break;
            }

            if(state_counter)[[unlikely]]{
                type_byte=first_c;
                stsb=string_type_set();
                foundType=get_type();
                if(!stsb)[[unlikely]]{
                    switch (foundType) {
                        case char_type: mem_size=1;break;
                        case short_type: mem_size=2;break;
                        case int_type: mem_size=4;break;
                        case long_type: mem_size=8;break;
                        case float_type: mem_size=4;break;
                        case double_type: mem_size=8;break;
                        case long_double_type: mem_size=12;break;
                        default: break;
                    }
                    size_representation.push_back(mem_size);
                }
                else[[likely]]{
                    bytes=static_cast<std::size_t>((type_byte>>4) & 0b0000111)+1;
                }
                state_counter=false;
            }
            else[[likely]]{
                if(count_bytes<bytes)[[likely]]{
                    mem_size+=static_cast<const unsigned char>(first_c)<<((bytes-count_bytes-1)*8);
                    size_representation.push_back(static_cast<const unsigned char>(first_c));
                    count_bytes++;
                }
                else[[unlikely]]{
                    if(count_read==0)[[unlikely]]{
                        delete[] internalBuffer;
                        internalBuffer=new unsigned char[mem_size];
                        if(foundType==long_double_type)[[unlikely]]{
                            count_read+=2;//first 2 bytes stay empty for memory alignment
                        }
                    }
                    internalBuffer[count_read] = static_cast<unsigned char>(first_c);
                    count_read++;
                    if(count_read==mem_size)[[unlikely]]{
                        if constexpr (SequentialContainer<SomeStream>){
                            std::get<1>(input_tup)++;
                        }
                        break;
                    }
                }
            }
            if constexpr (SequentialContainer<SomeStream>){
                loop = std::get<1>(input_tup)!=std::end(in);
                std::get<1>(input_tup)++;
            }
        }

        if (std::signed_integral<OutType> and !get_sign()){
            DEBUG << "The Type " + std::string(type_name<OutType>()) + " was not signed! Decoding error!" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        if (std::unsigned_integral<OutType> and get_sign()){
            DEBUG << "The Type " + std::string(type_name<OutType>()) + " was not unsigned! Decoding error!" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        fundamental As_type;
        if constexpr (std::is_same_v<bool,OutType> or std::is_same_v<char,OutType> or std::is_same_v<unsigned char,OutType>){
            As_type=char_type;
        }
        else{
            if constexpr (std::is_same_v<short,OutType> or std::is_same_v<unsigned short,OutType>){
                As_type=short_type;
            }
            else{
                if constexpr (std::is_same_v<int,OutType> or std::is_same_v<unsigned int,OutType>){
                    As_type=int_type;
                }
                else{
                    if constexpr (std::is_same_v<long int,OutType> or std::is_same_v<unsigned long int,OutType>){
                        As_type=long_type;
                    }
                    else{
                        if constexpr (std::is_same_v<float,OutType>){
                            As_type=float_type;
                        }
                        else{
                            if constexpr (std::is_same_v<double,OutType>){
                                As_type=double_type;
                            }
                            else{
                                if constexpr (std::is_same_v<long double,OutType>){
                                    As_type=long_double_type;
                                }
                                else{
                                    As_type=string_type;
                                }
                            }
                        }
                    }
                }
            }
        }

        if(As_type!=foundType){
            DEBUG << "The saved type " + std::string(type_name<OutType>()) + " of the encoding did not match the output wish!" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        if constexpr (ArithmeticFundamental<OutType>){
            //TODO: implement long double case with 10 bytes
            if(sizeof(OutType) != mem_size)[[unlikely]]{
                DEBUG << "The type " + std::string(type_name<OutType>()) + " that should be returned by the decoder did not fit the saved type!" << std::endl;
                std::exit(EXIT_FAILURE);
            }
            else[[likely]]{
                OutType n = *reinterpret_cast<OutType*>(internalBuffer);
                if constexpr (SequentialContainer<SomeStream>){
                    return std::make_tuple(n,std::get<1>(input_tup));
                }
                else{
                    return std::make_tuple(n);
                }
            }
        }
        else{
            if constexpr(std::is_same_v<OutType,unsigned char*> or std::is_same_v<OutType,char*> or std::is_same_v<OutType,bool*>){
                std::ostringstream oss;
                oss.write(reinterpret_cast<const char *>(internalBuffer), static_cast<long>(mem_size));
                if constexpr (SequentialContainer<SomeStream>){
                    return std::make_tuple(reinterpret_cast<OutType*>(oss.str().data()), std::get<1>(input_tup));
                }
                else{
                    return std::make_tuple(reinterpret_cast<OutType*>(oss.str().data()));
                }
            }
            else{
                if constexpr(String<OutType>){
                    std::ostringstream oss;
                    oss.write(reinterpret_cast<const char *>(internalBuffer), static_cast<long>(mem_size));

                    if constexpr (SequentialContainer<SomeStream>){
                        return std::make_tuple(oss.str(),std::get<1>(input_tup));
                    }
                    else{
                        return std::make_tuple(oss.str());
                    }
                }
                else{
                    if constexpr(SequentialContainer<OutType>){
                        using innerType=boost::mp11::mp_first<OutType>;
                        if constexpr (!ArithmeticFundamental<innerType>){
                            BOOST_STATIC_ASSERT_MSG("Error: The inner type " + std::string(type_name<OutType>()) + " of the sequential container was not a fundamental type!");
                        }
                        if(mem_size==0)[[unlikely]]{
                            if constexpr (SequentialContainer<SomeStream>){
                                return std::make_tuple(OutType{},std::get<1>(input_tup));
                            }
                            else{
                                return std::make_tuple(OutType{});
                            }
                        }
                        //const std::size_t numBytes=As_type==double_type?10:sizeof(innerType);
                        //TODO: read from a compressed container of long doubles, not implemented yet
                        if constexpr(std::is_same_v<long double,innerType>){
                            if(mem_size%10!=0)[[unlikely]]{
                                DEBUG << "The inner type " + std::string(type_name<OutType>()) + " of the expected container " + std::string(type_name<OutType>()) + " must be wrong since the modulo check of length failed while decoding!" << std::endl;
                                std::exit(EXIT_FAILURE);
                            }
                        }
                        else{
                            if(mem_size%sizeof(innerType)!=0)[[unlikely]]{
                                DEBUG << "The inner type " + std::string(type_name<OutType>()) + " of the expected container " + std::string(type_name<OutType>()) + " must be wrong since the modulo check of length failed while decoding!" << std::endl;
                                std::exit(EXIT_FAILURE);
                            }
                        }

                        OutType output{internalBuffer,internalBuffer+mem_size};

                        if constexpr (SequentialContainer<SomeStream>){
                            return std::make_tuple(output,std::get<1>(input_tup));
                        }
                        else{
                            return std::make_tuple(output);
                        }
                    }
                    else{
                        BOOST_STATIC_ASSERT("The output type of decode was unknown!");
                    }
                }
            }
        }
        ERROR << "The type " + std::string(type_name<OutType>()) + " to be returned was unknown by the encoder!" << std::endl;
        if constexpr (SequentialContainer<SomeStream>){
            return std::make_tuple(OutType{}, std::get<1>(input_tup));
        }
        else{
            return std::make_tuple(OutType{});
        }
    }

};


#endif //CMAKE_BUILD_DEBUG_ULTIHASH_ENCODER_H
