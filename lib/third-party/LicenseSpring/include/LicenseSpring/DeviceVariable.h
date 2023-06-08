#ifndef LS_DEVICE_VARIABLE_H
#define LS_DEVICE_VARIABLE_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include <ctime>
#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief Class for storing key-value device variable.
    /// \details This class is in use for passing device variables to the backend (see License::sendDeviceVariables)
    class LS_API DeviceVariable
    {
    public:
        /// \brief Default constructor, creates empty device variable.
        DeviceVariable();

        /// \brief Constructs device variable with given name (key) and value.
        /// \param name Name (key) of key-value data field for device variable
        /// \param value Value of key-value data field for device variable
        DeviceVariable( const std::string& name, const std::string& value );

        /// \brief Constructs device variable with given data.
        /// \param id Device variable identifier
        /// \param name Name (key) of key-value data field for device variable
        /// \param value Value of key-value data field for device variable
        /// \param dateTimeUpdated Date of last update
        DeviceVariable( int64_t id, const std::string& name, const std::string& value,
                        const std::string& dateTimeUpdated );

        /// \brief Check if variable is considered empty.
        /// \return True if name or value is empty.
        bool isEmpty() const;

        /// \brief Getter method for device variable id.
        /// \details This value is available after getting device variables data from the backend.
        /// \return DeviceVariable identifier.
        int64_t id() const { return m_id; }

        /// \brief Getter method for device variable name.
        /// \return DeviceVariable name in lowercase.
        const std::string& name() const { return m_name; }

        /// \brief Setter method for device variable name.
        /// \param name DeviceVariable name.
        void setName( const std::string& name ) { m_name = name; }

        /// \brief Getter method for device variable value.
        /// \return DeviceVariable value.
        const std::string& value() const { return m_value; }

        /// \brief Setter method for device variable value.
        /// \param value DeviceVariable value.
        void setValue( const std::string& value ) { m_value = value; }

        /// \brief Getter method for device variable date time when it was updated on the backend.
        /// \details This value is available after getting device variables data from the backend.
        /// \return Last date time DeviceVariable was updated.
        tm dateTimeUpdated() const;

        /// \brief Getter method for device variable date time in UTC when it was updated on the backend.
        /// \details This value is available after getting device variables data from the backend.
        /// \return Last date time in UTC DeviceVariable was updated.
        tm dateTimeUpdatedUtc() const;

    private:
        int64_t     m_id;
        std::string m_name;
        std::string m_value;
        std::string m_dateTimeUpdated;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_DEVICE_VARIABLE_H
