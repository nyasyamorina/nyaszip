#include <iostream>
#include <chrono>
#include <unordered_map>
#include <optional>
#include <filesystem>

//#define NYASZIP_WARN
#define NYASZIP_ALLOW_EMPTY_FILENAME

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
MsDosTime inline last_modified_time(fs::path path)
{
    auto t1 = fs::last_write_time(path);
    auto t2 = to_time(t1);
    return MsDosTime(t2);
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


/// @return (zip file name, input paths)
tuple<string, list<string>> process_input(int argc, char ** argv)
{
    string zip = "";
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

class nyaszipconfigs
{
public:
    class Config
    {
    public:
        optional<string>    password;
        optional<u16>       AES;
        optional<MsDosTime> modified;

        bool empty() const noexcept
        {
            return !(password.has_value() || AES.has_value() || modified.has_value());
        }
        bool full() const noexcept
        {
            return password.has_value() && AES.has_value() && modified.has_value();
        }
    };

protected:
    unordered_map<fs::path, Config> _configs;     // rel path in zip file -> Config
    unordered_map<fs::path, string> _contents;
    unordered_map<fs::path, string> _comments;

public:
    void add_config_file(fs::path const& root, fs::path const& rel)
    {
        fs::path toml_path = root / rel / "nyaszip.toml";

        auto const config_res = toml::try_parse(toml_path);
        if (config_res.is_err())
        {
            cerr << "cannot parse toml file at " << toml_path << ':' << endl;
            cerr << config_res.unwrap_err().at(0) << endl;
            return;
        }
        add_config(rel, config_res.unwrap().as_table());
    }

    void add_config(fs::path const& rel, toml::table const& ct)
    {
        Config config;

        for (auto const& entry : ct)
        {
            auto const& [key, value] = entry;
            if (value.type() == toml::value_t::table)
            {
                add_config(rel / key, value.as_table());
                continue;
            }

            i64 tmp;
            if (key == "password")
            {
                if (value.type() == toml::value_t::string)
                {
                    config.password = value.as_string();
                }
                else if (value.type() == toml::value_t::integer)
                {
                    tmp = value.as_integer();
                    if (tmp != 0)
                    {
                        cout << "should set to 0 to disable password";
                    }
                    config.password = "\xFF";
                }
                else
                {
                    // TODO: more user-friendly message
                    cerr << "got an unsupported type for `password`, expect a string, got " << value.type() << endl;
                }
            }
            else if (key == "AES")
            {
                if (value.type() == toml::value_t::integer)
                {
                    tmp = value.as_integer();
                    if (tmp != 128 && tmp != 192 && tmp != 256)
                    {
                        cerr << "got an invalid `AES`, expect 128, 192 or 256, got " << tmp << ", reselect as 256" << endl;
                        tmp = 256;
                    }
                    config.AES = static_cast<u16>(tmp);
                }
                else
                {
                    // TODO: more user-friendly message
                    cerr << "got an unsupported type for `AES`, expect a integer, got " << value.type() << endl;
                }
            }
            else if (key == "modified")
            {
                if (value.type() == toml::value_t::local_date
                 || value.type() == toml::value_t::local_datetime
                 || value.type() == toml::value_t::offset_datetime
                ) {
                    tmp = system_clock::to_time_t(toml::get<system_clock::time_point>(value));
                    config.modified = MsDosTime(tmp);
                }
                else
                {
                    cerr << "got an unsupported type for `modified`, expect a datetime, got " << value.type() << endl;
                }
            }
            else if (key == "content")
            {
                if (value.type() == toml::value_t::string)
                {
                    _contents[rel] = value.as_string();
                }
                else
                {
                    // TODO: more user-friendly message
                    cerr << "got an unsupported type for `content`, expect a string, got " << value.type() << endl;
                }
            }
            else if (key == "comment")
            {
                if (value.type() == toml::value_t::string)
                {
                    _comments[rel] = value.as_string();
                }
                else
                {
                    // TODO: more user-friendly message
                    cerr << "got an unsupported type for `comment`, expect a string, got " << value.type() << endl;
                }
            }
            else
            {
                // TODO: more user-friendly message
                cerr << "got an unexpect key: `" << key << "`, pass" << endl;
            }
        }

        if (!config.empty())
        {
            _configs[rel] = config;
        }
    }

    unordered_map<fs::path, string> const& contents() const noexcept
    {
        return _contents;
    }

    tuple<Config, string> get(fs::path const& rel) const
    {
        string comment = "";
        if (auto entry = _comments.find(rel); entry != _comments.end())
        {
            comment = entry->second;
        }

        Config config;
        if (auto entry = _configs.find(rel); entry != _configs.end())
        {
            config = entry->second;
        }
        fs::path path = rel;
        while (!config.full() && path.has_relative_path())
        {
            path = path.parent_path();
            if (auto entry = _configs.find(path); entry != _configs.end())
            {
                Config const& config_ = entry->second;
                if (!config.password.has_value() && config_.password.has_value())
                {
                    config.password = config_.password;
                }
                if (!config.AES.has_value() && config_.AES.has_value())
                {
                    config.AES = config_.AES;
                }
                if (!config.modified.has_value() && config_.modified.has_value())
                {
                    config.modified = config_.modified;
                }
            }
        }
        return {config, comment};
    }
};

class nyaszipbuilder
{
public:
    // (rel path, file size, the last modified time), file size = -1 if path is a empty dirctory
    using Path = tuple<fs::path, u64, MsDosTime>;
    // 3.999GiB, assume other thing in local file data except real file data is less than 1MiB
    static constexpr u64 THRESHOLD_SIZE = static_cast<u64>(3.999 * 1024 * 1024 * 1024);

protected:
    string _zip_name;
    nyaszipconfigs _configs;
    unordered_map<fs::path, list<Path>> _paths;

    void _add_path(fs::path const& root, fs::path const& rel)
    {
        if (!_paths.contains(root))
        {
            _paths[root] = list<Path>();
        }
        list<Path> & rels = _paths.at(root);

        fs::path path = root / rel;
        if (fs::is_directory(path))
        {
            bool empty_dir = true;
            /* make sure the files in the directory are added before the subdirectories
               to ensure that the `nyaszip.toml` in the subdirectories can override the above one. */
            for (auto const& entry : fs::directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    _add_path(root, rel / entry.path().filename());
                }
                empty_dir = false;
            }
            for (auto const& entry : fs::directory_iterator(path))
            {
                if (entry.is_directory())
                {
                    _add_path(root, rel / entry.path().filename());
                }
                empty_dir = false;
            }
            if (empty_dir)
            {
                rels.push_back({rel, static_cast<u64>(-1), last_modified_time(path)});
            }
        }
        else if (fs::is_regular_file(path))
        {
            if (rel.filename() == "nyaszip.toml")
            {
                _configs.add_config_file(root, rel.parent_path());
            }
            else
            {
                rels.push_back({rel, fs::file_size(path), last_modified_time(path)});
            }
        }
    }

    LocalFile & _add_file(Zip & zip, fs::path const& rel, u64 filesize, MsDosTime modified = MsDosTime(time(nullptr))) const
    {
        auto const [config, comment] = _configs.get(rel);
        LocalFile & file = zip.add(rel.string());

        file.comment(comment);
        file.modified(config.modified.value_or(modified));
        if (filesize == static_cast<u64>(-1))
        {
            file.external_attribute(FileAttributes::Directory);
        }
        else if (filesize > THRESHOLD_SIZE)
        {
            file.zip64(true);
        }
        if (auto pswd = config.password.value_or("\xFF"); pswd != "\xFF")
        {
            file.password(pswd, config.AES.value_or(256));
        }

        return file;
    }

public:
    void prepare(string const& zip_name, list<string> const& paths)
    {
        _zip_name = zip_name;
        bool has_zip_name = !zip_name.empty();

        /* make sure the files in the directory are added before the subdirectories
           to ensure that the `nyaszip.toml` in the subdirectories can override the above one. */
        // (root, rel, is_directory?)
        list<tuple<fs::path, fs::path, bool>> paths_;

        for (string const& path : paths)
        {
            fs::path root = fs::absolute(path);
            fs::path rel = "";
            bool is_directory = fs::is_directory(root);
            while (rel.empty() && root.has_relative_path())
            {
                rel = root.filename();
                root = root.parent_path();
            }

            if (rel.empty())
            {
                cerr << "got an invalid path: \"" << path << "\"" << endl;
                continue;
            }
            if (!has_zip_name)
            {
                _zip_name = rel.filename().string();
                if (paths.size() > 1)
                {
                    _zip_name += ", etc";
                }
                _zip_name += ".zip";
                has_zip_name = true;
            }
            paths_.push_back({root, rel, is_directory});
        }
        for (auto const& [root, rel, is_directory] : paths_)
        {
            if (!is_directory)
            {
                _add_path(root, rel);
            }
        }
        for (auto const& [root, rel, is_directory] : paths_)
        {
            if (is_directory)
            {
                _add_path(root, rel);
            }
        }

        if (_zip_name.empty())
        {
            _zip_name = "nyaszip.zip";
        }
    }

    void start()
    {
        Zip zip = Zip::create(_zip_name);
        if (zip.fail())
        {
            throw ZipCreateFailException(_zip_name);
        }
        if (auto comment = get<1>(_configs.get("")); !comment.empty())
        {
            zip.comment(comment);
        }

        for (auto const& [rel, content] : _configs.contents())
        {
            LocalFile & file = _add_file(zip, rel, content.size());
            file.write(content);
        }

        for (auto const& [root, rels] : _paths)
        {
            for (auto const& [rel, filesize, modified] : rels)
            {
                LocalFile & file = _add_file(zip, rel, filesize, modified);

                ifstream in(root / rel, ios::in | ios::binary);
                if (in.fail())
                {
                    cerr << "cannot open file: " << root / rel << endl;
                    continue;
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

        zip.close();
    }
};


void throweror(std::exception const& err, string const& stage)
{
    cerr << "\n\nan exception occurred during " << stage << " stage: \n" << endl;
    cerr << err.what() << endl;
    cerr << "\n please file an issue on GitHub so I can fix the bug" << endl;
    cout << "enter to exit..." << endl;
    cin.ignore(numeric_limits<i64>::max(), '\n');
    exit(1);
}

int main(int argc, char ** argv)
{
    //_build_test_zip();

    if (argc < 2)
    {
        print_help();
        return 0;
    }

    auto [zip_name, paths] = process_input(argc, argv);
    nyaszipbuilder builder;

    try
    {
        builder.prepare(zip_name, paths);
    }
    catch(std::exception const& err)
    {
        throweror(err, "preparing");
    }
    try
    {
        builder.start();
    }
    catch(std::exception const& err)
    {
        throweror(err, "running");
    }
}