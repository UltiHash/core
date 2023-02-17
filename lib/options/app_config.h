#ifndef OPTIONS_APP_CONFIG_H
#define OPTIONS_APP_CONFIG_H

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#include <options/loader.h>
#include <options/config_file.h>
#include <options/basic_options.h>

#include <iostream>


/**
 * Generate code for application config reader.
 *
 * Sample usage:
 * APPLICATION_CONFIG(
 *     (logging, uh::options::logging_options),
 *     (metrics, uh::options::metrics_options) );
 *
 * will generate the following class:
 *
 * class application_config : public uh::options::application_config_base
 * {
 * public:
 *      const uh::options::logging_options& logging() const { return m_logging; }
 *      const uh::options::metrics_options& metrics() const { return m_metrics; }
 *
 * private:
 *      uh::options::logging_options m_logging;
 *      uh::options::metrics_options m_metrics;
 * };
 */

#define APP_CONFIG_REGISTER(name, type) add(m_##name);
#define APP_CONFIG_ACCESSOR(name, type) const auto& name() const { return m_##name.config(); }
#define APP_CONFIG_MEMBERS(name, type) type m_##name;

#define APPLY(_, X, P) X P
#define APPLICATION_CONFIG_SEQ(...)                                                 \
    class application_config : public uh::options::application_config_base          \
    {                                                                               \
    public:                                                                         \
        application_config()                                                        \
        {                                                                           \
            BOOST_PP_SEQ_FOR_EACH(APPLY, APP_CONFIG_REGISTER, __VA_ARGS__)          \
        }                                                                           \
        BOOST_PP_SEQ_FOR_EACH(APPLY, APP_CONFIG_ACCESSOR, __VA_ARGS__)              \
        virtual void print_version() override                                       \
        {                                                                           \
            std::cout << "version: " << PROJECT_NAME << " "                         \
                << PROJECT_VERSION << "\n";                                         \
        }                                                                           \
        virtual void print_vcsid() override                                         \
        {                                                                           \
            std::cout << "vcsid: " << PROJECT_REPOSITORY << " - "                   \
                << PROJECT_VCSID << "\n";                                           \
        }                                                                           \
    private:                                                                        \
        BOOST_PP_SEQ_FOR_EACH(APPLY, APP_CONFIG_MEMBERS, __VA_ARGS__)               \
    }

#define APPLICATION_CONFIG(...) \
    APPLICATION_CONFIG_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

namespace uh::options
{

// ---------------------------------------------------------------------

class application_config_base
{
public:
    application_config_base();
    virtual ~application_config_base() = default;

    virtual action evaluate(int argc, const char** argv);

    void print_help();
    virtual void print_version() = 0;
    virtual void print_vcsid() = 0;

    void add(options& opts);
private:
    basic_options m_basic;
    config_file m_config_file;
    loader m_loader;
};

// ---------------------------------------------------------------------

} // namespace uh::options

#endif
