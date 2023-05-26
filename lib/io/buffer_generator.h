#ifndef IO_BUFFER_GENERATOR_H
#define IO_BUFFER_GENERATOR_H

#include <io/data_generator.h>

#include <vector>


namespace uh::io
{

// ---------------------------------------------------------------------

class buffer_generator : public data_generator
{
public:
    buffer_generator(std::vector<char>&& b);

    std::optional<data_chunk> next() override;

    std::size_t size() const override;
private:
    std::vector<char> m_buffer;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
