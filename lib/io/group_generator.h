#ifndef IO_GROUP_GENERATOR_H
#define IO_GROUP_GENERATOR_H

#include <io/data_generator.h>

#include <list>
#include <memory>


namespace uh::io
{

// ---------------------------------------------------------------------

/**
 * A data_generator that groups multiple data_generators and returns
 * their data in order.
 */
class group_generator : public data_generator
{
public:
    void append(std::unique_ptr<data_generator>&& dg);

    std::optional<data_chunk> next() override;

    std::size_t size() const override;

private:
    std::list<std::unique_ptr<data_generator>> m_generators;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
