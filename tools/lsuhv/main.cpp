#include <uhv/file.h>
#include <util/string.h>

#include <iostream>


// ---------------------------------------------------------------------

void dump_metadata(std::ostream& out, const uh::uhv::meta_data& md)
{
    out << md.size() << " \t"
        << md.chunk_sizes().size() << " \t"
        << md.path() << " \t"
        << "\n";

    for (auto i = 0u; i < md.hashes().size(); i += 64)
    {
        out << "\t"
            << uh::util::to_hex_string(md.hashes().begin() + i, md.hashes().begin() + i + 64) << "\n";
    }
}

// ---------------------------------------------------------------------

void dump_file(std::ostream& out, const std::filesystem::path& p)
{
    out << "contents of " << p << ":\n";
    out << "size \tchunks \tpath\n";

    uh::uhv::file f(p);

    for (const auto& entry : f.deserialize())
    {
        dump_metadata(out, *entry);
    }
}

// ---------------------------------------------------------------------

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <files>\n";
        return 1;
    }

    try
    {
        for (int id = 1; id < argc; ++id)
        {
            dump_file(std::cout, argv[id]);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
