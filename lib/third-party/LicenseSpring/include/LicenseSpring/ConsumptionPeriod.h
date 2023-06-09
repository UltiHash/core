#pragma once

#include <string>
#include "APIDef.h"
#include "PODTypes.h"

namespace LicenseSpring
{
    /// \brief Class used for encapsulating LSConsumptionPeriod and converting cosumption period to/from string
    class LS_API ConsumptionPeriod
    {
    public:
        /// \brief Converts string to ConsumptionPeriod.
        /// \param str String to convert to ConsumptionPeriod
        /// \return ConsumptionPeriod of corresponding string.
        static ConsumptionPeriod fromString( const std::string& str );

        /// \brief Constructs ConsumptionPeriod with type ConsumptionPeriodNone.
        ConsumptionPeriod();

        /// \brief Constructs ConsumptionPeriod with given period type.
        /// \param period Consumption reset period
        ConsumptionPeriod( LSConsumptionPeriod period );

        operator LSConsumptionPeriod() const;

        /// \brief Converts ConsumptionPeriode to a string
        /// \return Corresponding string based on consumption reset period.
        std::string toString() const;

        /// \brief Converts ConsumptionPeriode to a string, with first letter capitalized.
        /// \return Same as toString(), but with first letter capitalized.
        std::string toFormattedString() const;

    private:
        LSConsumptionPeriod m_period;
    };
}