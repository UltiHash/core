#ifndef LS_ENCRYPT_STRING_H
#define LS_ENCRYPT_STRING_H

// -----------------------------------------------------------------------------------------------------------------
// This code provided as is, without any guarantees.
// Credit to https://stackoverflow.com/questions/4102320/c-how-to-encrypt-strings-at-compile-time/56847099#56847099
//           https://creativecommons.org/licenses/by-sa/4.0/
// -----------------------------------------------------------------------------------------------------------------

#include <iostream>

namespace LicenseSpring
{

#if 1
// Convert __TIME__ == "hh:mm:ss" to a sum of seconds this gives a compile-time seed
#define TBX_XSTR_SEED ((__TIME__[7] - '0') * 1ull    + (__TIME__[6] - '0') * 10ull  + \
                       (__TIME__[4] - '0') * 60ull   + (__TIME__[3] - '0') * 600ull + \
                       (__TIME__[1] - '0') * 3600ull + (__TIME__[0] - '0') * 36000ull)
#else
// Use constant seed
#define TBX_XSTR_SEED (3600ull)
#endif

// -----------------------------------------------------------------------------

// return a pseudo random number clamped at 0xFFFFFFFF
constexpr unsigned long long linear_congruent_generator( unsigned rounds )
{
    return 1013904223ull + (1664525ull * ((rounds> 0) ? linear_congruent_generator(rounds - 1) : (TBX_XSTR_SEED) )) % 0xFFFFFFFF;
}

// -----------------------------------------------------------------------------

#define Random_LCG() linear_congruent_generator(10)
#define XSTR_RANDOM_NUMBER(Min, Max) (Min + (Random_LCG() % (Max - Min + 1)))

// -----------------------------------------------------------------------------

constexpr const unsigned long long XORKEY = XSTR_RANDOM_NUMBER(0, 0xFF);

// -----------------------------------------------------------------------------

template<typename Char> constexpr Char encrypt_character( const Char character, int index )
{
    return character ^ static_cast<Char>( XORKEY + index );
}

// -----------------------------------------------------------------------------

template <unsigned size, typename Char> class Xor_string
{
public:
    const unsigned _nb_chars = (size - 1);
    Char _string[size];

    // If everything goes right this constructor should be executed at compile time
    inline constexpr Xor_string( const Char* string ) : _string{}
    {
        for( unsigned i = 0u; i < size; ++i )
            _string[i] = encrypt_character<Char>( string[i], i );
    }

    // This is executed at runtime
    const Char* decrypt()
    {
        Char* string = _string;
        for( unsigned t = 0; t < _nb_chars; t++ )
            string[t] = string[t] ^ static_cast<Char>( XORKEY + t );
        string[ _nb_chars ] = '\0';
        return string;
    }
};

} // namespace LicenseSpring

// -----------------------------------------------------------------------------

#define XorS(name, my_string)    constexpr LicenseSpring::Xor_string<(sizeof(my_string)/sizeof(char)), char> name(my_string)
#define XorString(my_string) []{ constexpr LicenseSpring::Xor_string<(sizeof(my_string)/sizeof(char)), char> expr(my_string); return expr; }().decrypt()

#define XorWS(name, my_string)       constexpr LicenseSpring::Xor_string<(sizeof(my_string)/sizeof(wchar_t)), wchar_t> name(my_string)
#define XorWideString(my_string) []{ constexpr LicenseSpring::Xor_string<(sizeof(my_string)/sizeof(wchar_t)), wchar_t> expr(my_string); return expr; }().decrypt()

/// \def EncryptStr(s)
/// \brief Encrypts c-string at compile time
/// \details Please note this macro does not work for global (static) strings
/// This feature may require C++14.
/// In order to enable C++14 in the Xcode navigate to Targets->MyTarget->Build Settings->Apple Clang - Language - C++ -> C++ Language Dialect and set GNU++14 or C++14.
/// \param s input string.
/// \returns Decrypted string at runtime.
#define EncryptStr(s) XorString(s)

/// \def EncryptWStr(s)
/// \brief Encrypts wide (wchar_t) string at compile time
/// \details Please note this macro does not work for global (static) strings
/// This feature may require C++14.
/// In order to enable C++14 in the Xcode navigate to Targets->MyTarget->Build Settings->Apple Clang - Language - C++ -> C++ Language Dialect and set GNU++14 or C++14.
/// \param s input string.
/// \returns Decrypted string at runtime.
#define EncryptWStr(s) XorWideString(s)

#endif // LS_ENCRYPT_STRING_H