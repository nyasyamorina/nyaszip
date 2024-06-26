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


template<class clock> inline i64 to_time(time_point<clock> const& t)
{
    return system_clock::to_time_t(system_clock::now() + duration_cast<system_clock::duration>(t - clock::now()));
}


void print_help()
{
    cout << "help document:" << endl;
    cout << "=======================" << endl;
    cout << endl;
    cout << "    `nyaszip.exe [in1 [in2 [in3 ...]]] [-o out]`" << endl;
}

void _build_test_zip()
{
    auto zip = Zip::create("nyastestzip.zip");
    zip.comment("https://github.com/nyasyamorina/nyaszip");

    auto & aaa = zip.add("aaa.txt");
    aaa.password("123456", 128);
    aaa.write("the password of \"folder/test.txt\" is \"abcdef\"");
    //aaa.close();  it will be automatically closed when add a new file or close the zip

    auto & test = zip.add("folder\\test.txt");
    test.password("abcdef", 192);
    test.write("The forgotten garbage that transcends all causes and effects.");
    test.comment("Fin");

    auto & trash = zip.add("\\\\just\\for\\testing");
    trash.zip64(true);
    for (u64 total = 0; total < 6746518852 /* 2pi GB */; total += trash.buffer_length())
    {
        trash.flush_buff(trash.buffer_length());
    }
    trash.comment("Don't open !!!");

    auto & readme = zip.add("readme.txt");
    readme.write("the password of \"aaa.txt\" is \"123456\"");
    readme.comment("here is the entry");
    readme.modified(MsDosTime(946684800));

    // obviously, zip does not have any way to prevent duplicate files.
    auto & test2 = zip.add("folder\\test.txt");
    test2.password("not published password" /* AES-256 is the default encryption */);
    test2.write("made by nyasyamorina");

    zip.close();
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
        bool empty_dir = true;
        for (auto const& entry : fs::directory_iterator(path))
        {
            add_to(zip, root, rel / entry.path().filename());
            empty_dir = false;
        }
        if (empty_dir)
        {
            cout << endl << "append: " << path << endl;
            auto & dir = zip.add(rel.string());
            dir.external_attribute(FileAttributes::Directory);
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

        auto modified = to_time(fs::last_write_time(path));
        file.modified(MsDosTime(modified));
        // 3.999GiB, assume other thing in local file data except real file data is less than 1MiB
        constexpr u64 threshold = static_cast<u64>(3.999 * 1024 * 1024 * 1024);
        u64 size = fs::file_size(path);
        if (size > threshold)
        {
            file.zip64(true);
        }
        else if (size == 0)
        {
            return;
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
    //_build_test_zip();

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