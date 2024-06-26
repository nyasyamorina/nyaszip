# nyaszip.hpp

This is a introduction for [`nyaszip.hpp`](nyaszip.hpp).

If you're looking for standard documents, here is some I used:

- [the ZIP file "standard"](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT) from PKWARE.

    note: the "magic number" used in crc-32 from section 4.4.7 seems to be not correct, may be I just don't use it in a correct way? Anyway, the crc-32 calculation described in the wiki is correct.

- [the file attributes](https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants)

- [the starndard](http://www.faqs.org/rfcs/rfc2898.html) included PBKDF2 key generation.

- [the AES standard](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197.pdf), also included step by step example result in the end of file, nice for debugging.

- some simple stuff just using wiki: [SHA-1](https://en.wikipedia.org/wiki/SHA-1), [HMAC](https://en.wikipedia.org/wiki/HMAC), [CTR encrypt mode](https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation), [crc-32](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)

- [the PCG random generator](https://www.pcg-random.org) for generating salt.

- [the AES encrytion for ZIP](http://www.winzip.com/aes_info.htm) from WinZip.

    note: there are few detail missing in the website:

    1. there is no `nonce` in the CTR encryption mode.

    2. using `HMAC` with `SHA-1` to generate the authentication code, and only use the first `10` byte of `SHA-1` output as the code.

    3. the length of the key generated from `PBKDF2` is `2 * KEY_LENGTH + 2`, where `KEY_LENGTH` is the AES mod (`KEY_LENGTH = (16|24|32)` for AES-(128|192|256)). The first `KEY_LENGTH` bytes is the AES key, the last `2` bytes is the password verification value and the middle `KEY_LENGTH` bytes is the key for authentication `HMAC`.

---

### nyaszip::Zip

You can use `Zip(ostream output)` to create a zip writter at the `output` stream, or use `Zip::create(string path)` to create a zip file at `path` and get the writer.

Use the `state()` method to get the `WritingState` of the zip writter, it will be `Writing` after created.

During the `Writing` state, you can use `add(string file_name)` method to add a file into zip and get a `LocalFile &` object. The `add` method will automatically close the previous `LocalFile`.

the name of file must follow some rules:

1. not contain a drive or device letter,
2. use `/` as the path separator, instead of `\`,
3. cannot starts with any `/`.

The file name will be automatically fixed by `LocalFile::safe_file_name(string)` to obey rules 2 and 3.

Using the `current()` method can get the pointer of the last added `LocalFile`, it will return `NULL` if there is no file or the zip writter is closed. And use `close_current()` instead of `current()->close()` to close current file.

Use the `comment(string)` to add or change the comment for the zip file.

Use `close()` to close the zip writter, the state will become `Closed` after closing. this method will also automatically close the last `LocalFile`. The `close()` method must be called before exit or deleting the output stream.

After closing, any change to the zip file is invalid and throw an error, including adding file and changing comment. Calling `close()` multiple time is allowed, but it will just run at the first time.

---

### nyaszip::LocalFile

You can use `Zip::add(string file_name)` to add a file into the zip, see above.

Use the `state()` method to get the `WritingState` of the file, it will be `Preparing` after created.

During the `Preparing` state, you can set some property for the file, like changing file name or comment, specifying the file name and the comment are the utf-8 encoded, setting the file attribute using `external_attribute(u32)`, changing the last modified time,

But the most useful thing here is using `password(string)` to set the password for the file, note that the password can be empty. Also you can use `password(nullptr)` to unset the password.

The default encrytion is AES-256, you can specify the AES mod after the password argument: `password(string, u16 bits)`, where `bits` can be 128, 192 or 256.

Please call `zip64(true)` during the `Preparing` state to declare the file size may be larger than 4GB, otherwise, it will throw an error if writing over 4GB data.

You can start writing data into file after preparing by calling `start()` method. Calling this method is optional, it will be automatically called before actually writing data.

There are two way to writing data:

1. use `write(u8 const*, u64 length)` to write data, this is the common use for data writing.

2. use `buffer()` to get the internal buffer pointer, and directly writing data into it, and then call `flush_buffer(u64 length)` to flush `length` bytes data into file, where the maximum length of internal buffer is `buffer_length()`. This is good for prevent second buffering and copying. (not ready for widely use)

After writing, you can optionally call `close()` to close file, it will be automatically called in `Zip` anyway, so, forget about it.

There are few thing can be changed after file closed, or even before the zip writer closed: the last modified time and the general purpose bit flag (only for setting the utf-8 specification for now).

---

## TODO Lists (not sorted)

- more modern way of storing the last modified times

- implantment `LocalFile : streambuf` for using `ostream` wrap araound it to `<<`

- use `u8string` instead of `string` to store file names and comments

- add NTFS or UNIX extra field to support more file information

- add compression

- add the Strong Encryption to encrypt file path (maybe will not added, because the Strong Encryption will make the zip file only use one password)

- add ZIP64 format to supported the zip file lager than 4GiB

- add the multi volume archive support