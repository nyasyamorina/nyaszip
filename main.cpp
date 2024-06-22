#include <iostream>

#define NYASZIP_WARN

#include "nyaszip.hpp"
#include "toml.hpp" // from "https://github.com/ToruNiina/toml11"

using namespace std;
using namespace nyaszip;


int main(int argc, char ** argv) {

    auto zip = Zip::create("nyastestzip.zip");
    zip.comment("https://github.com/nyasyamorina/nyaszip");

    auto & aaa = zip.add("aaa.txt");
    aaa.password("123456", 128);
    aaa.write("the password of \"folder/test.txt\" is \"abcdef\"");
    //aaa.close();  it will be automatically closed when add a new file or close the zip

    auto & test = zip.add("folder/test.txt");
    test.password("abcdef", 192);
    test.write("The forgotten garbage that transcends all causes and effects.");
    test.comment("Fin");

    auto & readme = zip.add("readme.txt");
    readme.write("the password of \"aaa.txt\" is \"123456\"");
    readme.comment("here is the entry");
    readme.modified(MsDosTime(946684800));

    // obviously, zip does not have any way to prevent duplicate files.
    auto & test2 = zip.add("folder/test.txt");
    test2.password("not published password" /* AES-256 is the default encryption */);
    test2.write("made by nyasyamorina");

    zip.close();
}