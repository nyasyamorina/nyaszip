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

### TODO Lists

- auto process files with command line input
- use file `nyaszip.toml` to indicate file configurations

**todo**

### nyaszip.toml [TODO]

this file will not appear in the final zip file.

```toml
comment = "https://github.com/nyasyamorina/nyaszip"

[aaa.txt]
password = "123456"
AES_bits = 128
# search file if there is no `content` key in this file section

[folder/test.txt]
password = "abcdef"
AES_bits = 192
# search file if there is no `content` key in this file section

[readme.txt]
comment = "here is the entry"
modified = "2000-01-01T00:00:00"
# search file if there is no `content` key in this file section

[folder/test.txt]
password = "not published password"
content = "made by nyasyamorina"
```