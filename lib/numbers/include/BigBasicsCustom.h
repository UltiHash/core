//
// Created by benjamin-elias on 07.11.22.
//

#ifndef INC_21_NUMBERS_BIGBASICSCUSTOM_H
#define INC_21_NUMBERS_BIGBASICSCUSTOM_H

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

namespace ultihash::basics {

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

/*
 * Conventions: every array has a size of at least 1 if the value is 0 or at least 2 elements,
 * if the value is not 0, the last element of each array is free and 0
 * the len0 and len1 are the actual array size minus 1, so they represent the number content size
 * Use of BigEndian on all arrays so overflow is in order
 * --> max size of Block is UINT32_MAX - 1
 * 16 bit and higher
 */
    template<typename CALCTYPE>
    class BigBasicsCustom {
    protected:
//TODO: replace all new operators with custom malloc
        //check hardware for memory constraints to improve performance by using more RAM
        static std::size_t getTotalSystemMemory() {
            struct sysinfo memInfo{};
            sysinfo (&memInfo);
            std::size_t totalVirtualMem = memInfo.totalram;
            //Add other values in next statement to avoid int overflow on right hand side...
            //totalVirtualMem += memInfo.totalswap;
            totalVirtualMem *= memInfo.mem_unit;
            return totalVirtualMem;
        }

        static std::size_t getUsedSystemMemory() {
            struct sysinfo memInfo{};
            sysinfo (&memInfo);
            std::size_t virtualMemUsed = memInfo.totalram - memInfo.freeram;
            //Add other values in next statement to avoid int overflow on right hand side...
            //virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
            virtualMemUsed *= memInfo.mem_unit;
            return virtualMemUsed;
        }

        static std::size_t getFreeSystemMemory(){
            return getTotalSystemMemory()-getUsedSystemMemory();
        }

        //in0+in1
        static std::tuple<CALCTYPE *, std::size_t>
        plus(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            //setup new variable with 1 buffer at end
            std::size_t outlen = std::max(len0, len1) + 1;

            auto *out = new CALCTYPE[outlen];
            out[outlen - 1] = 0;

            bool overflow_detect = false;
            for (std::size_t i = 0; i < std::min(len0, len1) or overflow_detect; ++i) {
                out[i] = i <= len0 ? in0[i] : 0 + i <= len1 ? in1[i] : 0 + overflow_detect;
                overflow_detect = out[i] < (std::max(in0[i], in1[i]));
            }

            if (out[outlen - 1] != 0) {
                auto *out2 = new CALCTYPE[outlen + 1];
                std::memcpy(out2, out, outlen * sizeof(CALCTYPE));
                std::free(out);
                return std::make_tuple(out2, outlen);
            }

            return std::make_tuple(out, outlen - 1);
        }

        //in0-in1; in1 is defined being smaller than in0
        static std::tuple<CALCTYPE *, std::size_t>
        minus(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 < len1)throw UnderflowSubtractException();
            //setup new variable with 1 buffer at end
            std::size_t outlen = std::max(len0, len1) + 1;

            auto *out = new CALCTYPE[outlen];
            out[outlen - 1] = 0;

            bool underflow_detect = false;
            for (std::size_t i = 0; i < std::min(len0, len1) or (underflow_detect and i < std::max(len0, len1)); ++i) {
                out[i] = in0[i] - i < len1 ? in1[i] : 0 - underflow_detect;
                underflow_detect = out[i] > in0[i];
            }
            if (underflow_detect) {
                std::free(out);
                throw UnderflowSubtractException();
            }

            if (out[outlen - 1] != 0) {
                auto *out2 = new CALCTYPE[outlen + 1];
                std::memcpy(out2, out, outlen * sizeof(CALCTYPE));
                std::free(out);
                return std::make_tuple(out2, outlen);
            }

            return std::make_tuple(out, outlen - 1);
        }

        //in0*in1
        static std::tuple<CALCTYPE *, std::size_t>
        multiply(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            //TODO: use 4 buffer caches in main function to split main algo to 3 main loops and use SIMD more straight forward
            //TODO: shiftr if the size is 1 and is an exponent of 2
            if (len0 == 0 or len1 == 0) {
                auto *out = (CALCTYPE*) std::malloc(sizeof(CALCTYPE));
                out[0] = 0;
                return std::make_tuple(out, 0);
            }

            const unsigned char shift_var = (sizeof(CALCTYPE) * 8 / 2);
            CALCTYPE mask_low = std::numeric_limits<CALCTYPE>::max() >> shift_var;
            CALCTYPE mask_high = mask_low << shift_var;

            long memory_left = static_cast<long>(getFreeSystemMemory());
            std::size_t outlen = len0 + len1 + 1;
            while (memory_left < outlen) {
                std::cerr << "Warning: No Memory left for Multiplication workflow out prepare! Retry!" << std::endl;
                sleep(800);
                memory_left = static_cast<long>(getFreeSystemMemory());
            }
            auto *out = (CALCTYPE*) std::malloc(outlen * sizeof(CALCTYPE));
            for (std::size_t i = 0; i < outlen; ++i) {
                out[i] = 0;
            }

            if (len0 > len1) {
                CALCTYPE low_low_tmp, low_high_tmp, high_low_tmp, high_high_tmp, buffer1, buffer2, buffer_output, low_low_remember{};
                bool in_tmp{}, out_tmp{};
                for (std::size_t i = 0; i < len1; ++i) {
                    CALCTYPE in1_low_var = in1[i] & mask_low;
                    CALCTYPE in1_high_var = (in1[i] & mask_high) >> shift_var;

                    std::size_t j = 0;
                    for (; j < len0; ++j) {
                        //in0 long, in1 short; streamline multiply add
                        CALCTYPE in0_low_var_mod = in0[j] & mask_low;
                        CALCTYPE in0_high_var_mod = (in0[j] & mask_high) >> shift_var;

                        CALCTYPE out_low_low = in1_low_var * in0_low_var_mod + low_low_tmp;

                        CALCTYPE out_low_high = in1_low_var * in0_high_var_mod + low_high_tmp;

                        CALCTYPE out_high_low = in1_high_var * in0_low_var_mod + high_low_tmp;

                        CALCTYPE out_high_high = in1_high_var * in0_high_var_mod + high_high_tmp;

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
                        buffer2 = out[i + j];
                        buffer_output = buffer1 + buffer2 + out_tmp;
                        out_tmp = buffer_output < (std::max(buffer1, buffer2));

                        out[i + j] = buffer_output;

                        low_low_remember = out_low_low;
                    }

                    ++j;
                    while (out_tmp or in_tmp) {
                        CALCTYPE old_out = out[i + j];
                        out[i + j] += in_tmp + out_tmp;
                        if (in_tmp)in_tmp = false;
                        out_tmp = out[i + j] < (std::max(old_out, (CALCTYPE)in_tmp + (CALCTYPE)out_tmp));
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
                CALCTYPE low_low_tmp, low_high_tmp, high_low_tmp, high_high_tmp, buffer1, buffer2, buffer_output, low_low_remember{};
                bool in_tmp{}, out_tmp{};
                for (std::size_t i = 0; i < len0; ++i) {
                    CALCTYPE in1_low_var = in0[i] & mask_low;
                    CALCTYPE in1_high_var = (in0[i] & mask_high) >> shift_var;

                    std::size_t j = 0;
                    for (; j < len1; ++j) {
                        //in0 long, in1 short; streamline multiply add
                        CALCTYPE in0_low_var_mod = in1[j] & mask_low;
                        CALCTYPE in0_high_var_mod = (in1[j] & mask_high) >> shift_var;

                        CALCTYPE out_low_low = in1_low_var * in0_low_var_mod + low_low_tmp;

                        CALCTYPE out_low_high = in1_low_var * in0_high_var_mod + low_high_tmp;

                        CALCTYPE out_high_low = in1_high_var * in0_low_var_mod + high_low_tmp;

                        CALCTYPE out_high_high = in1_high_var * in0_high_var_mod + high_high_tmp;

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
                        buffer2 = out[i + j];
                        buffer_output = buffer1 + buffer2 + out_tmp;
                        out_tmp = buffer_output < (std::max(buffer1, buffer2));

                        out[i + j] = buffer_output;

                        low_low_remember = out_low_low;
                    }

                    ++j;
                    while (out_tmp or in_tmp) {
                        CALCTYPE old_out = out[i + j];
                        out[i + j] += in_tmp + out_tmp;
                        if (in_tmp)in_tmp = false;
                        out_tmp = out[i + j] < (std::max(old_out, (CALCTYPE)in_tmp + (CALCTYPE)out_tmp));
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

            if (out[outlen - 1] != 0) {
                auto *out2 = new CALCTYPE[outlen + 1];
                std::memcpy(out2, out, outlen * sizeof(CALCTYPE));
                std::free(out);
                return std::make_tuple(out2, outlen);
            }

            return std::make_tuple(out, outlen);
        }

        //in0/%in1 returns <<div,div_len><mod,mod_len>>
        static std::tuple<std::tuple<CALCTYPE *, std::size_t>, std::tuple<CALCTYPE *, std::size_t>>
        divmod(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            //TODO: shiftl if divisor is exp 2
            if (len1 == 1) {
                //if there can only be modulo
                if(in1[0] == 1){
                    auto *out = new CALCTYPE[1];
                    out[0] = 0;
                    std::size_t outlen = len0 + 1;
                    auto *out2 = new CALCTYPE[outlen + 1];
                    std::memcpy(out2, in0, outlen * sizeof(CALCTYPE));
                    return std::make_tuple(std::make_tuple(out2, outlen), std::make_tuple(out, 0));
                }
                if(len0 == 1){
                    auto *out_div=new CALCTYPE[1+1];
                    out_div[0]=in0[0]/in1[0];
                    auto *out_mod=new CALCTYPE[1+1];
                    out_mod[0]=in0[0]%in1[0];
                    return std::make_tuple(std::make_tuple(out_div, 1), std::make_tuple(out_mod, 1));
                }
            }
            if (len0 == 0) {
                if(in0[0] == 0)throw OutOfBoundsException();
                //if there can only be modulo
                auto *out = new CALCTYPE[1];
                out[0] = 0;
                std::size_t outlen = len0;
                auto *out2 = new CALCTYPE[outlen + 1];
                std::memcpy(out2, in0, outlen * sizeof(CALCTYPE));
                return std::make_tuple(std::make_tuple(out2, outlen), std::make_tuple(out, 0));
            }
            if (lt(in0, len0, in1, len1)) {
                //if there can only be modulo
                auto *out = new CALCTYPE[1];
                out[0] = 0;
                std::size_t outlen = len0 + 1;
                auto *out2 = new CALCTYPE[outlen + 1];
                std::memcpy(out2, in0, outlen * sizeof(CALCTYPE));
                return std::make_tuple(std::make_tuple(out, 0), std::make_tuple(out2, outlen));
            }

            //long memory_left = static_cast<long>(getFreeSystemMemory());
            std::size_t outlen = len0 - len1 + 1;
            auto *out = new CALCTYPE[outlen];
            auto *work_ptr = new CALCTYPE[len0 + 1];
            std::memcpy(work_ptr, in0, (len0 + 1) * sizeof(CALCTYPE));
            std::size_t bit_height_in0 = len0 * 8 * sizeof(CALCTYPE) + std::countr_zero(in0[len0 - 1]);
            std::size_t bit_height_in1 = len1 * 8 * sizeof(CALCTYPE) + std::countr_zero(in1[len1 - 1]);
            long long bit_normalize = static_cast<long long>(bit_height_in0) - static_cast<long long>(bit_height_in1);
            long long state_elment = bit_normalize % (8 * sizeof(CALCTYPE));
            long long old_state_element = state_elment;
            auto to_dividend_height = shiftr(in1, len1, bit_normalize);

            //TODO: do not call subfunctions and reference to divisor

            //if (memory_left < outlen * sizeof(CALCTYPE) * 8) {
            //not enough RAM for full optimized shift prepare, slow version
            do {
                while (bit_normalize > 0 and lt(in0, len0, std::get<0>(to_dividend_height), std::get<1>(to_dividend_height))) {
                    auto to_dividend_height_tmp = to_dividend_height;
                    while (not in1[len1 - 1] and len1 > 0)--len1;
                    bit_height_in1 = len1 * 8 * sizeof(CALCTYPE) + std::countr_zero(in1[len1 - 1]);
                    long long bit_normalize_tmp =
                            static_cast<long long>(bit_height_in0) - static_cast<long long>(bit_height_in1);
                    long long total_shift_left = bit_normalize_tmp - bit_normalize;
                    bit_normalize -= total_shift_left;
                    if (bit_normalize < 0) {
                        total_shift_left += std::abs(bit_normalize);
                        bit_normalize = 0;
                    }
                    state_elment = bit_normalize % (8 * sizeof(CALCTYPE));
                    to_dividend_height = shiftl(std::get<0>(to_dividend_height), std::get<1>(to_dividend_height),
                                                static_cast<std::size_t>(total_shift_left));
                    std::free(std::get<0>(to_dividend_height_tmp));
                }

                auto output_ptr = minus(work_ptr, len0, std::get<0>(to_dividend_height), std::get<1>(to_dividend_height));
                std::free(work_ptr);
                work_ptr = std::get<0>(output_ptr);
                len0 = std::get<1>(output_ptr);

                while (old_state_element > state_elment) {
                    out[state_elment] = 0;
                    --old_state_element;
                }

                bool overflow_detect = false;
                std::size_t j = state_elment;
                do {
                    CALCTYPE old_out = out[j];
                    out[j] += j == state_elment ? std::pow(2, bit_normalize % (8 * sizeof(CALCTYPE))) : 0 + overflow_detect;
                    overflow_detect = out[j] < old_out;
                    ++j;
                } while (overflow_detect);//add 2^x to out

                bit_height_in0 = len0 * 8 * sizeof(CALCTYPE) + std::countr_zero(in0[len0 - 1]);
                bit_normalize = static_cast<long long>(bit_height_in0) - static_cast<long long>(bit_height_in1);

                if (bit_normalize > 0) {
                    auto to_dividend_height_tmp = to_dividend_height;
                    to_dividend_height = shiftr(in1, len1, bit_normalize);
                    std::free(std::get<0>(to_dividend_height_tmp));
                }
            } while (bit_normalize > 0);
            //TODO: if there is a lot of RAM left, shift divisor 64 times by 1 and store all those arrays single --> reference
            //} else {
            //use a lot of RAM, but fast version with direct var copy reference and no call of sub-functions

            //}

            return std::make_tuple(std::make_tuple(out,outlen-1),std::make_tuple(work_ptr,len0));
        }

        //in0=>>shiftr_num
        static std::tuple<CALCTYPE *, std::size_t>
        shiftr(CALCTYPE *in0, std::size_t len0, std::size_t shiftr_num) {
            //if the last bit of the last element is 1 we need an additional element at the end
            CALCTYPE lowest_bit =
                    std::numeric_limits<CALCTYPE>::max() >> (8 * sizeof(CALCTYPE) - shiftr_num % (8 * sizeof(CALCTYPE)));
            std::size_t el_shift = shiftr_num / (8 * sizeof(CALCTYPE));

            std::size_t outlen = 1 + len0 + el_shift + ((len0 > 1 and shiftr_num > 0) ? (in0[len0 - 1] & lowest_bit) : 0);
            auto *out = new CALCTYPE[outlen];
            if (not shiftr_num) {
                std::memcpy(out, in0, (len0 + 1) * sizeof(CALCTYPE));
                return std::make_tuple(out, outlen - 1);
            }
            CALCTYPE tmp{};

            std::size_t i = 0;
            for (; i < el_shift; ++i) {
                out[i] = 0;
            }

            i = 0;

            std::size_t shift_step = (shiftr_num % (8 * sizeof(CALCTYPE)));
            for (; i < len0; ++i) {
                out[i + el_shift] = tmp << ((8 * sizeof(CALCTYPE)) - shift_step);
                tmp = in0[i];
                out[i + el_shift] += in0[i] >> shift_step;
            }
            out[i + el_shift] = tmp;

            if (out[outlen - 1] != 0) {
                auto *out2 = new CALCTYPE[outlen + 1];
                std::memcpy(out2, out, outlen * sizeof(CALCTYPE));
                std::free(out);
                return std::make_tuple(out2, outlen);
            }

            return std::make_tuple(out, outlen - 1);
        }

        //in0=<<shiftl_num
        static std::tuple<CALCTYPE *, std::size_t>
        shiftl(CALCTYPE *in0, std::size_t len0, std::size_t shiftl_num) {
            //if the last bit of the last element is 1 we need an additional element at the beginning
            std::size_t el_shift = shiftl_num / (8 * sizeof(CALCTYPE));

            if (el_shift > len0)el_shift = len0;

            std::size_t outlen = 1 + len0 - el_shift;
            auto *out = new CALCTYPE[outlen];
            if (not shiftl_num) {
                std::memcpy(out, in0, (len0 + 1) * sizeof(CALCTYPE));
                return std::make_tuple(out, outlen - 1);
            }
            CALCTYPE tmp{};

            std::size_t i = outlen - 1;
            std::size_t shift_step = (shiftl_num % (8 * sizeof(CALCTYPE)));
            for (; i > (outlen - 1) - el_shift; ++i) {
                out[i - el_shift] += tmp >> (8 * sizeof(CALCTYPE) - shift_step);
                tmp = in0[i];
                out[i - el_shift] = in0[i] << shift_step;
            }
            out[i - el_shift] += tmp >> (8 * sizeof(CALCTYPE) - shift_step);

            return std::make_tuple(out, outlen - 1);
        }

        //in0&in1
        static std::tuple<CALCTYPE *, size_t>
        and_func(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            std::size_t outlen = std::min(len0, len1) + 1;
            auto *out = new CALCTYPE[outlen];

            for (std::size_t i = 0; i < outlen - 1; ++i) {
                out[i] = in0[i] & in1[i];
            }

            out[outlen - 1] = 0;

            return std::make_tuple(out, outlen - 1);
        }

        //in0||in1
        static std::tuple<CALCTYPE *, size_t>
        or_func(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            std::size_t outlen = std::max(len0, len1) + 1;
            auto *out = new CALCTYPE[outlen];

            out[outlen - 1] = 0;

            for (std::size_t i = 0; i < outlen - 1; ++i) {
                out[i] = in0[i] || in1[i];
            }

            return std::make_tuple(out, outlen - 1);
        }

        //in0^in1
        static std::tuple<CALCTYPE *, size_t>
        xor_func(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            std::size_t outlen = std::max(len0, len1) + 1;
            auto *out = new CALCTYPE[outlen];

            out[outlen - 1] = 0;

            for (std::size_t i = 0; i < outlen - 1; ++i) {
                out[i] = in0[i] ^ in1[i];
            }

            return std::make_tuple(out, outlen - 1);
        }

        // !in0
        static std::tuple<CALCTYPE *, size_t>
        complement_func(CALCTYPE *in0, std::size_t len0) {
            auto *out = new CALCTYPE[len0 + 1];

            out[len0] = 0;

            for (std::size_t i = 0; i < len0; ++i) {
                out[i] = ~in0[i];
            }

            return std::make_tuple(out, len0);
        }

        //in0>in1
        static bool gt(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 != len1)return len0 > len1;

            for (; len0 > 0; --len0) {
                if (in0[len0] < in1[len0])return false;
                else if (in0[len0] > in1[len0]) return true;
            }

            return false;
        }

        //in0>=in1
        static bool gte(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 != len1)return len0 > len1;
            else {
                if (equ(in0, len0, in1, len1))return true;
            }

            for (; len0 > 0; --len0) {
                if (in0[len0] < in1[len0])return false;
                else if (in0[len0] > in1[len0]) return true;
            }

            return true;
        }

        //in0<in1
        static bool lt(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 != len1)return len0 < len1;

            for (; len0 > 0; --len0) {
                if (in0[len0] > in1[len0])return false;
                else if (in0[len0] < in1[len0]) return true;
            }

            return false;
        }

        //in0<=in1
        static bool lte(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 != len1)return len0 < len1;
            else {
                if (equ(in0, len0, in1, len1))return true;
            }

            for (; len0 > 0; --len0) {
                if (in0[len0] > in1[len0])return false;
                else if (in0[len0] < in1[len0]) return true;
            }

            return true;
        }

        //in0==in1
        static bool equ(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 != len1)return false;

            for (std::size_t i = 0; i < len0; ++i) {
                if (in0[i] != in1[i])return false;
            }

            return true;
        }

        //in0!=in1
        static bool ne(CALCTYPE *in0, std::size_t len0, CALCTYPE *in1, std::size_t len1) {
            if (len0 != len1)return true;

            for (std::size_t i = 0; i < len0; ++i) {
                if (in0[i] != in1[i])return true;
            }

            return false;
        }

    };
}
#endif //INC_21_NUMBERS_BIGBASICSCUSTOM_H
