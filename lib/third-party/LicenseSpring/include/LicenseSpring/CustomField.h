#ifndef LS_CUSTOM_FIELD_H
#define LS_CUSTOM_FIELD_H

#ifdef _MSC_VER
#pragma once
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include <string>
#include "APIDef.h"

namespace LicenseSpring
{
    /// \brief Class for storing key-value data field.
    /// \see License::customFields(), to obtain CustomField objects from your license.
    class LS_API CustomField
    {
    public:
        /// \brief Default constructor, creates empty data field.
        CustomField() = default;

        /// \brief Constructs data field with given name (key) and value.
        /// \param name Name (key) of key-value data field
        /// \param value Value of key-value data field
        CustomField( const std::string& name, const std::string& value )
            : m_name( name )
            , m_value( value )
        {}

        /// \brief Getter method for data field name.
        /// \return CustomField name.
        const std::string& fieldName() const { return m_name; };

        /// \brief Setter method for data field name.
        /// \param name CustomField name
        void setFieldName( const std::string& name ) { m_name = name; }

        /// \brief Getter method for data field value.
        /// \return CustomField value.
        const std::string& fieldValue() const { return m_value; };

        /// \brief Setter method for data field value.
        /// \param value CustomField value
        void setFieldValue( const std::string& value ) { m_value = value; }

    private:
        std::string m_name;
        std::string m_value;
    };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // LS_CUSTOM_FIELD_H
