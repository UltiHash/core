//
// Created by max on 28.10.22.
//

#ifndef INC_21_NUMBERS_BigInteger_H
#define INC_21_NUMBERS_BigInteger_H

#include <type_traits>
#include <gmpxx.h>

namespace uh::numbers {

    class BigInteger {
    protected:
        mpz_class number;
    public:
        template<typename Fundamental, std::enable_if_t<std::is_arithmetic<Fundamental>::value, bool> = true>
        explicit BigInteger(Fundamental in) {
            if (sizeof(in) > (mp_bits_per_limb / 8)) {

                std::cerr<<"Limb width of " + std::to_string(sizeof(in)) + " bytes not supported, but only " +
                           std::to_string((mp_bits_per_limb / 8)) + " bytes!"<<std::endl;
                return;
            }

            if constexpr (std::is_floating_point<Fundamental>::value) {
                this->number = mpz_class(static_cast<std::size_t>(in));
            } else {
                this->number = mpz_class(in);
            }
        }

        BigInteger(const std::string value, bool is_minus = false) {
            number = value;
            number = is_minus ? -number : number;
        }

        const mpz_class &getNumber() const {
            return number;
        }

        //todo: import/export binary data from std::vector
        //todo: use mp_bits_per_limb constant to determine the fundamental type used in std::vector



        BigInteger&  operator++() {          //prefix increment
            ++number;
            return *this;
        }

        BigInteger   operator++(int) {      //postfix increment
            BigInteger tmp = *this;
            operator++();
            return tmp;
        }

        BigInteger&  operator--() {        //prefix decrement
            --number;
            return *this;
        }

        BigInteger   operator--(int) {      //postfix decrement
            BigInteger tmp = *this;
            operator--();
            return tmp;
        }

        //compound assignment variants of binary arithmetic operators
        BigInteger operator+() {
            BigInteger tmp = *this;
            tmp.number = +(tmp.number);
            return tmp;
        }

        BigInteger operator-() {
            BigInteger tmp = *this;
            tmp.number = -(tmp.number);
            return tmp;
        }

        BigInteger operator~() {
            BigInteger tmp = *this;
            tmp.number = ~(tmp.number);
            return tmp;
        }

        //compound assignment variants of binary arithmetic operators
        BigInteger& operator+=(const BigInteger& rhs) {
            this->number += rhs.number;
            return *this;
        }

        BigInteger& operator-=(const BigInteger& rhs) {
            this->number -= rhs.number;
            return *this;
        }

        BigInteger& operator*=(const BigInteger& rhs) {
            this->number *= rhs.number;
            return *this;
        }

        BigInteger& operator/=(const BigInteger& rhs) {
            this->number /= rhs.number;
            return *this;
        }

        BigInteger& operator%=(const BigInteger& rhs) {
            this->number %= rhs.number;
            return *this;
        }

        BigInteger& operator&=(const BigInteger& rhs) {
            this->number &= rhs.number;
            return *this;
        }

        BigInteger& operator|=(const BigInteger& rhs) {
            this->number |= rhs.number;
            return *this;
        }

        BigInteger& operator^=(const BigInteger& rhs) {
            this->number ^= rhs.number;
            return *this;
        }

        BigInteger& operator<<=(const std::size_t& rhs) {
            this->number <<= rhs;
            return *this;
        }

        BigInteger& operator>>=(const std::size_t& rhs) {
            this->number >>= rhs;
            return *this;
        }

        //basic arithmetic operators
        friend BigInteger operator+(BigInteger lhs, const BigInteger& rhs) {
            lhs += rhs;
            return lhs;
        }

        friend BigInteger operator-(BigInteger lhs, const BigInteger& rhs) {
            lhs -= rhs;
            return lhs;
        }

        friend BigInteger operator*(BigInteger lhs, const BigInteger& rhs) {
            lhs *= rhs;
            return lhs;
        }

        friend BigInteger operator/(BigInteger lhs, const BigInteger& rhs) {
            lhs /= rhs;
            return lhs;
        }
        friend BigInteger operator%(BigInteger lhs, const BigInteger& rhs) {
            lhs %= rhs;
            return lhs;
        }

        friend BigInteger operator&(BigInteger lhs, const BigInteger& rhs) {
            lhs &= rhs;
            return lhs;
        }

        friend BigInteger operator|(BigInteger lhs, const BigInteger& rhs) {
            lhs |= rhs;
            return lhs;
        }

        friend BigInteger operator^(BigInteger lhs, const BigInteger& rhs) {
            lhs ^= rhs;
            return lhs;
        }

        friend BigInteger operator<<(BigInteger lhs, const std::size_t& rhs) {
            lhs <<= rhs;
            return lhs;
        }

        friend BigInteger operator>>(BigInteger lhs, const std::size_t& rhs) {
            lhs >>= rhs;
            return lhs;
        }

        //comparison operators
        friend bool operator< (const BigInteger& lhs, const BigInteger& rhs) { return lhs.number < rhs.number; }
        friend bool operator> (const BigInteger& lhs, const BigInteger& rhs) { return rhs < lhs; }
        friend bool operator<=(const BigInteger& lhs, const BigInteger& rhs) { return !(lhs > rhs); }
        friend bool operator>=(const BigInteger& lhs, const BigInteger& rhs) { return !(lhs < rhs); }
        friend bool operator==(const BigInteger& lhs, const BigInteger& rhs) { return lhs.getNumber() == rhs.getNumber(); }
        friend bool operator!=(const BigInteger& lhs, const BigInteger& rhs) { return !(lhs == rhs); }
        template<typename InType, std::enable_if_t<std::disjunction<std::is_arithmetic<InType>,std::is_same<InType, BigInteger>,std::is_same<InType, std::string>>::value, bool> = true>
        friend bool operator==(const BigInteger &lhs, const InType &rhs) {
            if constexpr (std::is_same_v<BigInteger,InType>){
                return lhs == rhs;
            }
            else{
                BigInteger tmp2(rhs);
                return lhs == tmp2;
            }
        }

        template<typename InType=unsigned int>
        std::string as_string(const InType& base=10) {
            return this->number.get_str(base);
        }

        [[nodiscard]] std::size_t size() const {
            return mpz_size(this->number.get_mpz_t());
        }

        std::size_t operator[](std::size_t at){
            return mpz_getlimbn(this->number.get_mpz_t(), at);
        }

        template<typename InType, std::enable_if_t<std::is_arithmetic<InType>::value, bool> = true>
        BigInteger &operator=(InType rhs) {
            *this=BigInteger(rhs);
            return *this;
        }
    };



    inline std::ostream& operator<<(std::ostream& os, const uh::numbers::BigInteger& obj) {
        // write obj to stream
        return os << obj.getNumber().get_str();
    }

}

#endif //INC_21_NUMBERS_BigInteger_H
