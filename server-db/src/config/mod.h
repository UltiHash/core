#ifndef SERVER_DATABASE_CONFIG_MOD_H
#define SERVER_DATABASE_CONFIG_MOD_H

#include <config/options.h>

#include <memory>


namespace uh::dbn::config
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(int argc, const char** argv);
    ~mod();

    /**
     * Handle options that can be handled in this module.
     * @return true if the application should exit afterwards.
     */
    bool handle() const;

    /**
     * Get access to parsed options.
     */
    const dbn::config::options& options() const;
private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::config

#endif
