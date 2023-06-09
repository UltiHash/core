#ifndef LS_CRYPTO_PROVIDER_H
#define LS_CRYPTO_PROVIDER_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include "APIDef.h"
#include <memory>
#include <string>

namespace LicenseSpring
{
    /// \brief CryptoProvider interface.
    /// \details You can implement this interface to provide your custom approach for local license encryption.
    /// Before implementing your CryptoProvider class consider just changing Salt or Key of default provided.
    class LS_API CryptoProvider
    {
    public:
        using ptr_t = std::shared_ptr<CryptoProvider>;

        /// \brief Destructor for CryptoProvider object.
        virtual ~CryptoProvider();

        /// \brief Encrypt given string.
        /// \param inputString String to encrypt
        /// \return Encrypted string.
        virtual std::string encrypt( const std::string& inputString ) = 0;

        /// \brief Decrypt given string.
        /// \param inputString String to decrypt
        /// \return Decrypted string.
        virtual std::string decrypt( const std::string& inputString ) = 0;

        /// \brief Set cryptographic salt.
        /// \param salt New value to set salt to equal
        virtual void setSalt( const std::string& salt );

        /// \brief Set key.
        /// \param key New value to set key to equal
        virtual void setKey( const std::string& key );

        /// \brief Checker for whether key is set.
        /// \return True if the key is set (not empty), false if not set (empty).
        bool isKeySet() const;

    protected:
        std::string m_salt;
        std::string m_key;
    };

    /// \brief If no CryptoProvider is set, this is the default CryptoProvider that will be used.
    class LS_API DefaultCryptoProvider final : public CryptoProvider
    {
    public:
        using ptr_t = std::shared_ptr<DefaultCryptoProvider>;

        /// \brief DefaultCryptoProvider constructor.
        /// \details For internal needs, use create instead.
        DefaultCryptoProvider();

        /// \brief DefaultCryptoProvider destructor.
        ~DefaultCryptoProvider() override;

        /// \brief Allocates and constructs a DefaultCryptoProvider object and passes args to DefaultCryptoProvider().
        /// \return A shared_ptr object that owns and stores a pointer to a newly allocated DefaultCryptoProvider object.
        static ptr_t create()
        {
            return std::make_shared<DefaultCryptoProvider>();
        }

        /// \brief Encrypt given string.
        /// \param inputString String to encrypt
        /// \return Encrypted string.
        std::string encrypt( const std::string& inputString ) override;

        /// \brief Decrypt given string.
        /// \param inputString String to decrypt
        /// \return Decrypted string.
        std::string decrypt( const std::string& inputString ) override;

    private:
        const unsigned char* getKey( const std::string& key );
    };
} // namespace LicenseSpirng

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_CRYPTO_PROVIDER_H