function bitlength(b::Unsigned)
    msb = 0x0
    while b ≠ 0
        b >>= 1
        msb += 1
    end
    return msb
end

function polyGF2mul(b1::T, b2::T) where {T <: Unsigned}
    m = zero(T)
    while b1 ≠ 0 && b2 ≠ 0
        isodd(b2) && (m ⊻= b1)
        b1 <<= 1
        b2 >>= 1
    end
    return m
end

function polyGF2divrem(b1::T, b2::T) where {T <: Unsigned}
    lb1 = bitlength(b1)
    lb2 = bitlength(b2)
    d = zero(T)
    while lb1 ≥ lb2
        d |= one(T) << (lb1 - lb2)
        b1 ⊻= b2 << (lb1 - lb2)
        lb1 = bitlength(b1)
    end
    return (d, b1)
end

function polyGF2modmul(b1::T, b2::T, divisor::T) where {T <: Unsigned}
    m = zero(T)
    lr = bitlength(divisor)
    (b1, b2) = getindex.(polyGF2divrem.((b1, b2), divisor), 2)
    while b1 ≠ 0 && b2 ≠ 0
        isodd(b2) && (m ⊻= b1)
        b1 <<= 1
        bitlength(b1) ≥ lr && (b1 ⊻= divisor)
        b2 >>= 1
    end
    return m
end

function polyGF2modmulinv(b::T, divisor::T) where {T <: Unsigned}
    b == 0 && return b
    (r1, r2) = (divisor, b)
    (s1, s2) = T .|> (zero, one)
    while r2 ≠ 1
        (r1, (d, r2)) = (r2, polyGF2divrem(r1, r2))
        (s1, s2) = (s2, s1 ⊻ polyGF2mul(s2, d))
    end
    return s2
end


bytemul(a::UInt8, b::UInt8) = UInt8(polyGF2modmul(UInt16(a), UInt16(b), 0x011B))
byteinv(a::UInt8) = UInt8(polyGF2modmulinv(UInt16(a), 0x011B))

function wordmul(a::NTuple{4, UInt8}, b::NTuple{4, UInt8})
    d1 = bytemul(a[1], b[1]) ⊻ bytemul(a[4], b[2]) ⊻ bytemul(a[3], b[3]) ⊻ bytemul(a[2], b[4])
    d2 = bytemul(a[2], b[1]) ⊻ bytemul(a[1], b[2]) ⊻ bytemul(a[4], b[3]) ⊻ bytemul(a[3], b[4])
    d3 = bytemul(a[3], b[1]) ⊻ bytemul(a[2], b[2]) ⊻ bytemul(a[1], b[3]) ⊻ bytemul(a[4], b[4])
    d4 = bytemul(a[4], b[1]) ⊻ bytemul(a[3], b[2]) ⊻ bytemul(a[2], b[3]) ⊻ bytemul(a[1], b[4])
    return (d1, d2, d3, d4)
end

function cipher(in_b::NTuple{16, UInt8}, w::Vector{UInt8}, Nr)
    @assert length(w) ≥ 16(Nr + 1)
    state = collect(in_b)

    #addroundkey(state, w[1:16])
    state .⊻= w[1:16]

    for round ∈  1:(Nr - 1)
        state .= subbytes.(state)
        shiftrows!(state)
        mixcolumns!(state)
        state .⊻= w[16round + 1:16(round + 1)]
    end

    state .= subbytes.(state)
    shiftrows!(state)
    state .⊻= w[16Nr + 1:16(Nr + 1)]

    return Tuple(state)
end

subbytes(b::UInt8) = 0x63 ⊻ mapfoldl(r -> bitrotate(byteinv(b), r), ⊻, 0:4)

function shiftrows!(state)
    (state[2], state[6], state[10], state[14]) = (state[6], state[10], state[14], state[2])
    (state[3], state[7], state[11], state[15]) = (state[11], state[15], state[3], state[7])
    (state[4], state[8], state[12], state[16]) = (state[16], state[4], state[8], state[12])
end

function mixcolumns!(state)
    words = reinterpret(NTuple{4, UInt8}, state)
    words .= wordmul.(Ref((0x02, 0x01, 0x01, 0x03)), words)
end

function keyexpansion(key::NTuple{N, UInt8}) where {N}
    @assert N ∈ (16, 24, 32) "AES only support (128|192|256)-bit key, got $(8N)"
    Nk = N ÷ 4
    Nr = Nk + 6

    word = Vector{UInt8}(undef, 16(Nr + 1))
    tmp = Vector{UInt8}(undef, 4)

    fill!(word, 0)
    word[1:N] .= collect(key)

    for i ∈ Nk:4(Nr + 1) - 1
        tmp .= word[4i - 3:4i]
        if i % Nk == 0
            (tmp[1], tmp[2], tmp[3], tmp[4]) = (tmp[2], tmp[3], tmp[4], tmp[1])
            tmp .= subbytes.(tmp) .⊻ rcon(i ÷ Nk - 1)
        elseif N == 32 && i % Nk == 4
            tmp .= subbytes.(tmp)
        end
        word[4i + 1:4(i + 1)] .= word[4(i - Nk) + 1: 4(i - Nk + 1)] .⊻ tmp
    end

    return word
end

function rcon(i)
    msB = i < 8 ? 0x1 << i : iseven(i) ? 0x1B : 0x36
    return reinterpret(NTuple{4, UInt8}, UInt32(msB))
end



function AES_CTR(key::NTuple{N, UInt8}, data::Array{UInt8}) where {N}
    round_key = keyexpansion(key)
    Nr = N ÷ 4 + 6

    output = similar(data)

    n = 0
    count = zero(UInt128)
    while n ≠ length(data)
        count += 1
        mask = cipher(reinterpret(NTuple{16, UInt8}, count), round_key, Nr)

        remaind = length(data) - n
        if remaind ≥ 16
            output[n + 1:n + 16] .= data[n + 1:n + 16] .⊻ mask
            n += 16
        else
            output[n + 1:end] .= data[n + 1:end] .⊻ mask[1:remaind]
            n += remaind
        end
    end

    return output
end


mutable struct SHA1State
    h::Vector{UInt32}       # length = 5
    buffer::Vector{UInt8}   # length = 64
    ml::UInt64

    SHA1State() = new([0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0], Vector{UInt8}(undef, 64), 0)
end
SHA1State(data::AbstractArray{UInt8}) = update!(SHA1State(), data)

function update!(state::SHA1State, data::AbstractArray{UInt8})
    buffer_left = state.ml % 64
    push_size = min(64 - buffer_left, length(data))
    state.buffer[buffer_left + 1: buffer_left + push_size] .= data[1:push_size]
    buffer_left + push_size == 64 && update_internal!(state)

    n = push_size
    while n ≠ length(data)
        remaind = length(data) - n
        if remaind ≥ 64
            state.buffer .= data[n + 1:n + 64]
            update_internal!(state)
            n += 64
        else
            state.buffer[1:remaind] .= data[n + 1:end]
            n += remaind
        end
    end

    state.ml += length(data)
    return state
end

function update_internal!(state::SHA1State)
    words = bswap.(reinterpret(UInt32, state.buffer))
    (a, b, c, d, e) = state.h

    for i ∈ 1:80
        i > 16 && (words[mod1(i, 16)] = bitrotate(mapfoldl(j -> words[mod1(i - j, 16)], ⊻, (3, 8, 14, 16)), 1))

        (f, k) = if i ≤ 20
            ((b & c) | (~b & d), 0x5A827999)
        elseif i ≤ 40
            (b ⊻ c ⊻ d, 0x6ED9EBA1)
        elseif i ≤ 60
            ((b & c) | (b & d) | (c & d), 0x8F1BBCDC)
        else
            (b ⊻ c ⊻ d, 0xCA62C1D6)
        end

        f = bitrotate(a, 5) + f + e + k + words[mod1(i, 16)]
        (a, b, c, d, e) = (f, a, bitrotate(b, 30), c, d)
    end

    state.h .+= [a, b, c, d, e]

    return state
end

function final!(state::SHA1State)
    n = state.ml % 64
    state.buffer[n += 1] = 0x80
    if n > 56
        state.buffer[n + 1:end] .= 0
        update_internal!(state)
        n = 0
    end
    state.buffer[n + 1:56] .= 0
    state.buffer[57:end] .= reinterpret(NTuple{8, UInt8}, bswap(8state.ml))
    update_internal!(state)
    return state
end

output(state::SHA1State) = reinterpret(UInt8, bswap.(state.h))


mutable struct HMAC_SHA1_State
    inner::SHA1State
    outer::SHA1State

    function HMAC_SHA1_State(K::AbstractArray{UInt8})
        K_0 = zeros(UInt8, 64)
        if length(K) > 64
            sha1 = final!(SHA1State(K))
            K_0[1:20] .= output(sha1)
        else
            K_0[1:length(K)] .= K
        end

        inner = SHA1State(K_0 .⊻ 0x36)
        outer = SHA1State(K_0 .⊻ 0x5C)
        return new(inner, outer)
    end
end

function update!(state::HMAC_SHA1_State, data::AbstractArray{UInt8})
    update!(state.inner, data)
    return state
end

function final!(state::HMAC_SHA1_State)
    final!(state.inner)
    update!(state.outer, output(state.inner))
    final!(state.outer)
    return state
end

output(state::HMAC_SHA1_State) = output(state.outer)


function PBKDF2_SHA1(p::AbstractArray{UInt8}, salt::AbstractArray{UInt8}, c, dkLen)
    l = cld(dkLen, 20)
    r = dkLen - 20(l - 1)

    DK = Vector{UInt8}(undef, dkLen)
    T = Vector{UInt8}(undef, 20)
    U = Vector{UInt8}(undef, 20)

    for idx ∈ 1:l
        i1 = reinterpret(UInt8, bswap.(UInt32[idx]))
        println(reinterpret(Int32, i1))
        hmac = final!(update!(HMAC_SHA1_State(p), vcat(reshape(salt, :), i1)))
        U .= output(hmac)
        T .= U
        for _ ∈ 2:c
            hmac = final!(update!(HMAC_SHA1_State(p), U))
            U .= output(hmac)
            T .⊻= U
        end

        if idx ≠ l
            DK[20(idx - 1) + 1:20idx] .= T
        else
            DK[20(l - 1) + 1:end] .= T[1:r]
        end
    end

    return DK
end


function magic(password::AbstractArray{UInt8}, data::AbstractArray{UInt8}; AES = 256, salt = rand(UInt8, AES ÷ 16))
    @assert length(salt) == AES ÷ 16

    keys = PBKDF2_SHA1(password, salt, 1000, AES ÷ 4 + 2)
    println(u82hex(keys))
    aes_key = keys[1:AES ÷ 8]
    auth_key = keys[AES ÷ 8 + 1:AES ÷ 4]
    vari_code = keys[AES ÷ 4 + 1:end]

    encrypted = AES_CTR(Tuple(aes_key), data)
    auth = final!(update!(HMAC_SHA1_State(auth_key), encrypted))
    auth_code = output(auth)[1:10]

    return vcat(salt, vari_code, encrypted, auth_code)
end

const magic_number = 0x24f9b7c98b4f68e1


hex2u8(str) = parse.(UInt8, split(str); base = 16)

function u82hex(arr)
    res = IOBuffer()
    write(res, '\n')
    for (idx, u8) ∈ enumerate(arr)
        write(res, bytes2hex(u8), ' ')
        idx % 16 == 0 && idx ≠ length(arr) && write(res, '\n')
    end
    return String(take!(res))
end
