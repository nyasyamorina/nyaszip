# nyaszip

A minimal implementation for multi-password ZIP file.

---

## nyaszip.hpp

This is a [single-file](nyaszip.hpp) zip API, you can copy this file and use in your project.

See more: [document](DOCUMENT.md).

---

## Behavior of third-party softwares

Admittedly, multi-password is not the usual case in zip files, although there are no regulations prohibiting this behavior, so there are some unexpected behavior in third-party software, generally:

- In GUI:

    Because there isn't using the Strong Encryption, so most software will not ask the password when opening the zip file.

    But it will ask when try to open an encrypted file, and cache the password in this session, then at the rest time of the session it will automatically use the cached password for all encrypted files.

    If the software encounters a file that cannot be opened it with the cached password, it just throw an error to the user, and the user must close the GUI window and reopen the zip file to fresh the cached password.

- In directly uncompression:

    Most softwares just ask the password once, and use it all the time. If encounters a file that cannot open with the password, the software will skip it (but I had seem a trash software, it just stuck there), but those files still appeare in the uncompressed directory, but the files is empty.

Currently I'm using [Bandizip](https://www.bandisoft.com/bandizip/) (definitely not a sponsor), it has a outstanding performance of the multi-password zip files: If it encounters a file that cannot be opened it with the cached password, it will ask user once again, instead of throwing errors or skip it (although skipping is somtimes necessary).

---

## nyaszip.toml

**This is the configuration file for nyaszip.**

There are 2 types of keys (inheritable, non-inheritable) and 5 different keys to configure the behavior of `nyaszip`:

- (inheritable): this key will be set for all files in this directory, unless otherwise set specifically for file or subdirectory.
- (non-inheritable): this key will only be set for the specific file.

Keys:

- password (inheritable):

    The password should be string, and can be empty. set to 0 (integer) for unset the password.

- AES (inheritable):

    Set the AES mode for encrytion, only can be 128, 192 or 256. Automatically set to 256 if the password is set but AES is not set.

- modified (inheritable):

    Set the last modified time. Automatically set to the last modified time of the file on disk if this is not set. The format of this is `yyyy-mm-ddTHH-MM-SSZZZZZ`, for example, 5:47:23 on December 25, 2012 is `2012-12-25T05:47:23`.

- comment (non-inheritable):

    set the comment for the specified file. When a "nyaszip.toml" is passed into `nyaszip` as a command line parameter (or drag into `nyaszip`), then comment at root table will be treated as the comment of the zip file.

- content (non-inheritable):

    Create a file in the zip file that is not on disk, and specify the content of this file. Note that the behavior is not defined if set content to a file that already on disk.

Due to the design of toml, the character `.` in table name is equivalent to the path separator `/` in "nyaszip.toml", please use double quotes `"` to enclose the file name.

Each "nyaszip.toml" only applies to the current directory and its contents. Each directory can contain a "nyaszip.toml", and the "nyaszip.toml" of the subdirectory will override the settings from the parent directory.

Check [example](example) to see how "nyaszip.toml" actually works.

### Example:

``` toml
password = "nyaszip" # set the global password

["README.txt"]  # configuration for the file "README.txt"
password = 0    # unset the password
modified = 2022-02-22T22:22:22  # set the last modified time
comment = "Here is the entry"   # set the comment

[dir1]      # configuration for the whole directory "dir1/"
AES = 128   # set the AES mode to 128 for all files in "nyastest/dir1"

["dir1/test.txt"]   # configuration for the whole directory "dir1/test.txt"
content = "Here is the file that is not on disk."

[dir2]  # this table would not work if there is a "nyaszip.toml" in "dir2/"
password = ""   # yeah, the empty password is valid
```

---

### TODO Lists

- fix that cannot add comment to the not empty directory
- add progress bar
- more beautifuller output message