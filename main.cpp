#include <iostream>
#include <chrono>
#include <filesystem>

#define NYASZIP_WARN

#include "nyaszip.hpp"
#include "toml.hpp" // from "https://github.com/ToruNiina/toml11"

using namespace std;
using namespace std::chrono;
namespace fs = std::filesystem;
using namespace nyaszip;


void print_help()
{
    cout << "help document:" << endl;
    cout << "=======================" << endl;
    cout << endl;
    cout << "    `nyaszip.exe [in1 [in2 [in3 ...]]] [-o out]`" << endl;
}


/// @brief return zip file name and input files
tuple<string, list<string>> process_input(int argc, char ** argv)
{
    string zip = "nyaszip-output.zip";
    list<string> files;

    int idx = 1;
    while (idx < argc)
    {
        char const* arg = argv[idx];

        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0)
        {
            idx++;
            if (idx < argc)
            {
                zip = argv[idx];
            }
        }
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            print_help();
            exit(0);
        }
        else
        {
            files.push_back(arg);
        }
        idx++;
    }

    return {zip, files};
}


struct ZipCreateFailException : public std::exception
{
public:
    string name;

    ZipCreateFailException(string const& name_)
    : name(name_) {}

    virtual char const* what() const noexcept override
    {
        return "cannot create the zip file";
    }
};

void add_to(Zip & zip, fs::path const& root, fs::path const& rel)
{
    fs::path path = root / rel;

    if (fs::is_directory(path))
    {
        u64 file_count = 0;
        for (auto const& entry : fs::directory_iterator(path))
        {
            add_to(zip, root, rel / entry.path().filename());
            file_count += 1;
        }
        if (file_count == 0)
        {
            auto & empty_dir = zip.add(rel.string());
            empty_dir.external_attribute(FileAttributes::Directory);
        }
    }
    else if (fs::is_regular_file(path))
    {
        if (rel.filename() == "nyaszip.toml")
        {
            // TODO:
            return;
        }

        cout << endl << "append: " << path << endl;
        ifstream in(path, ios::in | ios::binary);
        if (in.fail())
        {
            cout << "cannot open file: " << path << endl;
            return;
        }

        auto & file = zip.add(rel.string());

        auto modified = fs::last_write_time(path);
        auto mtime = system_clock::to_time_t(system_clock::now() + duration_cast<system_clock::duration>(modified - file_clock::now()));
        file.modified(MsDosTime(mtime));
        // 3.999GiB, assume other thing in local file data except real file data is less than 1MiB
        constexpr u64 threshold = static_cast<u64>(3.999 * 1024 * 1024 * 1024);
        if (fs::file_size(path) > threshold)
        {
            file.zip64(true);
        }

        auto buffer = reinterpret_cast<char *>(file.buffer());
        u64 get_size;
        do
        {
            in.read(buffer, file.buffer_length());
            get_size = in.gcount();
            file.flush_buff(get_size);
        }
        while (get_size == file.buffer_length());
    }
}

void build_zip(string const& zip_name, list<string> const& files)
{
    Zip zip = Zip::create(zip_name);
    if (zip.fail())
    {
        throw ZipCreateFailException(zip_name);
    }

    for (auto const& file : files)
    {
        fs::path root = fs::absolute(file);
        fs::path rel = "";
        while (rel.empty() && root.has_relative_path())
        {
            rel = root.filename();
            root = root.parent_path();
        }

        if (rel.empty())
        {
            cout << "invalid filename: \"" << file << "\"" << endl;
        }
        else
        {
            add_to(zip, root, rel);
        }
    }
    cout << endl;
    zip.close();
}


int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        print_help();
        return 0;
    }

    auto [zip, files] = process_input(argc, argv);

    try
    {
        build_zip(zip, files);
    }
    catch(std::exception const& e)
    {
        std::cerr << e.what() << '\n';
    }
}