#ifndef LS_LICENSE_FILE_STORAGE_H
#define LS_LICENSE_FILE_STORAGE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <mutex>
#include <string>
#include "LicenseStorage.h"

namespace boost { namespace interprocess { class permissions; } }

namespace LicenseSpring
{
    /// \brief Base class that saves license in a file, implements <c>ILicenseStorage</c> interface.
    /// \details This class does not use any techniques for file access synchronization so it is not thread safe.
    class LS_API LicenseFileStorageBase : public LicenseStorage
    {
    public:
        using ptr_t = std::shared_ptr<LicenseFileStorageBase>;

        /// \brief Allocates and constructs a LicenseFileStorageBase object and passes args to LicenseFileStorageBase().
        /// \param licensePath The path that license folder path will be set to
        /// \return A shared_ptr object that owns and stores a pointer to a newly allocated LicenseFileStorageBase object.
        static ptr_t create( const std::wstring& licensePath = std::wstring() );

        /// \brief Constructor for LicenseFileStorageBase object.
        /// \param licensePath The path that license folder path will be set to
        LicenseFileStorageBase( const std::wstring& licensePath = std::wstring() );

        /// \brief Saves given license data string to the file.
        /// \param data License data string
        void saveLicense( const std::string& data ) override;

        /// \brief Load license from file.
        /// \return Encrypted license data.
        std::string loadLicense() override;

        /// \brief Removes local license file.
        void clear() override;

        /// \brief Setter for license filename with extension (optional). Default filename is License.key.
        /// \param name New license filename
        void setLicenseFileName( const std::wstring& name );

        /// \brief Setter for folder path where License file is being stored.
        /// \param path New license folder path
        void setLicenseFolderPath( const std::wstring& path );

        /// \brief Getter for license filename with extension (optional). 
        /// \details Default filename is License.key.
        /// \return License filename with extension.
        const std::wstring& licenseFileName() const;

        /// \brief Getter for folder path where License file is being stored.
        /// \return License folder path.
        const std::wstring& licenseFolderPath() const;

        /// \brief Getter for path to the license file.
        /// \return Absolute path to the license file.
        std::wstring licenseFilePath() const;

    protected:
        std::wstring m_folderPath;
        std::wstring m_fileName;

        /// \brief Prepare to read license file.
        /// \return Absolute path to the license file or empty string if file does not exist.
        std::wstring prepareRead() const;

        /// \brief Prepare to write license file.
        /// \details Also creates corresponding folders if they do not exist.
        /// \return Absolute path to the license file.
        std::wstring prepareWrite() const;

        /// \brief Reads license from file.
        /// \param filePath Absolute path to the license file
        /// \return Encrypted license data.
        std::string readLicense( const std::wstring& filePath ) const;

        /// \brief Writes given license data string to the file.
        /// \param filePath Absolute path to the license file
        /// \param data License data string
        void writeLicense( const std::wstring& filePath, const std::string& data ) const;
    };


    /// \brief Class that saves license in a file, derived from <c>LicenseFileStorageBase</c> class.
    /// \details This is the default license storage type. File operations of this class is thread safe.
    class LS_API LicenseFileStorage : public LicenseFileStorageBase
    {
    public:
        using ptr_t = std::shared_ptr<LicenseFileStorage>;

        /// \brief Allocates and constructs a LicenseFileStorage object and passes args to LicenseFileStorage().
        /// \param licensePath The path that license folder path will be set to
        /// \return A shared_ptr object that owns and stores a pointer to a newly allocated LicenseFileStorage object.
        static ptr_t create( const std::wstring& licensePath = std::wstring() );

        /// \brief Constructor for LicenseFileStorage object.
        /// \param licensePath The path that license folder path will be set to
        LicenseFileStorage( const std::wstring& licensePath = std::wstring() );

        /// \brief Saves given license data string to the file.
        /// \param data License data string
        void saveLicense( const std::string& data ) override;

        /// \brief Load license from file.
        /// \return Encrypted license data.
        std::string loadLicense() override;

        /// \brief Removes local license file.
        void clear() override;

    private:
        std::mutex m_mutex;
    };


    /// \brief Class that saves license in a file, derived from <c>LicenseFileStorageBase</c> class.
    /// \details Enhanced license file storage type for interprocess use.
    /// It uses boost named mutex for synchronization. File operations of this class is thread and process safe.
    class LS_API LicenseFileStorageEx : public LicenseFileStorageBase
    {
    public:
        using ptr_t = std::shared_ptr<LicenseFileStorageEx>;

        /// \brief Allocates and constructs a LicenseFileStorageEx object and passes args to LicenseFileStorageEx().
        /// \param appName Application name to be used for mutex name.
        /// \param licensePath The path that license folder path will be set to
        /// \return A shared_ptr object that owns and stores a pointer to a newly allocated LicenseFileStorageEx object.
        static ptr_t create( const std::string& appName, const std::wstring& licensePath = std::wstring() );

        /// \brief LicenseFileStorageEx constructor. 
        /// \param appName Application name to be used for mutex name.
        /// \param licensePath The path that license folder path will be set to
        LicenseFileStorageEx( const std::string& appName, const std::wstring& licensePath = std::wstring() );

        /// \brief Destructor for LicenseFileStorageEx object.
        ~LicenseFileStorageEx() override;

        /// \brief Saves given license data string to the file.
        /// \param data License data string
        void saveLicense( const std::string& data ) override;

        /// \brief Load license from file.
        /// \return Encrypted license data.
        std::string loadLicense() override;

        /// \brief Removes local license file.
        void clear() override;

        //-------- Named mutex management --------

        /// \brief Set platform dependent permission (Unix like systems).
        /// \param permissions Permissions to be set
        void setPermissionsUnix( int permissions );

        /// \brief Set platform dependent permission (Windows).
        /// \param permissions Permissions to be set
        /// \details see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexexa.
        void setPermissionsWin( void* permissions );

        /// \brief Sets permissions to unrestricted access: null DACL for windows or 0666 for UNIX.
        void setPermissionsUnrestricted();

        /// \brief Sets permissions to default values: null security attributes pointer for Windows or 0644 for UNIX.
        /// \details This is the default value of the mutex permission.
        void setPermissionsDefault();

        /// \brief Get trylock timeout in milliseconds.
        /// \details Default value is 1500ms, min is 50ms.\n
        /// After timeout the SDK will try to remove mutex from the system and lock it again.
        /// \return Timeout in milliseconds.
        uint32_t lockTimeout() const;

        /// \brief Set timeout in milliseconds for trylock.
        /// \param timeout Timeout in milliseconds
        void setLockTimeout( uint32_t timeout );

    private:
        std::string m_name;
        uint32_t    m_lockTimeout;
        boost::interprocess::permissions* m_perm;
    };


    /// \brief Class that saves license in a file, derived from <c>LicenseFileStorageBase</c> class.
    /// \details Enhanced license file storage type for interprocess use.
    /// It uses file locking mechanism for synchronization. File operations of this class is thread and process safe.
    class LS_API FileStorageWithLock : public LicenseFileStorageBase
    {
    public:
        using ptr_t = std::shared_ptr<FileStorageWithLock>;

        /// \brief Allocates and constructs a FileStorageWithLock object and passes args to FileStorageWithLock().
        /// \param licensePath The path that license folder path will be set to
        /// \return A shared_ptr object that owns and stores a pointer to a newly allocated FileStorageWithLock object.
        static ptr_t create( const std::wstring& licensePath = std::wstring() );

        /// \brief FileStorageWithLock constructor. 
        /// \param licensePath The path that license folder path will be set to
        FileStorageWithLock( const std::wstring& licensePath = std::wstring() );

        /// \brief Saves given license data string to the file.
        /// \param data License data string
        void saveLicense( const std::string& data ) override;

        /// \brief Load license from file.
        /// \return Encrypted license data.
        std::string loadLicense() override;

        /// \brief Removes local license file.
        void clear() override;

        /// \brief Get file lock operation attempts count.
        /// \details Default value is 20 attempts, min is 1.
        /// \return Attempts count.
        uint32_t attemptsCount() const;

        /// \brief Set file lock operation attempts count.
        /// \param count Attempts count
        void setAttemptsCount( uint32_t count );

        /// \brief Get timeout in milliseconds between file lock attempts.
        /// \details Default value is 50ms, min is 10ms.
        /// \return Timeout in milliseconds.
        uint32_t waitTimeout() const;

        /// \brief Set timeout in milliseconds between file lock attempts.
        /// \param timeout Timeout in milliseconds
        void setWaitTimeout( uint32_t timeout );

    private:
        uint32_t m_attemptsCount;
        uint32_t m_waitTimeout;
    };
} // namespace LicenseSpirng

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_LICENSE_FILE_STORAGE_H