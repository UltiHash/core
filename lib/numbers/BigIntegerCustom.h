
//
// Created by max on 28.10.22.
//

#ifndef INC_21_NUMBERS_BIGINTEGERCUSTOM_H
#define INC_21_NUMBERS_BIGINTEGERCUSTOM_H

#include <cstddef>
#include <algorithm>
#include <tuple>
#include <bit>
#include <cmath>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "boost/algorithm/string.hpp"

namespace uh::numbers {

    struct UnderflowSubtractException : public std::exception {
        [[nodiscard]] const char *what() const noexcept override {
            return "Underflow at BigInteger minus detected!";
        }
    };

    struct OutOfBoundsException : public std::exception {
        [[nodiscard]] const char *what() const noexcept override {
            return "Out of bounds at BigInteger operator[] detected!";
        }
    };

    struct BigIntegerStringInterpretException : public std::exception {
        [[nodiscard]] const char *what() const noexcept override {
            return "While converting a string into an BigInteger an Error occurred!";
        }
    };

    template<typename LIMB_TYPE = std::size_t>
    class BigInteger {
    protected:
        std::unique_ptr<LIMB_TYPE[]> data;
        std::size_t len{};
        std::size_t lenAllocated{};

        void swap(BigInteger &in) noexcept {
            std::swap(this->data, in.data);
            std::swap(this->len, in.len);
            std::swap(this->lenAllocated, in.lenAllocated);
        }

        BigInteger(BigInteger const &integer) {
            this->lenAllocated = roundUpToVectorLength<std::size_t>(integer.len);
            this->data.reset(new (std::align_val_t(getAlignment())) std::size_t[this->lenAllocated]{});
            this->len = integer.len;

            std::memcpy(this->data.get(), integer.data.get(), this->len * sizeof(std::size_t));
        }

        // To figure out the ideal alignment for the target hardware, this information
        // is queried using a function to allow replacing magic numbers with dynamic detection in the future.
        // The alignment should consider cache line size, width of SIMD registers, and page size
        static constexpr std::size_t getAlignment() {
            return 64; // this value assuming AVX512 for now
        }

        // roundUp requires multiple to be a power of 2
        template<typename Fundamental, std::enable_if_t<std::conjunction<std::is_integral<Fundamental>,std::is_unsigned<Fundamental>>::value, bool> = true>
        static constexpr std::size_t roundUpToVectorLength(int64_t num, int64_t multiple = getAlignment()) {
            multiple /= sizeof(Fundamental);
            assert(multiple && ((multiple & (multiple - 1)) == 0));
            return static_cast<std::size_t>((num + multiple - 1) & -multiple);
        }

    public:
        BigInteger() = default;

        /*
        BigInteger(std::size_t *data, std::size_t len, bool sign = false) : BigBasicsCustom() {
            this->len = len;
            this->sign = sign;
            this->data.reset(new (std::align_val_t(ALIGNMENT_SIZE)) std::size_t[this->len + 1]);
            //todo: round up allocation size to next multiple of alignment for simplified SIMD/GPU acceleration

            std::memcpy(this->data.get(), data, this->len * sizeof(std::size_t));
        }
         */

        BigInteger(const std::string& value, bool sign = false) {
            //TODO: make this a non-dummy

        }


//        explicit BigInteger(const std::string& value, bool is_minus = false) : BigBasicsCustom() {
//            //<0x,0o,0b starting> or <<BigInt>mod<BigInt>>, only capital letters; duplicate minus is not allowed
//            this->sign=is_minus;
//            if(value.empty())throw basics::BigIntegerStringInterpretException();
//
//            *this=BigInteger();
//            auto pos_mod = value.find_last_of("mod");
//            if(value.cbegin()+static_cast<long long>(pos_mod)+3 == value.cend() or pos_mod==0)throw basics::BigIntegerStringInterpretException();
//            BigInteger mod(10);
//            BigInteger shift_mod(1);
//            //Build forward interpreter of numbers that are readable from mod range
//            decltype(value.cbegin()) value_end=value.cbegin()+std::max((long long)0,(long long)std::min(pos_mod,value.size()));
//            std::string value_string{value.cbegin(),value_end};
//            if(value_string.starts_with('-')){
//                this->sign=not this->sign;
//                value_string=value_string.substr(1,value_string.size()-1);
//            }
//            std::string prefix;
//            if(value_string.starts_with("0x") or value_string.starts_with("0b") or value_string.starts_with("0o")){
//                prefix=std::string{value_string.cbegin(),value_string.cbegin()+2};
//                value_string=value_string.substr(2,value_string.size()-1);
//            }
//            if(value_string.starts_with('-')){
//                this->sign=not this->sign;
//                value_string=value_string.substr(1,value_string.size()-1);
//            }
//            while(value_string.starts_with('0'))value_string=value_string.substr(1,value_string.size()-1);
//            pos_mod = value_string.find_last_of("mod");
//            if(pos_mod==std::numeric_limits<std::size_t>::max() or static_cast<long long>(pos_mod)+3>=value_string.size()){
//                bool change_mod=false;
//                if(prefix.starts_with("0x")){
//                    if(std::all_of(value_string.cbegin(),value_string.cend(),[](auto c){
//                        return ('0'<=c and c<='9') or ('A'<=c and c<= 'F');
//                    })){
//                        mod=BigInteger(16);
//                        change_mod=true;
//                    }
//                    else{
//                        throw basics::BigIntegerStringInterpretException();
//                    }
//                }
//                else{
//                    if(prefix.starts_with("0b")){
//                        if(std::all_of(value_string.cbegin(),value_string.cend(),[](auto c){
//                            return ('0'<=c and c<='1');
//                        })){
//                            mod=BigInteger(2);
//                            change_mod=true;
//                        }
//                        else{
//                            throw basics::BigIntegerStringInterpretException();
//                        }
//                    }
//                    else{
//                        if(prefix.starts_with("0o")){
//                            if(std::all_of(value_string.cbegin()+2,value_string.cend(),[](auto c){
//                                return ('0'<=c and c<='8');
//                            })){
//                                mod=BigInteger(8);
//                                change_mod=true;
//                            }
//                            else{
//                                throw basics::BigIntegerStringInterpretException();
//                            }
//                        }
//                        //else mod is 10
//                    }
//                }
//
//                const std::string val_max = std::to_string(std::numeric_limits<std::size_t>::max());
//                bool smaller=!change_mod and value_string.size()<=val_max.size();
//                for(auto i=value_string.cbegin(),j=val_max.cbegin();i<value_string.cend() and smaller;i++,j++){
//                    if(*i>*j){
//                        smaller=false;
//                        break;
//                    }
//                    if(*i==*j)continue;
//                    if(*i<*j)break;
//                }
//                if(!change_mod and (value_string.size()<val_max.size() or (value_string.size()==val_max.size() and smaller))){
//                    *this+=(std::size_t)std::stoull(value_string);
//                    return;
//                }
//
//                char multiply;
//                for(auto i=value_string.cend();i>=value_string.cbegin();--i){
//                    if(*i=='0'){
//                        shift_mod*=mod;
//                        continue;
//                    }
//                    multiply=0;
//                    if('0'<=*i and *i<='9')multiply+=*i-'0';
//                    if('A'<=*i and *i<='F')multiply+=10+*i-'A';
//                    *this+=shift_mod*multiply;
//                    shift_mod*=mod;
//                }
//
//                return;
//            }
//
//            if(value_string.cbegin()+static_cast<long long>(pos_mod)<value_string.cend())throw basics::BigIntegerStringInterpretException();
//            std::string mod_string{value_string.cbegin()+static_cast<long long>(pos_mod)+3,value_string.cend()};
//
//            std::size_t first_len=value_string.size()%mod_string.size();
//            if(first_len==0)first_len=mod_string.size();
//            mod=BigInteger(mod_string);
//            bool first=true;
//
//            for(auto i=value_string.cbegin();i<value_string.cend();){
//                decltype(value_string.cbegin()) endit=first?i+static_cast<long long>(first_len):i+static_cast<long long>(mod_string.size());
//                BigInteger multiply(prefix+std::string{i,endit});
//                if(multiply>=mod)throw basics::BigIntegerStringInterpretException();
//
//                i+=first?static_cast<long long>(first_len):static_cast<long long>(pos_mod);
//                if(first)first=false;
//
//                if(multiply==0){
//                    shift_mod*=mod;
//                    continue;
//                }
//
//                *this+=shift_mod*multiply;
//                shift_mod*=mod;
//            }
//        }

        template<typename Fundamental, std::enable_if_t<std::is_arithmetic<Fundamental>::value, bool> = true>
        explicit BigInteger(Fundamental in) {
            if constexpr (sizeof(in) > sizeof(std::size_t)) {
                std::cerr<<"Limb width of " + std::to_string(sizeof(in)) + " bytes not supported, but only " +
                           std::to_string(sizeof(std::size_t)) + " bytes!"<<std::endl;

                return;
            }

            this->lenAllocated = roundUpToVectorLength<std::size_t>(getAlignment() / sizeof(std::size_t));
            this->data.reset(new (std::align_val_t(getAlignment())) std::size_t[this->lenAllocated]{});
            this->len = 1;

            if constexpr (std::is_signed_v<Fundamental>) {
                this->data[0] = std::abs(in);
            }
            if constexpr (std::is_unsigned_v<Fundamental>) {
                this->data[0] = in;
            }
            if constexpr (std::is_floating_point_v<Fundamental>) {
                this->data[0] = static_cast<std::size_t>(in);
            }
        }

        //basic self assignment
        BigInteger operator+() {
            return *this;
        }

        /*
        BigInteger operator-() {
            sign = not sign;
            return *this;
        }
        */

        template<typename InType, std::enable_if_t<std::disjunction<std::is_same<InType, BigInteger>,std::is_same<InType, std::string>, std::is_arithmetic<InType>>::value, bool> = true>
        BigInteger &operator=(const InType& rhs) {
                BigInteger tmp(rhs);
                swap(tmp);
                return *this;
        }


        BigInteger &operator=(const BigInteger& rhs) {
            BigInteger tmp(rhs);
            swap(tmp);
            return *this;
        }

        //basic arithmetic operators
        //TODO: Why not change the function prototype to BigInteger operator+(BigInteger&, const BigInteger&) and use the compiler to construct the rhs? This way we duplicate the same functionality and have increased effort on maintenance.
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator+(BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            const BigInteger &a = lhs.len >= rhs.len ? lhs : rhs;
            const BigInteger &b = lhs.len >= rhs.len ? rhs : lhs;
            BigInteger c;

            // allocate space for result c
            c.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(std::max(a.len, b.len) + 1);
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.lenAllocated]{});

            bool overflow_detect = false;
            for (std::size_t i = 0; i < std::max(a.len, b.len) or overflow_detect; ++i) {
                LIMB_TYPE aTmp = (i < a.len ? a.data[i] : 0);
                LIMB_TYPE bTmp = (i < b.len ? b.data[i] : 0);
                c.data[i] = aTmp + bTmp + overflow_detect;
                // use pre-condition overflow check in order not to include the
                overflow_detect = ((std::numeric_limits<LIMB_TYPE>::max() - aTmp < bTmp)
                                   or ((aTmp + bTmp == std::numeric_limits<LIMB_TYPE>::max()) and overflow_detect)) ? 1 : 0;
            }

            size_t lastElement;
            for(lastElement = c.lenAllocated - 1; c.data[lastElement] == 0; --lastElement);
            c.len = lastElement + 1;
            return c;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator-(const BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            if (lhs.len < rhs.len or operator<(lhs,rhs))
                throw UnderflowSubtractException();
            BigInteger c;

            // allocate space for result c
            c.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(lhs.len);
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.lenAllocated]{});

            bool underflow_detect = false;
            for (std::size_t i = 0; i < lhs.len; ++i) {
                c.data[i] = lhs.data[i] - (i < rhs.len ? rhs.data[i] : 0) - underflow_detect;
                underflow_detect = c.data[i] > lhs.data[i];
            }
            if (underflow_detect)
                throw UnderflowSubtractException();

            size_t lastElement = c.lenAllocated - 1;
            for(; c.data[lastElement] == 0; --lastElement);
            c.len = lastElement + 1;
            return c;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator*(BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            //TODO: use 4 buffer caches in main function to split main algo to 3 main loops and use SIMD more straight forward
            //TODO: shiftr if the size is 1 and is an exponent of 2
            if ((lhs.len == 1 and lhs[0] == 0) or (rhs.len == 1 and rhs[0] == 0))
                return BigInteger(0);
            BigInteger c;

            const unsigned char shift_var = (sizeof(LIMB_TYPE) * 8 / 2);
            LIMB_TYPE mask_low = std::numeric_limits<LIMB_TYPE>::max() >> shift_var;
            LIMB_TYPE mask_high = mask_low << shift_var;

            c.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(lhs.len + rhs.len);
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.lenAllocated]{});

            if (lhs.len > rhs.len) {
                LIMB_TYPE low_low_tmp, low_high_tmp, high_low_tmp, high_high_tmp, buffer1, buffer2, buffer_output, low_low_remember{};
                bool in_tmp{}, out_tmp{};
                for (std::size_t i = 0; i < rhs.len; ++i) {
                    LIMB_TYPE in1_low_var = rhs.data[i] & mask_low;
                    LIMB_TYPE in1_high_var = (rhs.data[i] & mask_high) >> shift_var;

                    std::size_t j = 0;
                    for (; j < lhs.len; ++j) {
                        //in0 long, in1 short; streamline multiply add
                        LIMB_TYPE in0_low_var_mod = lhs.data[j] & mask_low;
                        LIMB_TYPE in0_high_var_mod = (lhs.data[j] & mask_high) >> shift_var;

                        LIMB_TYPE out_low_low = in1_low_var * in0_low_var_mod + low_low_tmp;

                        LIMB_TYPE out_low_high = in1_low_var * in0_high_var_mod + low_high_tmp;

                        LIMB_TYPE out_high_low = in1_high_var * in0_low_var_mod + high_low_tmp;

                        LIMB_TYPE out_high_high = in1_high_var * in0_high_var_mod + high_high_tmp;

                        low_low_tmp = out_low_low >> shift_var;
                        low_high_tmp = out_low_high >> shift_var;
                        high_low_tmp = out_high_low >> shift_var;
                        high_high_tmp = out_high_high >> shift_var;

                        buffer1 = (out_high_high << shift_var) + out_high_low;
                        //out_low_low is zero here, because it will be i-1
                        buffer2 = (low_low_remember << shift_var) + out_low_high;
                        buffer_output = buffer1 + buffer2 + in_tmp;
                        in_tmp = buffer_output < (std::max(buffer1, buffer2));

                        buffer1 = buffer_output;
                        buffer2 = c.data[i + j];
                        buffer_output = buffer1 + buffer2 + out_tmp;
                        out_tmp = buffer_output < (std::max(buffer1, buffer2));

                        c.data[i + j] = buffer_output;

                        low_low_remember = out_low_low;
                    }

                    ++j;
                    while (out_tmp or in_tmp) {
                        LIMB_TYPE old_out = c.data[i + j];
                        c.data[i + j] += in_tmp + out_tmp;
                        if (in_tmp)in_tmp = false;
                        out_tmp = c.data[i + j] < (std::max(old_out, (LIMB_TYPE)in_tmp + (LIMB_TYPE)out_tmp));
                        ++j;
                    }
                    low_low_tmp = 0;
                    low_high_tmp = 0;
                    high_low_tmp = 0;
                    high_high_tmp = 0;
                    in_tmp = false;
                    out_tmp = false;
                }
            } else {
                LIMB_TYPE low_low_tmp, low_high_tmp, high_low_tmp, high_high_tmp, buffer1, buffer2, buffer_output, low_low_remember{};
                bool in_tmp{}, out_tmp{};
                for (std::size_t i = 0; i < lhs.len; ++i) {
                    LIMB_TYPE in1_low_var = lhs.data[i] & mask_low;
                    LIMB_TYPE in1_high_var = (lhs.data[i] & mask_high) >> shift_var;

                    std::size_t j = 0;
                    for (; j < rhs.len; ++j) {
                        //in0 long, in1 short; streamline multiply add
                        LIMB_TYPE in0_low_var_mod = rhs.data[j] & mask_low;
                        LIMB_TYPE in0_high_var_mod = (rhs.data[j] & mask_high) >> shift_var;

                        LIMB_TYPE out_low_low = in1_low_var * in0_low_var_mod + low_low_tmp;

                        LIMB_TYPE out_low_high = in1_low_var * in0_high_var_mod + low_high_tmp;

                        LIMB_TYPE out_high_low = in1_high_var * in0_low_var_mod + high_low_tmp;

                        LIMB_TYPE out_high_high = in1_high_var * in0_high_var_mod + high_high_tmp;

                        low_low_tmp = out_low_low >> shift_var;
                        low_high_tmp = out_low_high >> shift_var;
                        high_low_tmp = out_high_low >> shift_var;
                        high_high_tmp = out_high_high >> shift_var;

                        buffer1 = (out_high_high << shift_var) + out_high_low;
                        //out_low_low is zero here, because it will be i-1
                        buffer2 = (low_low_remember << shift_var) + out_low_high;
                        buffer_output = buffer1 + buffer2 + in_tmp;
                        in_tmp = buffer_output < (std::max(buffer1, buffer2));

                        buffer1 = buffer_output;
                        buffer2 = c.data[i + j];
                        buffer_output = buffer1 + buffer2 + out_tmp;
                        out_tmp = buffer_output < (std::max(buffer1, buffer2));

                        c.data[i + j] = buffer_output;

                        low_low_remember = out_low_low;
                    }

                    ++j;
                    while (out_tmp or in_tmp) {
                        LIMB_TYPE old_out = c.data[i + j];
                        c.data[i + j] += in_tmp + out_tmp;
                        if (in_tmp)in_tmp = false;
                        out_tmp = c.data[i + j] < (std::max(old_out, (LIMB_TYPE)in_tmp + (LIMB_TYPE)out_tmp));
                        ++j;
                    }
                    low_low_tmp = 0;
                    low_high_tmp = 0;
                    high_low_tmp = 0;
                    high_high_tmp = 0;
                    in_tmp = false;
                    out_tmp = false;
                }
            }


            size_t lastElement;
            for(lastElement = c.lenAllocated - 1; c.data[lastElement] == 0; --lastElement);
            c.len = lastElement + 1;
            return c;
        }

        /*
         * //a/%b returns <<div,div_len><mod,mod_len>>
        static std::tuple<SUBCLASS, SUBCLASS> divmod(const SUBCLASS &lhs, const SUBCLASS &rhs) {
            //TODO: shiftl if divisor is exp 2
            if (rhs.len == 1) {
                // if there can only be modulo 0
                if(rhs.data[0] == 1)
                    return std::make_tuple(SUBCLASS(lhs), SUBCLASS(0));
                if(lhs.len == 1)
                    return std::make_tuple(SUBCLASS(lhs.data[0] / rhs.data[0]), SUBCLASS(lhs.data[0] % rhs.data[0]));
            }
            if (lhs.len == 1 and lhs.data[0] == 0)
                return std::make_tuple(SUBCLASS(0), SUBCLASS(0));
            if (lt(lhs, rhs)) {
                // with a < b, a % b = a
                return std::make_tuple(SUBCLASS(0), SUBCLASS(lhs));
            }

            SUBCLASS c;
            c.setLenAllocated(roundUp<LIMB_TYPE>(lhs.len - rhs.len));
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.lenAllocated]{});

            //long memory_left = static_cast<long>(getFreeSystemMemory());
            SUBCLASS work(lhs);
            std::size_t bit_height_in0 = lhs.len * 8 * sizeof(LIMB_TYPE) + std::countr_zero(lhs.data[lhs.len - 1]);
            std::size_t bit_height_in1 = rhs.len * 8 * sizeof(LIMB_TYPE) + std::countr_zero(rhs.data[rhs.len - 1]);
            long long bit_normalize = static_cast<long long>(bit_height_in0) - static_cast<long long>(bit_height_in1);
            long long state_elment = bit_normalize % (8 * sizeof(LIMB_TYPE));
            long long old_state_element = state_elment;
            auto to_dividend_height = shiftr(rhs, static_cast<std::size_t>(bit_normalize));

            //TODO: do not call subfunctions and reference to divisor

            //if (memory_left < outlen * sizeof(CALCTYPE) * 8) {
            //not enough RAM for full optimized shift prepare, slow version
            do {
                while (bit_normalize > 0 and lt(lhs, to_dividend_height)) {
                    std:size_t blen_tmp = rhs.len;
                    while (not rhs.data[blen_tmp - 1] and blen_tmp > 0)
                        --blen_tmp;
                    bit_height_in1 = blen_tmp * 8 * sizeof(LIMB_TYPE) + std::countr_zero(rhs.data[blen_tmp - 1]);
                    long long bit_normalize_tmp =
                            static_cast<long long>(bit_height_in0) - static_cast<long long>(bit_height_in1);
                    long long total_shift_left = bit_normalize_tmp - bit_normalize;
                    bit_normalize -= total_shift_left;
                    if (bit_normalize < 0) {
                        total_shift_left += std::abs(bit_normalize);
                        bit_normalize = 0;
                    }
                    state_elment = bit_normalize % (8 * sizeof(LIMB_TYPE));
                    to_dividend_height = shiftl(to_dividend_height, static_cast<std::size_t>(total_shift_left));

                }

                SUBCLASS c_tmp = minus(work, to_dividend_height);
                while (old_state_element > state_elment) {
                    c.data[state_elment] = 0;
                    --old_state_element;
                }

                bool overflow_detect = false;
                std::size_t j = state_elment;
                do {
                    LIMB_TYPE old_out = c.data[j];
                    c.data[j] += j == state_elment ? std::pow(2, bit_normalize % (8 * sizeof(LIMB_TYPE))) : 0 + overflow_detect;
                    overflow_detect = c.data[j] < old_out;
                    ++j;
                } while (overflow_detect);//add 2^x to out

                bit_height_in0 = c_tmp.len * 8 * sizeof(LIMB_TYPE) + std::countr_zero(lhs.data[c_tmp.len - 1]);
                bit_normalize = static_cast<long long>(bit_height_in0) - static_cast<long long>(bit_height_in1);

                if (bit_normalize > 0) {
                    auto to_dividend_height_tmp = to_dividend_height;
                    to_dividend_height = shiftr(rhs, static_cast<std::size_t>(bit_normalize));
                }
            } while (bit_normalize > 0);
            //TODO: if there is a lot of RAM left, shift divisor 64 times by 1 and store all those arrays single --> reference
            //} else {
            //use a lot of RAM, but fast version with direct var copy reference and no call of sub-functions

            //}

            size_t lastElement;
            for(lastElement = c.lenAllocated - 1; c.data[lastElement] == 0; --lastElement);
            c.len = lastElement + 1;
            return c;
        }
         */

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator/(BigInteger &lhs, const InType &rhs) {
            //TODO: repair operator/()
            return BigInteger(rhs);
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator%(BigInteger &lhs, const InType &rhs) {
            //TODO: repair operator%()
            return BigInteger(rhs);
        }

        /*
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator/(BigInteger &lhs, const InType &rhs) {
            std::tuple<BigInteger,BigInteger> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = BigInteger::divmod(lhs.data, lhs.len, rhs.data, rhs.len);
                return {std::get<0>(std::get<0>(out)), std::get<1>(std::get<0>(out)), static_cast<bool>(lhs.sign xor rhs.sign), true};
            }
            else{
                BigInteger tmp2(rhs);
                out = BigInteger::divmod(lhs.data, lhs.len, tmp2.data, tmp2.len);
                std::free(std::get<0>(std::get<1>(out)));
                return {std::get<0>(std::get<0>(out)), std::get<1>(std::get<0>(out)), static_cast<bool>(lhs.sign xor tmp2.sign), true};
            }
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator%(BigInteger &lhs, const InType &rhs) {
            std::tuple<std::tuple<std::size_t*,std::size_t>,std::tuple<std::size_t*,std::size_t>> out;
            if constexpr (std::is_same_v<BigInteger,InType>){
                out = BigInteger::divmod(lhs.data, lhs.len, rhs.data, rhs.len);
                std::free(std::get<0>(std::get<0>(out)));
                return {std::get<0>(std::get<0>(out)), std::get<1>(std::get<0>(out)), static_cast<bool>(lhs.sign xor rhs.sign), true};
            }
            else{
                BigInteger tmp2(rhs);
                out = BigInteger::divmod(lhs.data, lhs.len, tmp2.data, tmp2.len);
                std::free(std::get<0>(std::get<0>(out)));
                return {std::get<0>(std::get<0>(out)), std::get<1>(std::get<0>(out)), static_cast<bool>(lhs.sign xor tmp2.sign), true};
            }
        }*/

        //logic operations
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator&(BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            BigInteger c;
            c.len = std::min(lhs.len, rhs.len);
            c.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(c.len);
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.lenAllocated]{});

            for (std::size_t i = 0; i < c.len; ++i) {
                c.data[i] = lhs.data[i] & rhs.data[i];
            }

            return c;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator|(BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            BigInteger c;
            c.len = std::max(lhs.len, rhs.len);
            c.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(c.len);
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.LenAllocated]{});

            std::size_t i = 0;
            for (; i < std::min<std::size_t>(lhs.len, rhs.len); ++i) {
                c.data[i] = lhs.data[i] || rhs.data[i];
            }

            if(lhs.len > rhs.len) {
                for (; i < lhs.len; ++i)
                    c.data[i] = lhs.data[i];
            } else if (lhs.len < rhs.len) {
                for (; i < rhs.len; ++i)
                    c.data[i] = rhs.data[i];
            }

            return c;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator^(BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            BigInteger c;
            c.len = std::max(lhs.len, rhs.len);
            c.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(c.len);
            c.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[c.lenAllocated]{});

            std::size_t i = 0;
            for (; i < std::min<std::size_t>(lhs.len, rhs.len); ++i) {
                c.data[i] = lhs.data[i] ^ rhs.data[i];
            }

            if(lhs.len > rhs.len) {
                for (; i < lhs.len; ++i)
                    c.data[i] = lhs.data[i];
            } else if (lhs.len < rhs.len) {
                for (; i < rhs.len; ++i)
                    c.data[i] = rhs.data[i];
            }

            return c;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator~(const InType &lhs) {
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                lhs = BigInteger(lhs);
            BigInteger out;
            out.len = lhs.len;
            out.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(out.len);
            out.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[out.lenAllocated]{});

            for (std::size_t i = 0; i < lhs.len; ++i) {
                out.data[i] = ~lhs.data[i];
            }

            return out;
        }

        //TODO: fix shift left
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator<<(BigInteger &lhs, const std::size_t rhs) {
            //if the last bit of the last element is 1 we need an additional element at the beginning
            std::size_t el_shift = rhs / (8 * sizeof(LIMB_TYPE));

            if (el_shift > lhs.len)
                el_shift = lhs.len;

            if (not rhs)
                return SUBCLASS(lhs);

            return BigInteger(0);

//            SUBCLASS b;
//            std::size_t outputLenAllocated = roundUp<LIMB_TYPE>(a.len);
//            b.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[outputLenAllocated]{});
//
//            LIMB_TYPE tmp{};
//
//            std::size_t i = outlen - 1;
//            std::size_t shift_step = (shiftl_num % (8 * sizeof(LIMB_TYPE)));
//            for (; i > (outlen - 1) - el_shift; ++i) {
//                b.data[i - el_shift] += tmp >> (8 * sizeof(LIMB_TYPE) - shift_step);
//                tmp = a.data[i];
//                b.data[i - el_shift] = a.data[i] << shift_step;
//            }
//            b.data[i - el_shift] += tmp >> (8 * sizeof(LIMB_TYPE) - shift_step);
//
//            return std::make_tuple(out, outlen - 1);
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend BigInteger operator>>(BigInteger &lhs, const std::size_t rhs) {
            //if the last bit of the last element is 1 we need an additional element at the end
            LIMB_TYPE lowest_bit =
                    std::numeric_limits<LIMB_TYPE>::max() >> (8 * sizeof(LIMB_TYPE) - rhs % (8 * sizeof(LIMB_TYPE)));
            std::size_t el_shift = rhs / (8 * sizeof(LIMB_TYPE));

            if (not rhs)
                return BigInteger(lhs);

            BigInteger b;
            b.lenAllocated = roundUpToVectorLength<LIMB_TYPE>(lhs.len);
            b.data.reset(new (std::align_val_t(getAlignment())) LIMB_TYPE[b.lenAllocated]{});

            LIMB_TYPE tmp{};

            std::size_t i = 0;
            for (; i < el_shift; ++i) {
                b.data[i] = 0;
            }

            i = 0;

            std::size_t shift_step = (rhs % (8 * sizeof(LIMB_TYPE)));
            for (; i < lhs.len; ++i) {
                b.data[i + el_shift] = tmp << ((8 * sizeof(LIMB_TYPE)) - shift_step);
                tmp = lhs.data[i];
                b.data[i + el_shift] += lhs.data[i] >> shift_step;
            }
            b.data[i + el_shift] = tmp;

            size_t lastElement;
            for(lastElement = b.lenAllocated - 1; b.data[lastElement] == 0; --lastElement);
            b.len = lastElement + 1;
            return b;
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
        friend bool operator<(const BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            if (lhs.len != rhs.len)
                return lhs.len < rhs.len;

            std::size_t lastIndex = lhs.len - 1;
            for (; lastIndex >= 0; --lastIndex) {
                if (lhs.data[lastIndex] >= rhs.data[lastIndex])
                    return false;
                else if (lhs.data[lastIndex] < rhs.data[lastIndex])
                    return true;
            }

            return false;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator> (const BigInteger& lhs, InType& rhs) { return rhs < lhs; }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator<=(const BigInteger& lhs, InType& rhs) { return !(lhs > rhs); }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator>=(const BigInteger& lhs, InType& rhs) { return !(lhs < rhs); }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator==(const BigInteger &lhs, const InType &rhsTmp) {
            BigInteger rhs;
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhsTmp);
            else
                rhs = rhsTmp;

            if (lhs.len != rhs.len)
                return false;

            for (std::size_t i = 0; i < lhs.len; ++i) {
                if (lhs.data[i] != rhs.data[i])
                    return false;
            }

            return true;
        }

        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator!=(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::disjunction_v<std::is_arithmetic<InType>, std::is_same<InType, std::string>>)
                rhs = BigInteger(rhs);

            if (lhs.len != rhs.len)
                return true;

            for (std::size_t i = 0; i < lhs.len; ++i) {
                if (lhs.data[i] != rhs.data[i])
                    return true;
            }

            return false;
        }

        BigInteger &operator++() {          //prefix increment
            *this += 1;
            return *this;
        }

        const BigInteger operator++(int) {      //postfix increment
            BigInteger tmp(*this);
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


        std::string as_string() {
            //TODO: implement as_string()
            return "NOT IMPLEMENTED";
        }
        /*
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

            auto dividend=BigInteger(std::get<0>(this->as_array_ref()), std::get<1>(this->as_array_ref()));
            std::tuple<std::tuple<std::size_t*, std::size_t>, std::tuple<std::size_t*, std::size_t>> div;
            bool check_gt;

            do{
                div=divmod(std::get<0>(dividend.as_array_ref()),std::get<1>(dividend.as_array_ref()),std::get<0>(divisor.as_array_ref()),std::get<1>(divisor.as_array_ref()));
                divisor/=mod_big;
                //power--;
                std::string app_str=BigInteger(std::get<0>(std::get<0>(div)), std::get<1>(std::get<0>(div))).as_string();
                while(app_str.size()<mod_str_len){
                    app_str.insert(app_str.cbegin(),'0');
                }
                out+=app_str;
                dividend=BigInteger(std::get<0>(std::get<1>(div)), std::get<1>(std::get<1>(div)));
                check_gt = dividend > divisor;
                if(check_gt){
                    std::free(std::get<0>(std::get<0>(div)));
                    std::free(std::get<0>(std::get<1>(div)));
                }
            }while (check_gt);

            std::string app_str=BigInteger(std::get<0>(std::get<1>(div)), std::get<1>(std::get<1>(div))).as_string();
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
         */

        [[nodiscard]] std::size_t size() const{
            return this->len;
        }

        const std::size_t operator[](std::size_t at) const{
            if(at>this->lenAllocated)throw uh::numbers::OutOfBoundsException();
            return this->data[at];
        }
    };

}

#endif //INC_21_NUMBERS_BIGINTEGERCUSTOM_H
