#include "rabin_fp.h"

#include <util/exception.h>

extern "C"
{

#include <chunking/rabin_polynomial.h>
#include <chunking/rabin_polynomial_constants.h>

}


namespace uh::chunking
{

// ---------------------------------------------------------------------

rabin_fp::rabin_fp()
{
    static int __rabin_init_result = initialize_rabin_polynomial_defaults();

    if(!__rabin_init_result)
    {
        THROW(util::exception, "Error initializing rabin fingerprints");
    }
}

// ---------------------------------------------------------------------

std::vector<std::size_t> rabin_fp::chunk(std::span<char> b) const
{
    std::vector<std::size_t> rv;

    rab_block_info* block = read_rabin_block(b.data(), b.size(), nullptr);

    for (auto* c = block->head; c != nullptr; c = c->next_polynomial)
    {
        if (c->length != 0)
        {
            rv.push_back(c->length);
        }
    }

    free_chunk_data(block);
    free_rabin_fingerprint_list(block->head);
    free(block);

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
