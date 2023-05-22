#ifndef IO_TEST_GENERATOR_H
#define IO_TEST_GENERATOR_H

#include <io/data_generator.h>

#include <list>


namespace uh::io::test
{

// ---------------------------------------------------------------------

class generator : public uh::io::data_generator
{
public:
    generator(std::span<const char> data, std::size_t chunk_size = 16);

    std::optional<data_chunk> next() override;
    std::size_t size() const override;

private:
    std::list<io::data_chunk> m_chunks;
    std::size_t m_size;
};

// ---------------------------------------------------------------------

} // namespace uh::io::test

#endif
