#include <uhv/file.h>
#include <iostream>


// ---------------------------------------------------------------------

void dump_metadata(std::ostream& out, const uh::uhv::meta_data& md)
{
    out << md.path() << " \t"
        << md.chunk_sizes().size() << " \t";

    try
    {
        out << md.size();
    }
    catch (const std::bad_optional_access&)
    {
    }

    out << "\n";
}

// ---------------------------------------------------------------------

void dump_file(std::ostream& out, const std::filesystem::path& p)
{
    out << "contents of " << p << ":\n";

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
