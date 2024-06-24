# nyasnote

---

## file structure

**should** use z64 if file size exceeds 0xFFFFFFFF.

**should** use little endian store all values.

- local file 1
- ...
- local file n
- archive decryption header
- archive extra data record
- central direction
- end of central direction

## local file

file data can be empty if this is a folder.

- local file header
- encrypsion header (not compress)
- file data (variable)
- data descriptor

## local file header

**must** be accompanied by a corresponding central directory header.

**must not** be compressed local file header.

**must** set compressed/uncompressed size to -1 if using z64, and store the actual values in extra field.

**must** set crc-32, compressed/uncompressed size to 0 if 0x0008 of general purpose bit flag is set.

**must** set crc-32 to 0 if using AE-2.

**must** set uncompressed size to 0 if not using z64 and 0x2000 of general purpose bit flag is set.

**must** use a unique hexadecimal mask file name if 0x2000 of general purpose bit flag is set.

**must** use forward slashes '/' for path separator, and cannot have any leading slash.

last modified file time/date is MS-DOS format.

- u32 - signature (0x04034B50, PK\x03\x04)
- u16 - version needed to extract
- u16 - general purpose bit flag
- u16 - compression method
- u16 - last modified file time
- u16 - last modified file date
- u32 - crc-32
- u32 - compressed size
- u32 - uncompressed size
- u16 - file name length
- u16 - extra field length
- ----- file name   (variable size)
- ----- extra field (variable size)

## data descriptor

**should not** exit if using Central Directory Encryption method, if exit **must** set all value to 0.

**must** exit if 0x08 of general purpose bit flag is set.

**should** be used when it is unseekable stream.

**may not** exit signature.

**should** write signature.

- u32 - signature (0x08074B50, PK\x07\x08)
- u32 - crc-32
- u32 (u64 for z64) - compressed size
- u32 (u64 for z64) - uncompressed size

## archive decryption header

**must** exit if using Central Directory Encryption method.

**must** use z64 if exit.

## archive extra data record

**may** not exit even using Central Directory Encryption method.

- u32 - signature (0x08064B50, PK\x06\x08)
- u32 - extra field length
- ----- extra field data (variable size)

## central direction

**must** compress all if using Central Directory Encryption method.

**must not** compress digital signature.

- central directory header 1
- ...
- central directory header n
- digital signature

## central directory header

**must** set disk number start and relative offset of local header to -1 if using z64, and store the actual values in extra field.

- u32 - signature (0x02014B50, PK\x01\x02)
- u16 - version made by
- u16 - version needed to extract
- u16 - general purpose bit flag
- u16 - compression method
- u16 - last modified file time
- u16 - last modified file date
- u32 - crc-32
- u32 - compressed size
- u32 - uncompressed size
- u16 - file name length
- u16 - extra field length
- u16 - file comment length
- u16 - disk number start
- u16 - internal file attributes
- u32 - external file attributes
- u32 - relative offset of local header
- ----- file name    (variable size)
- ----- extra field  (variable size)
- ----- file comment (variable size)

## digital signature

store the information on the central direction encryption.

- u32 - signature (0x05054B50, PK\x05\x05)
- u16 - data size
- ----- signature data (variable size)

## end of central direction

**must** have only one.

- zip64 end of central directory record
- zip64 end of central directory locator
- end of central directory record

## zip64 end of central directory record

**must** set correct version needed to extract for indicate version 1 or 2.

**should** count size of zip64 end of central directory record start at version made by. `Size = SizeOfFixedFields + SizeOfVariableData - 12`.

**may** follow by special purpose data.

- u32 - signature (0x06064B50, PK\x06\x06)
- u64 - size of zip64 end of central directory record
- u16 - version made by
- u16 - version needed to extract
- u32 - number of this disk
- u32 - number of the disk with the start of the central directory
- u64 - total number of entries in the central directory on this disk
- u64 - total number of entries in the central directory
- u64 - size of the central directory
- u64 - offset of start of central directory with respect to the starting disk number
- ----- zip64 extensible data sector (variable size)

## zip64 end of central directory locator

- u32 - signature (0x07064B50, PK\x06\x07)
- u32 - number of the disk with the start of the zip64 end of central directory
- u64 - relative offset of the zip64 end of central directory record
- u32 - total number of disks

## end of central directory record

**should** use z64 if some value is too large, and set this value to -1.

**must** set number of this disk, number of the disk with the start of the central directory, total number of entries in the central directory on this disk, total number of entries in the central directory, size of the central directory and offset of start of central directory with respect to the starting disk number to -1 if using z64, and store the actual values in zip64 end of central directory record.

**must not** be compressed .ZIP file comment length.

- u32 - signature (0x06054B50, PK\x05\x06)
- u16 - number of this disk
- u16 - number of the disk with the start of the central directory
- u16 - total number of entries in the central directory on this disk
- u16 - total number of entries in the central directory
- u32 - size of the central directory
- u32 - offset of start of central directory with respect to the starting disk number
- u16 - .ZIP file comment length
- ----- .ZIP file comment (variable size)

## version needed to extract

- 1.0 - Default value
- 1.1 - File is a volume label
- 2.0 - File is a folder (directory)
- 2.0 - File is compressed using Deflate compression
- 2.0 - File is encrypted using traditional PKWARE encryption
- 2.1 - File is compressed using Deflate64(tm)
- 2.5 - File is compressed using PKWARE DCL Implode
- 2.7 - File is a patch data set
- 4.5 - File uses z64 format extensions
- 4.6 - File is compressed using BZIP2 compression*
- 5.0 - File is encrypted using DES
- 5.0 - File is encrypted using 3DES
- 5.0 - File is encrypted using original RC2 encryption
- 5.0 - File is encrypted using RC4 encryption
- 5.1 - File is encrypted using AES encryption
- 5.1 - File is encrypted using corrected RC2 encryption**
- 5.2 - File is encrypted using corrected RC2-64 encryption**
- 6.1 - File is encrypted using non-OAEP key wrapping***
- 6.2 - Central directory encryption
- 6.3 - File is compressed using LZMA
- 6.3 - File is compressed using PPMd+
- 6.3 - File is encrypted using Blowfish
- 6.3 - File is encrypted using Twofish

## general purpose bit flag

- 0x0001 - file is encrypted
- 0x0006 - compression method 6 - Implode

    - 0x0002 - if (set/not set), using an (8K/4K) sliding dictionary.
    - 0x0004 - if (set/not set), using (3/2) Shannon-Fano trees were used to encode the sliding dictionary output.

- 0x0006 - compression method 8 - Deflate

    - 0x0000 - Normal (-en) compression option was used.
    - 0x0002 - Maximum (-exx/-ex) compression option was used.
    - 0x0004 - Fast (-ef) compression option was used.
    - 0x0006 - Super Fast (-es) compression option was used.

- 0x0006 - compression method 14 - LZMA

    - 0x0002 - if (set/not set), (using/not using) end-of-stream (EOS) marker to mark the end of the compressed data stream, and the compressed data size (may not/must) known.

- 0x0006 - other compression methods

    - undefied

- 0x0008 - data descriptor thing

- 0x0010 - reserved for compression method 8 - Deflate

- 0x0020 - file is compressed patched data

- 0x0040 - Strong encryption, and **must** set 0x0001 too

- 0x0780 - unused

- 0x0800 - **must** encode file name and comment using utf-8

- 0x1000 - reserved

- 0x2000 - masked actual value in local file header when using Central Directory Encryption method

- 0xC000 - reserved

## compression method

- 00 - Store
- 07 - Tokenization
- 08 - Deflate
- 09 - Deflate64(tm)
- 10 - PKWARE Data Compression Library Imploding (old IBM TERSE)
- 12 - BZIP2
- 14 - LZMA
- 16 - IBM z/OS CMPSC
- 18 - IBM TERSE
- 19 - LZ77
- 93 - Zstandard
- 94 - MP3
- 95 - XZ
- 96 - JPEG
- 97 - WavPack
- 98 - PPMd v1 r1
- 99 - AE-x encryption

## crc-32

example:

```c++
uint32_t crc32(uint32_t crc, uint8_t byte) {
    crc ^= 0xFFFFFFFF /* <- pre-condition */ ^ byte;
    for (uint8_t i = 0; i < 8; i++) {
        bool b = crc & 1;
        crc >>= 1;
        if (b) { crc ^= 0xEDB88320; }
    }
    return crc ^ 0xFFFFFFFF; /* <- post-conditioned */
}
```

## extra field

- u16 - header id 1
- u16 - data size 1
- data 1
- u16 - header id 2
- u16 - data size 2
- data 2
- ...

### z64 extra field:

**must** have fields are set to -1 in original header, other fields are optional.

**must** follow the order.

**must** set compressed/uncompressed size to 0 in local file header if 0x2000 of general purpose bit flag is set.

- u16 - header id (0x0001)
- u16 - size of data follows
- u64 - uncompressed size
- u64 - compressed size
- u64 - relative offset of local header
- u32 - disk number start

### Strong Encryption Header

- u16 - header id (0x0017)
- u16 - size of data follows
- u16 - format
- u16 - encryption identifier
- u16 - bit length of encryption key
- u16 - flags
- ----- Certificate decryption extra field (variable)

### AE-x encryption

- u16 - header id (0x9901)
- u16 - size of data follows (0x0007)
- u16 - integer verion number specific to the zip vendor (0x0001 or 0x0002)
- u16 - vendor id ("AE")
- u8  - integer mode value indicating AES encryption strength (0x01(2|3): AES-128(192|256))
- u16 - the actual compression method used to compress the file

## AES encryption file data

encryption after compression, and encrypt using AES algorithm with "CTR" mode, block size 16 bytes (the last one may be smaller).

key generation using PBKDF2 algorithm with pseudorandom function HMAC-SHA1 and 1000 iteration, where HMAC-SHA1 is a HMAC using SHA-1. separate generate key to 128(192|256) + m + 16 bits part for encryption key, authentication key and password verification value.

- ----- salt (variable, AES-128(192|256) => u64(u96|u128))
- u16 - password verification value
- ----- encrypted file data (variable)
- u80 - authentication code

## PBKDF2

DK = PBKDF2\[PRF\](P, S, c, dkLen)

- DK - output
- PRF - pseudorandom function
- P - password
- S - salt
- c - itertion count
- dkLen - length of output
- hLen - the length of output from PRF

1. let l := ceil(dkLen / hLen), r := dkLen - (l - 1) * hLen
2. let T_i := U_1 xor U_2 xor ... xor U_c, where U_{k} := PRF(P, U_{k-1}) and U_1 := PRF(P, S concat i), i is a 32-bit positive integer.
3. let DK := T_1 concat T_2 concat ... concat T_l\[0..r-1\]

## HMAC

output = HMAC\[H\](K, m) = H((K' xor 0x5C) concat H((K' xor 0x36) concat m))

- H - hash function
- K - secret key
- m - message
- b - the input block size of H
- K' = if the length of K lager than b, equal to H(K), otherwise, equal to K padded with 0
