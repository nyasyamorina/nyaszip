# nyaszip

A minimal implementation for multi-password ZIP file.

---

### Features

- full AES encrption
- add comment for files or zip
- change the last modified time for files

### TODO Lists

- add compression method
- ZIP64 format support zip file greater than 4GB
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