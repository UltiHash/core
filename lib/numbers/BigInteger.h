//
// Created by max on 28.10.22.
//

#ifndef INC_21_NUMBERS_BIGINTEGER_H
#define INC_21_NUMBERS_BIGINTEGER_H

#include "BigBasicsCustom.h"

namespace ultihash::numbers {

    class BigInteger : virtual public ultihash::basics::BigBasicsCustom<std::size_t> {
    protected:
        std::size_t len{};
        bool is_minus = false;
        std::size_t *data{};

    public:

        BigInteger(std::size_t *array, std::size_t len, bool is_minus = false, bool only_assign = false) : BigBasicsCustom() {
            this->len = len;
            std::free(this->data);
            if(not only_assign){
                this->data = (std::size_t *)std::malloc((this->len + 1) * sizeof(std::size_t));
                std::memcpy(this->data, array, this->len * sizeof(std::size_t));
                this->data[this->len-1]=0;
            }
            else{
                this->data = array;
            }

            this->is_minus = is_minus;
        }

        explicit BigInteger(const std::string& value, bool is_minus = false) : BigBasicsCustom() {
            //<0x,0o,0b starting> or <<BigInt>mod<BigInt>>, only capital letters; duplicate minus is not allowed
            this->is_minus=is_minus;
            if(value.empty())throw basics::BigIntegerStringInterpretException();

            *this=BigInteger();
            auto pos_mod = value.find_last_of("mod");
            if(value.cbegin()+static_cast<long long>(pos_mod)+3 == value.cend() or pos_mod==0)throw basics::BigIntegerStringInterpretException();
            BigInteger mod(10);
            BigInteger shift_mod(1);
            //Build forward interpreter of numbers that are readable from mod range
            decltype(value.cbegin()) value_end=value.cbegin()+std::max((long long)0,(long long)std::min(pos_mod,value.size()));
            std::string value_string{value.cbegin(),value_end};
            if(value_string.starts_with('-')){
                this->is_minus=not this->is_minus;
                value_string=value_string.substr(1,value_string.size()-1);
            }
            std::string prefix;
            if(value_string.starts_with("0x") or value_string.starts_with("0b") or value_string.starts_with("0o")){
                prefix=std::string{value_string.cbegin(),value_string.cbegin()+2};
                value_string=value_string.substr(2,value_string.size()-1);
            }
            if(value_string.starts_with('-')){
                this->is_minus=not this->is_minus;
                value_string=value_string.substr(1,value_string.size()-1);
            }
            while(value_string.starts_with('0'))value_string=value_string.substr(1,value_string.size()-1);
            pos_mod = value_string.find_last_of("mod");
            if(pos_mod==std::numeric_limits<std::size_t>::max() or static_cast<long long>(pos_mod)+3>=value_string.size()){
                bool change_mod=false;
                if(prefix.starts_with("0x")){
                    if(std::all_of(value_string.cbegin(),value_string.cend(),[](auto c){
                        return ('0'<=c and c<='9') or ('A'<=c and c<= 'F');
                    })){
                        mod=BigInteger(16);
                        change_mod=true;
                    }
                    else{
                        throw basics::BigIntegerStringInterpretException();
                    }
                }
                else{
                    if(prefix.starts_with("0b")){
                        if(std::all_of(value_string.cbegin(),value_string.cend(),[](auto c){
                            return ('0'<=c and c<='1');
                        })){
                            mod=BigInteger(2);
                            change_mod=true;
                        }
                        else{
                            throw basics::BigIntegerStringInterpretException();
                        }
                    }
                    else{
                        if(prefix.starts_with("0o")){
                            if(std::all_of(value_string.cbegin()+2,value_string.cend(),[](auto c){
                                return ('0'<=c and c<='8');
                            })){
                                mod=BigInteger(8);
                                change_mod=true;
                            }
                            else{
                                throw basics::BigIntegerStringInterpretException();
                            }
                        }
                        //else mod is 10
                    }
                }

                const std::string val_max = std::to_string(std::numeric_limits<std::size_t>::max());
                bool smaller=!change_mod and value_string.size()<=val_max.size();
                for(auto i=value_string.cbegin(),j=val_max.cbegin();i<value_string.cend() and smaller;i++,j++){
                    if(*i>*j){
                        smaller=false;
                        break;
                    }
                    if(*i==*j)continue;
                    if(*i<*j)break;
                }
                if(!change_mod and (value_string.size()<val_max.size() or (value_string.size()==val_max.size() and smaller))){
                    *this+=(std::size_t)std::stoull(value_string);
                    return;
                }

                char multiply;
                for(auto i=value_string.cend();i>=value_string.cbegin();--i){
                    if(*i=='0'){
                        shift_mod*=mod;
                        continue;
                    }
                    multiply=0;
                    if('0'<=*i and *i<='9')multiply+=*i-'0';
                    if('A'<=*i and *i<='F')multiply+=10+*i-'A';
                    *this+=shift_mod*multiply;
                    shift_mod*=mod;
                }

                return;
            }

            if(value_string.cbegin()+static_cast<long long>(pos_mod)<value_string.cend())throw basics::BigIntegerStringInterpretException();
            std::string mod_string{value_string.cbegin()+static_cast<long long>(pos_mod)+3,value_string.cend()};

            std::size_t first_len=value_string.size()%mod_string.size();
            if(first_len==0)first_len=mod_string.size();
            mod=BigInteger(mod_string);
            bool first=true;

            for(auto i=value_string.cbegin();i<value_string.cend();){
                decltype(value_string.cbegin()) endit=first?i+static_cast<long long>(first_len):i+static_cast<long long>(mod_string.size());
                BigInteger multiply(prefix+std::string{i,endit});
                if(multiply>=mod)throw basics::BigIntegerStringInterpretException();

                i+=first?static_cast<long long>(first_len):static_cast<long long>(pos_mod);
                if(first)first=false;

                if(multiply==0){
                    shift_mod*=mod;
                    continue;
                }

                *this+=shift_mod*multiply;
                shift_mod*=mod;
            }
        }

        template<typename Fundamental, std::enable_if_t<std::is_arithmetic<Fundamental>::value, bool> = true>
        explicit BigInteger(Fundamental in) : BigBasicsCustom() {
            if constexpr (sizeof(in) > sizeof(std::size_t)) {
                std::cerr<<"Calculation width of " + std::to_string(sizeof(in)) + " bytes not supported, but only " +
                std::to_string(sizeof(std::size_t)) + " bytes!"<<std::endl;
                return;
            }
            if constexpr (std::is_signed<Fundamental>::value) {
                this->is_minus = in < 0;
                if (in < 0)in = std::abs(in);
            }
            if constexpr (std::is_floating_point<Fundamental>::value) {
                in = std::round(in);
            }
            auto out = static_cast<std::size_t>(in);
            this->len = out!=0;
            this->data = (std::size_t*)std::malloc((this->len+1) * sizeof(std::size_t));
            this->data[0]=out;
            this->data[this->len-1]=0;
        }

        BigInteger() {
            this->len = 0;
            this->data = (std::size_t*)std::malloc(sizeof(std::size_t));
            this->data[0]=0;
        }

        template<typename Fundamental, std::enable_if_t<std::is_same<Fundamental, BigInteger>::value, bool> = true>
        explicit BigInteger(const Fundamental &in) : BigBasicsCustom() {
            *this=in;
        }

        BigInteger operator+() {
            return *this;
        }

        BigInteger operator-() {
            is_minus = not is_minus;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator=(const InType& rhs) {
            if constexpr (std::is_same_v<std::string,InType>){
                *this=BigInteger(rhs);
                return *this;
            }
            std::free(this->data);
            this->data = (std::size_t *)std::malloc((rhs.len + 1) * sizeof(std::size_t));
            std::memcpy(this->data, rhs.data, rhs.len*sizeof(std::size_t));
            this->data[this->len-1]=0;
            this->is_minus = rhs.is_minus;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::is_arithmetic<InType>::value, bool> = true>
        BigInteger &operator=(InType rhs) {
            *this=BigInteger(rhs);
            return *this;
        }

        //basic arithmetic operators
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator+(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                if(lhs.is_minus==rhs.is_minus){
                    out = ultihash::numbers::BigInteger::plus(lhs.data, lhs.len, rhs.data, rhs.len);
                }
                else{
                    if(gt(lhs.data, lhs.len, rhs.data, rhs.len)){
                        out = ultihash::numbers::BigInteger::minus(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                    else{
                        out = ultihash::numbers::BigInteger::minus(rhs.data, rhs.len, lhs.data, lhs.len);
                        return {std::get<0>(out),std::get<1>(out),static_cast<bool>(not lhs.is_minus),true};
                    }
                }
            }
            else{
                BigInteger tmp2(rhs);
                if(lhs.is_minus==tmp2.is_minus){
                    out = ultihash::numbers::BigInteger::plus(lhs.data, lhs.len, tmp2.data, tmp2.len);
                }
                else{
                    if(gt(lhs.data, lhs.len, tmp2.data, tmp2.len)){
                        out = ultihash::numbers::BigInteger::minus(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                    else{
                        out = ultihash::numbers::BigInteger::minus(tmp2.data, tmp2.len, lhs.data, lhs.len);
                        return {std::get<0>(out),std::get<1>(out),static_cast<bool>(not lhs.is_minus),true};
                    }
                }
            }
            return {std::get<0>(out),std::get<1>(out),lhs.is_minus,true};
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator-(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                if(lhs.is_minus!=rhs.is_minus){
                    out = ultihash::numbers::BigInteger::plus(lhs.data, lhs.len, rhs.data, rhs.len);
                }
                else{
                    if(gt(lhs.data, lhs.len, rhs.data, rhs.len)){
                        out = ultihash::numbers::BigInteger::minus(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                    else{
                        out = ultihash::numbers::BigInteger::minus(rhs.data, rhs.len, lhs.data, lhs.len);
                        return {std::get<0>(out),std::get<1>(out),static_cast<bool>(not lhs.is_minus),true};
                    }
                }
            }
            else{
                BigInteger tmp2(rhs);
                if(lhs.is_minus!=tmp2.is_minus){
                    out = ultihash::numbers::BigInteger::plus(lhs.data, lhs.len, tmp2.data, tmp2.len);
                }
                else{
                    if(gt(lhs.data, lhs.len, tmp2.data, tmp2.len)){
                        out = ultihash::numbers::BigInteger::minus(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                    else{
                        out = ultihash::numbers::BigInteger::minus(tmp2.data, tmp2.len, lhs.data, lhs.len);
                        return {std::get<0>(out),std::get<1>(out),static_cast<bool>(not lhs.is_minus),true};
                    }
                }
            }
            return {std::get<0>(out),std::get<1>(out),lhs.is_minus,true};
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator*(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::multiply(lhs.data, lhs.len, rhs.data, rhs.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus xor rhs.is_minus),true};
            }
            else{
                BigInteger tmp2(rhs);
                out = ultihash::numbers::BigInteger::multiply(lhs.data, lhs.len, tmp2.data, tmp2.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus xor tmp2.is_minus),true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator/(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::tuple<std::size_t*,std::size_t>,std::tuple<std::size_t*,std::size_t>> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::divmod(lhs.data, lhs.len, rhs.data, rhs.len);
                std::free(std::get<0>(std::get<1>(out)));
                return {std::get<0>(std::get<0>(out)),std::get<1>(std::get<0>(out)),static_cast<bool>(lhs.is_minus xor rhs.is_minus),true};
            }
            else{
                BigInteger tmp2(rhs);
                out = ultihash::numbers::BigInteger::divmod(lhs.data, lhs.len, tmp2.data, tmp2.len);
                std::free(std::get<0>(std::get<1>(out)));
                return {std::get<0>(std::get<0>(out)),std::get<1>(std::get<0>(out)),static_cast<bool>(lhs.is_minus xor tmp2.is_minus),true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator%(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::tuple<std::size_t*,std::size_t>,std::tuple<std::size_t*,std::size_t>> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::divmod(lhs.data, lhs.len, rhs.data, rhs.len);
                std::free(std::get<0>(std::get<0>(out)));
                return {std::get<0>(std::get<0>(out)),std::get<1>(std::get<0>(out)),static_cast<bool>(lhs.is_minus xor rhs.is_minus),true};
            }
            else{
                BigInteger tmp2(rhs);
                out = ultihash::numbers::BigInteger::divmod(lhs.data, lhs.len, tmp2.data, tmp2.len);
                std::free(std::get<0>(std::get<0>(out)));
                return {std::get<0>(std::get<0>(out)),std::get<1>(std::get<0>(out)),static_cast<bool>(lhs.is_minus xor tmp2.is_minus),true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator&(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::and_func(lhs.data, lhs.len, rhs.data, rhs.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus and rhs.is_minus),true};
            }
            else{
                BigInteger tmp2(rhs);
                out = ultihash::numbers::BigInteger::and_func(lhs.data, lhs.len, tmp2.data, tmp2.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus xor tmp2.is_minus),true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator|(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::or_func(lhs.data, lhs.len, rhs.data, rhs.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus or rhs.is_minus),true};
            }
            else{
                BigInteger tmp2(rhs);
                out = ultihash::numbers::BigInteger::or_func(lhs.data, lhs.len, tmp2.data, tmp2.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus or tmp2.is_minus),true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator^(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::xor_func(lhs.data, lhs.len, rhs.data, rhs.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus xor rhs.is_minus),true};
            }
            else{
                BigInteger tmp2(rhs);
                out = ultihash::numbers::BigInteger::xor_func(lhs.data, lhs.len, tmp2.data, tmp2.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(lhs.is_minus xor tmp2.is_minus),true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator~(InType &lhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::complement_func(lhs.data, lhs.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(not lhs.is_minus),true};
            }
            else{
                BigInteger tmp2(lhs);
                out = ultihash::numbers::BigInteger::complement_func(tmp2.data, tmp2.len);
                return {std::get<0>(out),std::get<1>(out),static_cast<bool>(not tmp2.is_minus),true};
            }

            return {std::get<0>(out),std::get<1>(out),lhs.is_minus,true};
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator<<(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::shiftl(lhs.data, lhs.len, static_cast<std::size_t>(*rhs.data));
            }
            else{
                BigInteger tmp2(lhs);
                out = ultihash::numbers::BigInteger::shiftl(lhs.data, lhs.len, static_cast<std::size_t>(*tmp2.data));
            }

            return {std::get<0>(out),std::get<1>(out),lhs.is_minus,true};
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator>>(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::size_t*,std::size_t> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = ultihash::numbers::BigInteger::shiftr(lhs.data, lhs.len, static_cast<std::size_t>(*rhs.data));
            }
            else{
                BigInteger tmp2(lhs);
                out = ultihash::numbers::BigInteger::shiftr(lhs.data, lhs.len, static_cast<std::size_t>(*tmp2.data));
            }

            return {std::get<0>(out),std::get<1>(out),lhs.is_minus,true};
        }

        //compound assignment variants of binary arithmetic operators
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator+=(const InType &rhs) {
            *this = *this + rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator-=(const InType &rhs) {
            *this = *this - rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator*=(const InType &rhs) {
            *this = *this * rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator/=(const InType &rhs) {
            *this = *this / rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator%=(const InType &rhs) {
            *this = *this % rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator&=(const InType &rhs) {
            *this = *this & rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator|=(const InType &rhs) {
            *this = *this | rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator^=(const InType &rhs) {
            *this = *this ^ rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator<<=(const InType &rhs) {
            *this = *this << rhs;
            return *this;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        BigInteger &operator>>=(const InType &rhs) {
            *this = *this >> rhs;
            return *this;
        }

        //comparison operators
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator<(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                if(lhs.len==0 and rhs.len==0)return false;
                if(lhs.is_minus!=rhs.is_minus){
                    if(not lhs.is_minus){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::lt(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::gt(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                }
            }
            else{
                BigInteger tmp2(lhs);
                if(lhs.len==0 and tmp2.len==0)return false;
                if(lhs.is_minus!=tmp2.is_minus){
                    if(not lhs.is_minus){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::lt(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::gt(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                }
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator>(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                if(lhs.len==0 and rhs.len==0)return false;
                if(lhs.is_minus!=rhs.is_minus){
                    if(not lhs.is_minus){
                        return false;
                    }
                    else{
                        return true;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::gt(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::lt(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                }
            }
            else{
                BigInteger tmp2(lhs);
                if(lhs.len==0 and tmp2.len==0)return false;
                if(lhs.is_minus!=tmp2.is_minus){
                    if(not lhs.is_minus){
                        return false;
                    }
                    else{
                        return true;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::gt(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::lt(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                }
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator<=(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                if(lhs.len==0 and rhs.len==0)return true;
                if(lhs.is_minus!=rhs.is_minus){
                    if(not lhs.is_minus){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::lte(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::gte(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                }
            }
            else{
                BigInteger tmp2(lhs);
                if(lhs.len==0 and tmp2.len==0)return true;
                if(lhs.is_minus!=tmp2.is_minus){
                    if(not lhs.is_minus){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::lte(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::gte(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                }
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator>=(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                if(lhs.len==0 and rhs.len==0)return true;
                if(lhs.is_minus!=rhs.is_minus){
                    if(not lhs.is_minus){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::lte(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::gte(lhs.data, lhs.len, rhs.data, rhs.len);
                    }
                }
            }
            else{
                BigInteger tmp2(lhs);
                if(lhs.len==0 and tmp2.len==0)return true;
                if(lhs.is_minus!=tmp2.is_minus){
                    if(not lhs.is_minus){
                        return true;
                    }
                    else{
                        return false;
                    }
                }
                else{
                    if(not lhs.is_minus){
                        return ultihash::numbers::BigInteger::lte(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                    else{
                        return ultihash::numbers::BigInteger::gte(lhs.data, lhs.len, tmp2.data, tmp2.len);
                    }
                }
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator==(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                return lhs.len==rhs.len and lhs.is_minus==rhs.is_minus and ultihash::numbers::BigInteger::equ(lhs.data, lhs.len, rhs.data, rhs.len);
            }
            else{
                BigInteger tmp2(lhs);
                return lhs.len==tmp2.len and lhs.is_minus==tmp2.is_minus and ultihash::numbers::BigInteger::equ(lhs.data, lhs.len, tmp2.data, tmp2.len);
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator!=(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                return lhs.len!=rhs.len or lhs.is_minus!=rhs.is_minus or ultihash::numbers::BigInteger::ne(lhs.data, lhs.len, rhs.data, rhs.len);
            }
            else{
                BigInteger tmp2(lhs);
                return lhs.len!=tmp2.len or lhs.is_minus!=tmp2.is_minus or ultihash::numbers::BigInteger::ne(lhs.data, lhs.len, tmp2.data, tmp2.len);
            }
        }

        BigInteger &operator++() {          //prefix increment
            *this += 1;
            return *this;
        }

        const BigInteger operator++(int) {      //postfix increment
            BigInteger tmp = *this;
            *this += 1;
            return tmp;
        }

        BigInteger &operator--() {        //prefix decrement
            *this -= 1;
            return *this;
        }

        const BigInteger operator--(int) {      //postfix decrement
            BigInteger tmp = *this;
            *this -= 1;
            return tmp;
        }

        template<typename InType=unsigned int>
        std::string as_string(const InType& mod=10) {
            BigInteger mod_big(mod);
            if(this->size()==1 and mod_big==10){
                return std::to_string(this->data[0]);
            }
            std::string out;
            std::size_t mod_str_len=(mod_big-1).as_string().size();
            BigInteger divisor(mod);
            //BigInteger power(1);

            while(divisor < *this){
                divisor*=mod_big;
                //power++;
            }
            if(divisor > *this){
                divisor/=mod_big;
                //power--;
            }

            auto dividend=BigInteger(std::get<0>(this->as_array_ref()),std::get<1>(this->as_array_ref()));
            std::tuple<std::tuple<std::size_t*, std::size_t>, std::tuple<std::size_t*, std::size_t>> div;
            bool check_gt;

            do{
                div=divmod(std::get<0>(dividend.as_array_ref()),std::get<1>(dividend.as_array_ref()),std::get<0>(divisor.as_array_ref()),std::get<1>(divisor.as_array_ref()));
                divisor/=mod_big;
                //power--;
                std::string app_str=BigInteger(std::get<0>(std::get<0>(div)),std::get<1>(std::get<0>(div))).as_string();
                while(app_str.size()<mod_str_len){
                    app_str.insert(app_str.cbegin(),'0');
                }
                out+=app_str;
                dividend=BigInteger(std::get<0>(std::get<1>(div)),std::get<1>(std::get<1>(div)));
                check_gt = dividend > divisor;
                if(check_gt){
                    std::free(std::get<0>(std::get<0>(div)));
                    std::free(std::get<0>(std::get<1>(div)));
                }
            }while (check_gt);

            std::string app_str=BigInteger(std::get<0>(std::get<1>(div)),std::get<1>(std::get<1>(div))).as_string();
            while(app_str.size()<mod_str_len){
                app_str.insert(app_str.cbegin(),'0');
            }
            out+=app_str;

            std::free(std::get<0>(std::get<0>(div)));
            std::free(std::get<0>(std::get<1>(div)));

            if(mod_big!=10){
                out+="mod"+mod_big.as_string();
            }

            return out;
        }

        [[nodiscard]] bool sig() const{
            return is_minus;
        }

        [[nodiscard]] std::size_t size() const{
            return this->len;
        }

        std::size_t operator[](std::size_t at){
            if(at>this->len)throw basics::OutOfBoundsException();
            return this->data[at];
        }

        std::tuple<std::size_t*,std::size_t> as_array_ref() {
            return std::make_tuple(this->data,this->len);
        }

        ~BigInteger() {
            std::free(this->data);
        }
    };

}

#endif //INC_21_NUMBERS_BIGINTEGER_H
