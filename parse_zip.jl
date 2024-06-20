# for debugging

mutable struct LocalFile
    v_need::UInt16
    flag::UInt16
    method::UInt16
    mtime::UInt16
    mdate::UInt16
    crc::UInt32
    compressed_size::UInt32
    uncompressed_size::UInt32
    name::String
    extra_field::Vector{UInt8}
    data::Vector{UInt8}
end

function LocalFile(io::IO)
    @assert read(io, UInt32) == 0x04034b50
    v_need = read(io, UInt16)
    flag = read(io, UInt16)
    method = read(io, UInt16)
    mtime = read(io, UInt16)
    mdate = read(io, UInt16)
    crc = read(io, UInt32)
    compressed_size = read(io, UInt32)
    uncompressed_size = read(io, UInt32)
    name_length = read(io, UInt16)
    extra_field_length = read(io, UInt16)
    name_buf = Vector{UInt8}(undef, name_length)
    extra_field = Vector{UInt8}(undef, extra_field_length)
    data = Vector{UInt8}(undef, compressed_size)
    read!(io, name_buf)
    read!(io, extra_field)
    read!(io, data)
    name = String(name_buf)
    LocalFile(v_need, flag, method, mtime, mdate, crc, compressed_size, uncompressed_size,
    name, extra_field, data)
end

mutable struct CentralDir
    v_made::UInt16
    v_need::UInt16
    flag::UInt16
    method::UInt16
    mtime::UInt16
    mdate::UInt16
    crc::UInt32
    compressed_size::UInt32
    uncompressed_size::UInt32
    name::String
    extra_field::Vector{UInt8}
    file_comment::String
    _1::UInt16  # disk number start
    internal_attr::UInt16
    external_attr::UInt32
    offset::UInt32
end

function CentralDir(io::IO)
    @assert read(io, UInt32) == 0x02014b50
    v_made = read(io, UInt16)
    v_need = read(io, UInt16)
    flag = read(io, UInt16)
    method = read(io, UInt16)
    mtime = read(io, UInt16)
    mdate = read(io, UInt16)
    crc = read(io, UInt32)
    compressed_size = read(io, UInt32)
    uncompressed_size = read(io, UInt32)
    name_length = read(io, UInt16)
    extra_field_length = read(io, UInt16)
    file_comment_length = read(io, UInt16)
    _1 = read(io, UInt16)
    internal_attr = read(io, UInt16)
    external_attr = read(io, UInt32)
    offset = read(io, UInt32)
    name_buf = Vector{UInt8}(undef, name_length)
    extra_field = Vector{UInt8}(undef, extra_field_length)
    file_comment_buf = Vector{UInt8}(undef, file_comment_length)
    read!(io, name_buf)
    read!(io, extra_field)
    read!(io, file_comment_buf)
    name = String(name_buf)
    file_comment = String(file_comment_buf)
    CentralDir(v_made, v_need, flag, method, mtime, mdate, crc, compressed_size, uncompressed_size,
    name, extra_field, file_comment, _1, internal_attr, external_attr, offset)
end

mutable struct CentralDirEnd
    _1::UInt16  # number of this disk
    _2::UInt16  # number of the disk with the start of the central directory
    _3::UInt16  # number of files on this disk
    files::UInt16
    central_dir_size::UInt32
    central_dir_pos::UInt32
    zip_comment::String
end

function CentralDirEnd(io::IO)
    @assert read(io, UInt32) == 0x06054b50
    _1 = read(io, UInt16)
    _2 = read(io, UInt16)
    _3 = read(io, UInt16)
    files = read(io, UInt16)
    central_dir_size = read(io, UInt32)
    central_dir_pos = read(io, UInt32)
    zip_comment_length = read(io, UInt16)
    zip_comment_buf = Vector{UInt8}(undef, zip_comment_length)
    read!(io, zip_comment_buf)
    zip_comment = String(zip_comment_buf)
    CentralDirEnd(_1, _2, _3, files, central_dir_size, central_dir_pos, zip_comment)
end

mutable struct ZipFile
    lfs::Vector{LocalFile}
    cds::Vector{CentralDir}
    cdend::CentralDirEnd
end

ZipFile(path::AbstractString) = open(io -> ZipFile(io), path)
function ZipFile(io::IO)
    seekend(io)
    pos = findsignature(io, 0x06054b50, 1 * 1024)
    pos ≡ nothing && throw("cannot find signature for end of central directory")

    seek(io, pos)
    cdend = CentralDirEnd(io)
    zip = ZipFile([], [], cdend)
    sizehint!(zip.lfs, cdend.files)
    sizehint!(zip.cds, cdend.files)

    seek(io, cdend.central_dir_pos)
    for _ ∈ Base.OneTo(cdend.files)
        cd = CentralDir(io)
        push!(zip.cds, cd)
    end

    for cd ∈ zip.cds
        seek(io, cd.offset)
        lf = LocalFile(io)
        push!(zip.lfs, lf)
    end

    zip
end

function findsignature(io::IO, sig::Integer, buff_size)
    buff_size = min(buff_size, position(io))
    seek(io, position(io) - buff_size)
    buff_pos = position(io)
    buff = Vector{UInt8}(undef, buff_size)
    read!(io, buff)

    sig_u8 = reinterpret(reshape, UInt8, [sig])
    matched = 0
    # this method will mismatch some signature with certain patterns,
    # but it is good enough for zip.
    for idx ∈ reverse(eachindex(buff))
        if buff[idx] == sig_u8[end - matched]
            matched += 1
            matched == length(sig_u8) && return buff_pos + idx - 1
        else
            matched = 0
        end
    end
    return nothing
end