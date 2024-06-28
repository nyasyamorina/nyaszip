// project: https://github.com/nyasyamorina/nyaszip
// made by nyasyamorina

#pragma once

#include <stdint.h>
#include <algorithm>
#include <ctime>
#include <bit>
#include <array>
#include <list>
#include <tuple>
#include <string>
#include <exception>
#include <fstream>
#ifdef NYASZIP_WARN
#include <iostream>
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;


namespace nyaszip
{
    /// @brief write into bytes buffer and step over
    template<typename T> inline void _write_into(u8 * & buffer, T const& value)
    {
        *reinterpret_cast<T *>(buffer) = value;
        buffer += sizeof(T);
    }

    /// @brief the `operator^=` for bytes
    /// @tparam max the number for one operation, must be smaller than 64
    template<u8 length> static inline void xor_to(u8 * dst, u8 const* src)
    {
        static_assert(length < 64, "the number is greater than 63");

        if constexpr (length & 32)
        {
            reinterpret_cast<u64 *>(dst)[0] ^= reinterpret_cast<u64 const*>(src)[0];
            reinterpret_cast<u64 *>(dst)[1] ^= reinterpret_cast<u64 const*>(src)[1];
            reinterpret_cast<u64 *>(dst)[2] ^= reinterpret_cast<u64 const*>(src)[2];
            reinterpret_cast<u64 *>(dst)[3] ^= reinterpret_cast<u64 const*>(src)[3];
            dst += 32; src += 32;
        }
        if constexpr (length & 16)
        {
            reinterpret_cast<u64 *>(dst)[0] ^= reinterpret_cast<u64 const*>(src)[0];
            reinterpret_cast<u64 *>(dst)[1] ^= reinterpret_cast<u64 const*>(src)[1];
            dst += 16; src += 16;
        }
        if constexpr (length & 8)
        {
            *reinterpret_cast<u64 *>(dst) ^= *reinterpret_cast<u64 const*>(src);
            dst += 8; src += 8;
        }
        if constexpr (length & 4)
        {
            *reinterpret_cast<u32 *>(dst) ^= *reinterpret_cast<u32 const*>(src);
            dst += 4; src += 4;
        }
        if constexpr (length & 2)
        {
            *reinterpret_cast<u16 *>(dst) ^= *reinterpret_cast<u16 const*>(src);
            dst += 2; src += 2;
        }
        if constexpr (length & 1)
        {
            *dst ^= *src;
            dst += 1; src += 1;
        }
    }
    /// @brief the `operator^=` for bytes
    /// @tparam max the max number for one operation, must be smaller than 33 and a power of 2
    /// @return the number of this operation, `length` also subtract.
    template<u8 max> static inline u8 xor_to(u8 * dst, u8 const* src, u64 & length)
    {
        static_assert(::std::has_single_bit(max), "the max number is not a power of 2");
        static_assert(max < 64, "the max number is greater than 32");

        constexpr u8 bw = ::std::bit_width(max) - 1;
        if (length >> bw)
        {
            xor_to<max>(dst, src);
            dst += max; src += max;
            length -= max;
            return max;
        }
        else
        {
            if constexpr (max > 16) if (length & 16)
            {
                reinterpret_cast<u64 *>(dst)[0] ^= reinterpret_cast<u64 const*>(src)[0];
                reinterpret_cast<u64 *>(dst)[1] ^= reinterpret_cast<u64 const*>(src)[1];
                dst += 16; src += 16;
            }
            if constexpr (max > 8)  if (length & 8)
            {
                *reinterpret_cast<u64 *>(dst) ^= *reinterpret_cast<u64 const*>(src);
                dst += 8; src += 8;
            }
            if constexpr (max > 4)  if (length & 4)
            {
                *reinterpret_cast<u32 *>(dst) ^= *reinterpret_cast<u32 const*>(src);
                dst += 4; src += 4;
            }
            if constexpr (max > 2)  if (length & 2)
            {
                *reinterpret_cast<u16 *>(dst) ^= *reinterpret_cast<u16 const*>(src);
                dst += 2; src += 2;
            }
            if constexpr (max > 1)  if (length & 1)
            {
                *dst ^= *src;
                dst += 1; src += 1;
            }
            return static_cast<u8>(::std::exchange(length, 0));
        }
    }

    static constexpr inline ::std::tuple<u8, u8, u8, u8> u32_to_u8s(u32 x) noexcept
    {
        u8 x0 = x & 0xFF, x1 = (x >> 8) & 0xFF, x2 = (x >> 16) & 0xFF, x3 = (x >> 24) & 0xFF;
        return {x0, x1, x2, x3};
    }
    static constexpr inline u32 u8s_to_u32(u8 x0, u8 x1, u8 x2, u8 x3) noexcept
    {
        return static_cast<u32>(x0) | (static_cast<u32>(x1) << 8) | (static_cast<u32>(x2) << 16) | (static_cast<u32>(x3) << 24);
    }

    /// @brief store file modifird time
    struct MsDosTime
    {
    public:
        /// @brief (year - 1980, 7 bits)(month [1,12], 4 bits)(day [1,31], 5 bits)
        u16 date;
        /// @brief (hour [0,23], 5 bits)(minite [0,59], 6 bits)(second / 2 [0,30] 4 bits)
        u16 time;

        MsDosTime()
        : date(0b10001), time(0) {}
        MsDosTime(i64 time)
        {
            tm t;
            localtime_s(&t, &time);
            set(t);
        }
        MsDosTime(tm const& t)
        {
            set(t);
        }

        void set(tm const& t)
        {
            date = ((u16)(t.tm_year - 80) << 9) | ((u16)(t.tm_mon + 1) << 5) | ((u16)(t.tm_mday));
            time = ((u16)(t.tm_hour) << 11) | ((u16)(t.tm_min) << 5) | ((u16)(t.tm_sec) >> 1);
        }
    };

    struct PCG_XSH_RR
    {
        u64 state;

        // see more: https://en.wikipedia.org/wiki/Permuted_congruential_generator
        // and https://www.pcg-random.org
    public:
        static constexpr u64 MUL = 0x5851F42D4C957F2D;
        static constexpr u64 INC = 0x14057B7EF767814F;

        PCG_XSH_RR() noexcept
        {
            u64 s = 0x24F9B7C98B4F68E1;
            s ^= ::std::time(nullptr);
            s ^= reinterpret_cast<u64>(this);
            seed(s);
        }
        PCG_XSH_RR(u64 s) noexcept
        {
            seed(s);
        }

        void seed(u64 s) noexcept
        {
            state = s + INC;
            get();
        }
        u32 get() noexcept {
            u64 x = state;
            state = x * MUL + INC;

            u8 count = x >> 59;
            x ^= x >> 18;
            return ::std::rotr(static_cast<u32>(x >> 27), count);
        }
        void gen(u8 * out, u64 length)
        {
            while (length != 0)
            {
                u32 x = get();
                if (length >= sizeof(u32)) {
                    _write_into<u32>(out, x);
                    length -= sizeof(u32);
                }
                else {
                    if (length & sizeof(u16)) {
                        _write_into<u16>(out, static_cast<u16>(x));
                        x >>= 16;
                    }
                    if (length & sizeof(u8)) {
                        *out = static_cast<u8>(x);
                    }
                    length = 0;
                }
            }
        }
    };

    [[maybe_unused]]    // keep this method to show how crc-32 works
    static u32 _crc32(u32 crc /* set to 0 first */, u8 const* data, u64 length)
    {
        u64 buff[1];
        // pre-conditioning
        *buff = static_cast<u64>(~crc);

        while (length > 0)
        {
            // copy data into `buff` and do xor operation
            u8 n = xor_to<sizeof(u64)>(reinterpret_cast<u8 *>(buff), data, length);
            data += n;
            u64 flag = n == sizeof(u64) ? 0 /* ? 1ULL << 64 = 1ULL ? */ : (static_cast<u64>(1) << (n << 3));
            flag -= 1;

            // calcute the remainder polynomial in GF(2) finite field with
            // divisor `x³² + x²⁶ + x²³ + x²² + x¹⁶ + x¹² + x¹¹ + x¹⁰ + x⁸ + x⁷ + x⁵ + x⁴ + x² + x¹ + x⁰`.
            // note that the polynomial order is different from that of `GF2poly`,
            // polynomial `b₇x⁷ + b₆x⁶ + b₅x⁵ + b₄x⁴ + b₃x³ + b₂x² + b₁x¹ + b₀x⁰` is
            // stored as `b₇2⁰ + b₆2¹ + b₅2² + b₄2³ + b₃2⁴ + b₂2⁵ + b₁2⁶ + b₀2⁷` in a byte,
            // but `B₃x²⁴ + B₂x¹⁶ + B₁x⁸ + B₀x⁰` where `B₃,B₂,B₁,B₀ < x⁸` is
            // stored as `B₀2⁰ + B₁2⁸ + B₂2¹⁶ + B₃2²⁴` (big endian) in a word (4 bytes).
            constexpr u64 divisor = (static_cast<u64>(0xEDB88320) << 1) | 1;
            while (flag)
            {
                if (*buff & 1) { *buff ^= divisor; }
                *buff >>= 1;
                flag >>= 1;
            }
        }
        // post-conditioning
        return ~static_cast<u32>(*buff);
    }

    static constexpr u32 crc32(u8 value)
    {
        u64 buff = value;
        constexpr u64 divisor = (static_cast<u64>(0xEDB88320) << 1) | 1;
        for (u8 flag = 0xFF; flag; flag >>= 1)
        {
            if (buff & 1) { buff ^= divisor; }
            buff >>= 1;
        }
        return buff;
    }
    static constexpr ::std::array<u32, 256> _gen_crc32_table()
    {
        ::std::array<u32, 256> res;
        u8 value = 0;
        ::std::for_each(res.begin(), res.end(), [&value](u32 & crc) { crc = crc32(value++); });
        return res;
    }
    static auto const crc32_table = _gen_crc32_table();
    static inline u32 crc32(u32 crc /* set to 0 first */, u8 const* data, u64 length)
    {
        u8 const* const end = data + length;
        u32 tmp = ~crc; // pre-conditioning
        for (; data < end; data++)
        {
            tmp ^= *data;
            tmp = (tmp >> 8) ^ crc32_table[tmp & 0xFF];
        }
        return ~tmp;    // post-conditioning
    }

    template<typename T> class GF2poly
    {
        // polynomial calculation in GF(2) finite field.
        // using binary number `...+ b₂2² + b₁2¹ + b₀2⁰` to represent polynomial `... + b₂x² + b₁x¹ + b₀x⁰`.
        // addition and subtraction in GF(2) finite field are equal to exclusive or operation.
    public:
        static constexpr T mul(T x, T y) noexcept
        {
            T res = 0;
            while (x != 0 && y != 0)
            {
                if (y & 1)
                {
                    res ^= x;
                }
                x <<= 1;
                y >>= 1;
            }
            return res;
        }
        static constexpr ::std::tuple<T, T> divrem(T x, T y) noexcept
        {
            T res = 0;
            auto bw_diff = ::std::bit_width(x) - ::std::bit_width(y);
            while (bw_diff >= 0)
            {
                res |= static_cast<T>(1) << bw_diff;
                x ^= y << bw_diff;
                bw_diff = ::std::bit_width(x) - ::std::bit_width(y);
            }
            return {res, x};
        }
        static constexpr T modmul(T x, T y, T r /* the divisor */) noexcept
        {
            T res = 0;
            x = ::std::get<1>(GF2poly::divrem(x, r));
            y = ::std::get<1>(GF2poly::divrem(y, r));
            while (x != 0 && y != 0)
            {
                if (y & 1)
                {
                    res ^= x;
                }
                x <<= 1;
                y >>= 1;
                if (::std::bit_width(x) == ::std::bit_width(r))
                {
                    x ^= r;
                }
            }
            return res;
        }
        static constexpr T invmodmul(T x, T r /* the divisor */) noexcept {
            if (x == 0) {
                return x;
            }
            T r0 = r, r1 = x;
            T s0 = 0, s1 = 1;
            while (r1 != 0)
            {
                auto [d, tmp] = GF2poly::divrem(r0, r1);
                r0 = ::std::exchange(r1, tmp);
                s0 = ::std::exchange(s1, s0 ^ GF2poly::mul(s1, d));
            }
            return s0;
        }
    };

    class AES_basic  // store algorithms used in AES
    {
    public:
        static constexpr u16 GF2_POLY_DIVISOR = 0x011B;


        static constexpr inline u8 rcon(u8 i) noexcept
        {
            // some mysterious constants called `round constants` defined in AES standard.
            if (i < 8) {
                return static_cast<u8>(1) << i;
            }
            return i & 1 ? 0x36 : 0x1B;
        }

        static constexpr inline u8 byte_mul(u8 x, u8 y) noexcept
        {
            return static_cast<u8>(GF2poly<u16>::modmul(static_cast<u16>(x), static_cast<u16>(y), GF2_POLY_DIVISOR));
        }
        static constexpr inline u8 byte_inv(u8 x) noexcept
        {
            return static_cast<u8>(GF2poly<u16>::invmodmul(static_cast<u16>(x), GF2_POLY_DIVISOR));
        }
        [[maybe_unused]]
        static constexpr u32 _word_mul(u32 X, u32 Y) noexcept
        {
            // both `X` and `Y` are two-variable polynomial `B₃y³ + B₂y² + B₁y¹ + B₀y⁰`
            // that be stored as `B₀2⁰ + B₁2⁸ + B₂2¹⁶ + B₃2²⁴` in a word,
            // where `B₃,B₂,B₁,B₀ < x⁸` are polynomials about `x` defined in `GF2poly`.
            // this calculation is calculating modular multiplication `XY mod (y⁴ + y⁰)(GF2_POLY_DIVISOR)`
            auto [x0, x1, x2, x3] = u32_to_u8s(X); auto [y0, y1, y2, y3] = u32_to_u8s(Y);
            u8 r0 = byte_mul(x0, y0) ^ byte_mul(x3, y1) ^ byte_mul(x2, y2) ^ byte_mul(x1, y3);
            u8 r1 = byte_mul(x1, y0) ^ byte_mul(x0, y1) ^ byte_mul(x3, y2) ^ byte_mul(x2, y3);
            u8 r2 = byte_mul(x2, y0) ^ byte_mul(x1, y1) ^ byte_mul(x0, y2) ^ byte_mul(x3, y3);
            u8 r3 = byte_mul(x3, y0) ^ byte_mul(x2, y1) ^ byte_mul(x1, y2) ^ byte_mul(x0, y3);
            return u8s_to_u32(r0, r1, r2, r3);
        }

        [[maybe_unused]]    // keep this method to show how sub_byte works
        static constexpr inline u8 _sub_byte(u8 x) noexcept
        {
            u8 inv = byte_inv(x);
            // linear transform
            u8 trans = inv ^ ::std::rotl(inv, 1) ^ ::std::rotl(inv, 2) ^ ::std::rotl(inv, 3) ^ ::std::rotl(inv, 4);
            // add a constant
            return trans ^ 0x63;
        }

        static constexpr ::std::array<u32, 256> _gen_word_mul_table() noexcept
        {
            auto [x0, x1, x2, x3] = u32_to_u8s(0x03010102);

            ::std::array<u32, 256> res;
            u8 y = 0;
            ::std::for_each(res.begin(), res.end(), [x0, x1, x2, x3, &y](u32 & word) {
                u8 r0 = byte_mul(x0, y);
                u8 r1 = byte_mul(x1, y);
                u8 r2 = byte_mul(x2, y);
                u8 r3 = byte_mul(x3, y);
                y++;
                word = u8s_to_u32(r0, r1, r2, r3);
            });
            return res;
        }
        static ::std::array<u32, 256> const ByteMul0x03010102;
        static inline u32 word_mul_0x03010102(u32 y) noexcept
        {
            auto [y0, y1, y2, y3] = u32_to_u8s(y);
            u32 r0 =             ByteMul0x03010102[y0];
            u32 r1 = ::std::rotl(ByteMul0x03010102[y1],  8);
            u32 r2 = ::std::rotl(ByteMul0x03010102[y2], 16);
            u32 r3 = ::std::rotl(ByteMul0x03010102[y3], 24);
            return r0 ^ r1 ^ r2 ^ r3;
        }

        static constexpr ::std::array<u8, 256> _gen_sbox() noexcept
        {
            ::std::array<u8, 256> res;
            u8 value = 0;
            ::std::for_each(res.begin(), res.end(), [&value](u8 & sb) { sb = _sub_byte(value++); });
            return res;
        }
        static ::std::array<u8, 256> const SBox;
        static inline u8 sub_byte(u8 x) noexcept
        {
            return SBox[x];
        }

        static constexpr inline u32 sub_bytes(u32 x) noexcept
        {
            auto [x0, x1, x2, x3] = u32_to_u8s(x);
            u8 r0 = sub_byte(x0), r1 = sub_byte(x1), r2 = sub_byte(x2), r3 = sub_byte(x3);
            return u8s_to_u32(r0, r1, r2, r3);
        }
        static inline void sub_bytes(u8 * state)
        {
            auto words = reinterpret_cast<u32 *>(state);
            words[0] = sub_bytes(words[0]);
            words[1] = sub_bytes(words[1]);
            words[2] = sub_bytes(words[2]);
            words[3] = sub_bytes(words[3]);
        }

        static inline void shift_rows(u8 * state)
        {
            using ::std::exchange;
            // (1,0 | 1,1 | 1,2 | 1,3) => (1,1 | 1,2 | 1,3 | 1,0)
            state[1] = exchange(state[5], exchange(state[9], exchange(state[13], state[1])));
            // (2,0 | 2,1 | 2,2 | 2,3) => (2,2 | 2,3 | 2,0 | 2,1)
            ::std::swap(state[2], state[10]); ::std::swap(state[6], state[14]);
            // (3,0 | 3,1 | 3,2 | 3,3) => (3,3 | 3,0 | 3,1 | 3,2)
            state[3] = exchange(state[15], exchange(state[11], exchange(state[7], state[3])));
        }
        static inline void mix_cols(u8 * state)
        {
            auto words = reinterpret_cast<u32 *>(state);
            words[0] = word_mul_0x03010102(words[0]);
            words[1] = word_mul_0x03010102(words[1]);
            words[2] = word_mul_0x03010102(words[2]);
            words[3] = word_mul_0x03010102(words[3]);
        }
        static inline void add_round_key(u8 * state, u8 const* rkey)
        {
            xor_to<16>(state, rkey);
        }
    };
    auto const AES_basic::ByteMul0x03010102 = AES_basic::_gen_word_mul_table();
    auto const AES_basic::SBox = AES_basic::_gen_sbox();

    /// @brief the AES encrption
    /// @tparam bits only can be 128, 192 or 256.
    template<u16 bits> class AES : public AES_basic
    {
        static_assert(bits == 128 || bits == 192 || bits == 256,
        "got an unsupported AES mode");

    public:
        static constexpr u8 KEY_LENGTH = bits / 8;
        static constexpr u8 Nk = KEY_LENGTH / 4;
        static constexpr u8 Nr = Nk + 6;
        static constexpr u8 ROUND_KEY_LENGTH = 16 * (Nr + 1);
        static constexpr u8 STATE_LENGTH = 16;
        static constexpr u8 BLOCK_LENGTH = STATE_LENGTH;

    protected:
        u8 _round_key[ROUND_KEY_LENGTH];

    public:
        AES() noexcept {}
        AES(u8 const* key)
        {
            set_key(key);
        }

        AES & swap(AES & other) noexcept
        {
            if (this != ::std::addressof(other))
            {
                u8 tmp[ROUND_KEY_LENGTH];
                ::std::memcpy(tmp, _round_key, ROUND_KEY_LENGTH);
                ::std::memcpy(_round_key, other._round_key, ROUND_KEY_LENGTH);
                ::std::memcpy(other._round_key, tmp, ROUND_KEY_LENGTH);
            }
            return *this;
        }

        static constexpr u64 key_length() noexcept
        {
            return KEY_LENGTH;
        }
        static constexpr u64 block_length() noexcept
        {
            return BLOCK_LENGTH;
        }

        AES & set_key(u8 const* key)
        {
            ::std::memcpy(_round_key, key, KEY_LENGTH);
            auto words = reinterpret_cast<u32 *>(_round_key) + Nk;

            for (u8 i = Nk; i < ROUND_KEY_LENGTH / 4; i++)
            {
                u32 word0 = *(words - Nk);
                u32 word1 = *(words - 1);

                u8 d = i / Nk, r = i % Nk;
                if (r == 0)
                {
                    word1 = sub_bytes(::std::rotr(word1, 8)) ^ static_cast<u32>(rcon(d - 1));
                }
                else if constexpr (bits == 256) if (r == 4)
                {
                    word1 = sub_bytes(word1);
                }
                *(words++) = word0 ^ word1;
            }

            return *this;
        }

        AES const& encrypt(u8 * state) const
        {
            auto rkey = _round_key;
            add_round_key(state, rkey);
            rkey += 16;

            for (u8 round = 1; round < Nr; round++)
            {
                sub_bytes(state);
                shift_rows(state);
                mix_cols(state);
                add_round_key(state, rkey);
                rkey += 16;
            }

            sub_bytes(state);
            shift_rows(state);
            add_round_key(state, rkey);
            return *this;
        }
    };

    /// @brief the CTR (counter) mode of block cipher
    /// @tparam blockCipher cipher that only process fixed-size block
    /// @tparam nonceLength length of constant nonce before counter
    template<class blockCipher, u64 nonceLength> class CTR
    {
        static_assert(nonceLength < blockCipher::BLOCK_LENGTH,
        "need to leave at least 1 byte for the counter");

    public:
        static constexpr u64 BLOCK_LENGTH = blockCipher::block_length();
        static constexpr u64 NONCE_LENGTH = nonceLength;
        static constexpr u64 COUNTER_LENGTH = BLOCK_LENGTH - NONCE_LENGTH;

    protected:
        blockCipher _cipher;
        u8 _block[BLOCK_LENGTH];
        u8 _mask[BLOCK_LENGTH];
        u64 _remaining_masks;

        u8 * _counter_start()
        {
            return _block;
        }
        u8 * _counter_end()
        {
            return _counter_start() + COUNTER_LENGTH;
        }
        u8 * _nonce_start()
        {
            return _counter_end();
        }
        u8 * _nonce_end()
        {
            return _nonce_start() + NONCE_LENGTH;
        }

        void _count()
        {
            for (u8 * ptr = _counter_start(); ptr < _counter_end(); ptr++)
            {
                *ptr += 1;
                if (*ptr != 0) { break; }
            }
            // generate mask
            ::std::memcpy(_mask, _block, BLOCK_LENGTH);
            _cipher.encrypt(_mask);
            _remaining_masks = BLOCK_LENGTH;
        }

    public:
        CTR()
        : _cipher() {
            reset();
        }
        CTR(u8 const* key)
        : _cipher(key) {
            reset();
        }
        CTR(u8 const* key, u8 const* nonce)
        : _cipher(key) {
            reset();
            set_nonce(nonce);
        }

        CTR & swap(CTR & other) noexcept
        {
            using ::std::memcpy;

            if (this != ::std::addressof(other))
            {
                _cipher.swap(other._cipher);
                u8 tmp[BLOCK_LENGTH];
                memcpy(tmp, _block, BLOCK_LENGTH);
                memcpy(_block, other._block, BLOCK_LENGTH);
                memcpy(other._block, tmp, BLOCK_LENGTH);
                memcpy(tmp, _mask, BLOCK_LENGTH);
                memcpy(_mask, other._mask, BLOCK_LENGTH);
                memcpy(other._mask, tmp, BLOCK_LENGTH);
                ::std::swap(_remaining_masks, other._remaining_masks);
            }
            return *this;
        }

        static constexpr u64 counter_length() noexcept
        {
            return COUNTER_LENGTH;
        }
        static constexpr u64 nonce_length() noexcept
        {
            return NONCE_LENGTH;
        }
        static constexpr u64 key_length() noexcept
        {
            return blockCipher::key_length();
        }

        /// @brief reset the counter
        CTR & reset()
        {
            ::std::fill(_counter_start(), _counter_end(), 0);
            _remaining_masks = 0;
            return *this;
        }
        /// @param nonce the length of nonce must be nonce_length()
        CTR & set_nonce(u8 const* nonce)
        {
            ::std::memcpy(_nonce_start(), nonce, NONCE_LENGTH);
            return *this;
        }
        CTR & set_key(u8 const* key)
        {
            _cipher.set_key(key);
            return *this;
        }

        // the CTR mode is symmetry for both encryption and decryption

        CTR & apply(u8 * data, u64 length)
        {
            u8 consumed;
            if (_remaining_masks != 0)
            {
                // use up all remaining masks first (or not)
                auto use = ::std::min(_remaining_masks, length);
                auto mask_start = _mask + (BLOCK_LENGTH - _remaining_masks);
                consumed = xor_to<BLOCK_LENGTH>(data, mask_start, use);
                _remaining_masks -= consumed;
                data += consumed; length -= consumed;

                if (_remaining_masks) { return *this; }   // length < _remaining_masks
            } // _remaining_masks == 0

            while (length != 0)
            {
                if (_remaining_masks == 0)
                {
                    _count();
                }
                consumed = xor_to<BLOCK_LENGTH>(data, _mask, length);
                _remaining_masks -= consumed;
                data += consumed;
            }
            return *this;
        }
    };

    namespace Hash
    {
        struct SHA1
        {
        public:
            static constexpr u8 BLOCK_LENGTH = 64;
            static constexpr u8 OUTPUT_LENGTH = 20;

        protected:
            u32 _h[OUTPUT_LENGTH / sizeof(u32)];
            u8 _buff[BLOCK_LENGTH];
            u64 _byte_count;

            void _update_buff()
            {
                _bswap_buff();
                auto words = reinterpret_cast<u32 *>(_buff);
                auto a = _h[0], b = _h[1], c = _h[2], d = _h[3], e = _h[4];

                for (u8 i = 0; i < 80; i++)
                {
                    if (i > 15)
                    {
                        words[i & 15] = ::std::rotl(words[(i - 3) & 15] ^ words[(i - 8) & 15] ^ words[(i - 14) & 15] ^ words[(i - 16) & 15], 1);
                    }

                    u32 f, k;
                    if (i < 20)
                    {
                        f = (b & c) | (~b & d);
                        k = 0x5A827999;
                    }
                    else if (i < 40)
                    {
                        f = b ^ c ^ d;
                        k = 0x6ED9EBA1;
                    }
                    else if (i < 60)
                    {
                        f = (b & c) | (b & d) | (c & d);
                        k = 0x8F1BBCDC;
                    }
                    else
                    {
                        f = b ^ c ^ d;
                        k = 0xCA62C1D6;
                    }

                    f += ::std::rotl(a, 5) + e + k + words[i & 15];
                    e = d;
                    d = c;
                    c = ::std::rotl(b, 30);
                    b = a;
                    a = f;
                }
                _h[0] += a;
                _h[1] += b;
                _h[2] += c;
                _h[3] += d;
                _h[4] += e;
            }
            void _bswap_buff()
            {
                auto end_ptr = reinterpret_cast<u32 *>(_buff + BLOCK_LENGTH);
                auto ptr = reinterpret_cast<u32 *>(_buff);
                ::std::for_each(ptr, end_ptr, [](u32 & word){ word = ::std::byteswap(word); });
            }
            void _bswap_h()
            {
                _h[0] = ::std::byteswap(_h[0]);
                _h[1] = ::std::byteswap(_h[1]);
                _h[2] = ::std::byteswap(_h[2]);
                _h[3] = ::std::byteswap(_h[3]);
                _h[4] = ::std::byteswap(_h[4]);
            }

        public:
            SHA1()
            {
                reset();
            }

            SHA1 & swap(SHA1 & other) noexcept
            {
                using ::std::memcpy;

                if (this != ::std::addressof(other))
                {
                    u8 tmp[::std::max(BLOCK_LENGTH, OUTPUT_LENGTH)];
                    memcpy(tmp, _h, OUTPUT_LENGTH);
                    memcpy(_h, other._h, OUTPUT_LENGTH);
                    memcpy(other._h, tmp, OUTPUT_LENGTH);
                    memcpy(tmp, _buff, BLOCK_LENGTH);
                    memcpy(_buff, other._buff, BLOCK_LENGTH);
                    memcpy(other._buff, tmp, BLOCK_LENGTH);
                    ::std::swap(_byte_count, other._byte_count);
                }
                return *this;
            }

            static constexpr u64 block_length() noexcept
            {
                return BLOCK_LENGTH;
            }
            static constexpr u64 output_length() noexcept
            {
                return OUTPUT_LENGTH;
            }

            SHA1 & reset() noexcept
            {
                _h[0] = 0x67452301;
                _h[1] = 0xEFCDAB89;
                _h[2] = 0x98BADCFE;
                _h[3] = 0x10325476;
                _h[4] = 0xC3D2E1F0;
                _byte_count = 0;
                return *this;
            }

            SHA1 & update(u8 const* data, u64 length)
            {
                auto buff_left = _byte_count & 63;
                auto push_length = ::std::min(BLOCK_LENGTH - buff_left, length);
                ::std::memcpy(_buff + buff_left, data, push_length);
                _byte_count += push_length; data += push_length; length -= push_length;
                if (buff_left + push_length < BLOCK_LENGTH) { return *this; }
                _update_buff();

                while (length != 0)
                {
                    if (length >= BLOCK_LENGTH)
                    {
                        ::std::memcpy(_buff, data, BLOCK_LENGTH);
                        _byte_count += BLOCK_LENGTH; data += BLOCK_LENGTH; length -= BLOCK_LENGTH;
                        _update_buff();
                    }
                    else
                    {
                        ::std::memcpy(_buff, data, length);
                        _byte_count += length; length = 0;
                    }
                }
                return *this;
            }
            SHA1 & finalize()
            {
                auto buff_left = _byte_count & 63;
                _buff[buff_left++] = 0x80;
                if (buff_left > 56)
                {
                    ::std::fill(_buff + buff_left, _buff + BLOCK_LENGTH, 0);
                    _update_buff();
                    buff_left = 0;
                }

                ::std::fill(_buff + buff_left, _buff + 56, 0);
                *reinterpret_cast<u64 *>(_buff + 56) = ::std::byteswap(_byte_count * 8);
                _update_buff();

                _bswap_h();
                return *this;
            }

            u8 const* output() const noexcept
            {
                return reinterpret_cast<u8 const*>(_h);
            }
            void output(u8 * out) const
            {
                ::std::memcpy(out, output(), OUTPUT_LENGTH);
            }
            void output(u8 * out, u8 max) const
            {
                if (max > OUTPUT_LENGTH)
                {
                    max = OUTPUT_LENGTH;
                }
                ::std::memcpy(out, output(), max);
            }
        };

        /// @brief keyed-hash message authentication
        /// @tparam H hash function (class)
        template<class H> class HMAC
        {
        public:
            static constexpr u64 BLOCK_LENGTH = H::block_length();

        protected:
            H _h0, _h1;

        public:
            HMAC()
            : _h0(), _h1() {}
            HMAC(u8 const* key, u64 length)
            {
                reset(key, length);
            }

            HMAC & swap(HMAC & other) noexcept
            {
                if (this != ::std::addressof(other))
                {
                    _h0.swap(other._h0);
                    _h1.swap(other._h1);
                }
                return *this;
            }

            static constexpr u64 output_length() noexcept
            {
                return H::output_length();
            }

            HMAC & reset()
            {
                _h0.reset();
                _h1.reset();
                return *this;
            }
            HMAC & reset(u8 const* key, u64 length)
            {
                reset();
                u8 key0[BLOCK_LENGTH] = {0};
                if (length > BLOCK_LENGTH)
                {
                    H().update(key, length).finalize().output(key0);
                }
                else
                {
                    ::std::memcpy(key0, key, length);
                }

                ::std::for_each(key0, key0 + BLOCK_LENGTH, [](u8 & B){ B ^= 0x36; });
                _h0.update(key0, BLOCK_LENGTH);
                ::std::for_each(key0, key0 + BLOCK_LENGTH, [](u8 & B){ B ^= 0x36 ^ 0x5C; });
                _h1.update(key0, BLOCK_LENGTH);

                return *this;
            }

            HMAC & update(u8 const* data, u64 length)
            {
                _h0.update(data, length);
                return *this;
            }
            HMAC & finalize()
            {
                _h1.update(_h0.finalize().output(), output_length()).finalize();
                return *this;
            }

            u8 const* output() const noexcept
            {
                return _h1.output();
            }
            void output(u8 * out) const
            {
                _h1.output(out);
            }
            void output(u8 * out, u8 max) const
            {
                _h1.output(out, max);
            }
        };

        /// @tparam PRF underlying pseudo-random function (class)
        /// @param dk derived key output
        /// @param dk_len length of derived key
        /// @param p password input
        /// @param p_len length of password
        /// @param s salt input
        /// @param s_len length of salf
        /// @param c iteration count
        template<class PRF> void PBKDF2(u8 * dk, u64 dk_len, u8 const* p, u64 p_len, u8 const* s, u64 s_len, u64 c)
        {
            constexpr u64 h_len = PRF::output_length();
            auto l = (dk_len - 1) / h_len + 1;
            auto r = dk_len - h_len * (l - 1);

            u8 u[h_len];
            u32 i[1] = {0};
            auto i_ptr = reinterpret_cast<u8 *>(i);
            auto const end_ptr = dk + dk_len;

            while (dk < end_ptr)
            {
                // *i += 1 in big endian
                *i = ::std::byteswap(*i) + 1;
                u64 len = *i < l ? h_len : r;
                *i = ::std::byteswap(*i);

                PRF(p, p_len).update(s, s_len).update(i_ptr, sizeof(u32)).finalize().output(u);
                ::std::memcpy(dk, u, len);

                for (u64 k = 1; k < c; k++)
                {
                    u64 xor_len = len;
                    PRF(p, p_len).update(u, h_len).finalize().output(u);
                    xor_to<::std::bit_ceil(h_len)>(dk, u, xor_len);
                }
                dk += len;
            }
        }
    } // namespace Hash

    /// @brief the interface of ZipAES template class
    class AbstractZipAES
    {
    public:
        static constexpr u8 VARI_CODE_LENGTH = 2;
        static constexpr u8 AUTH_CODE_LENGTH = 10;

    protected:
        Hash::HMAC<Hash::SHA1> _auth;
        u8 _vari_code[VARI_CODE_LENGTH];

    public:
        AbstractZipAES()
        : _auth() {}

        virtual ~AbstractZipAES()
        {}

        AbstractZipAES & swap(AbstractZipAES & other)
        {
            if (this != ::std::addressof(other))
            {
                _auth.swap(other._auth);
                u8 tmp[VARI_CODE_LENGTH];
                ::std::memcpy(tmp, _vari_code, VARI_CODE_LENGTH);
                ::std::memcpy(_vari_code, other._vari_code, VARI_CODE_LENGTH);
                ::std::memcpy(other._vari_code, tmp, VARI_CODE_LENGTH);
            }
            return *this;
        }

        virtual constexpr u16 bits() const noexcept = 0;

        static constexpr u64 vari_code_length() noexcept
        {
            return VARI_CODE_LENGTH;
        }
        static constexpr u64 auth_code_length() noexcept
        {
            return AUTH_CODE_LENGTH;
        }
        u8 const* vari_code() const noexcept
        {
            return _vari_code;
        }
        u8 const* auth_code() const noexcept
        {
            return _auth.output();
        }

        virtual constexpr u64 salt_length() const noexcept = 0;
        /// @brief generate salt directly in an existing array from outside
        virtual u8 * salt() noexcept = 0;
        virtual u8 const* salt() const noexcept = 0;
        /// @brief set salt, call `set(password, length)` later to regenerate keys
        virtual AbstractZipAES & salt(u8 const*) = 0;

        /// @brief set password and generate keys, set salt before calling this
        virtual AbstractZipAES & set(u8 const* password, u64 length) = 0;

        /// @brief encrypt or decrypt data
        virtual AbstractZipAES & apply(u8 * data, u64 length) = 0;

        virtual AbstractZipAES & finalize()
        {
            _auth.finalize();
            return *this;
        }
    };

    /// @brief the actual AES method used in zip encryption
    template<u16 bits_> class ZipAES : public AbstractZipAES
    {
    public:
        static constexpr u64 KEY_LENGTH = AES<bits_>::key_length();
        static constexpr u64 SALT_LENGTH = KEY_LENGTH / 2;

    protected:
        CTR<AES<bits_>, 0> _ctr;
        u8 _salt[SALT_LENGTH];

    public:
        ZipAES()
        : AbstractZipAES(), _ctr() {}
        ZipAES(u8 const* password, u64 pswd_length, u8 const* salt_)
        : AbstractZipAES(), _ctr() {
            salt(salt_).set(password, pswd_length);
        }

        ZipAES & swap(ZipAES & other) noexcept
        {
            if (this != ::std::addressof(other))
            {
                AbstractZipAES::swap(other);
                _ctr.swap(other._ctr);
                u8 tmp[SALT_LENGTH];
                ::std::memcpy(tmp, _salt, SALT_LENGTH);
                ::std::memcpy(_salt, other._salt, SALT_LENGTH);
                ::std::memcpy(other._salt, tmp, SALT_LENGTH);
            }
            return *this;
        }

        virtual constexpr u16 bits() const noexcept override
        {
            return bits_;
        }

        virtual constexpr u64 salt_length() const noexcept override
        {
            return SALT_LENGTH;
        }
        virtual u8 const* salt() const noexcept override
        {
            return _salt;
        }
        /// @brief generate salt directly in an existing array from outside
        virtual u8 * salt() noexcept override
        {
            return _salt;
        }
        /// @brief set salt, call `set(password, pswd_length)` later to regenerate keys
        virtual ZipAES & salt(u8 const* salt_) override
        {
            ::std::memcpy(_salt, salt_, SALT_LENGTH);
            return *this;
        }

        /// @brief set password and generate keys, set salt before calling this
        virtual ZipAES & set(u8 const* password, u64 length) override
        {
            using namespace Hash;

            constexpr u64 keys_length = KEY_LENGTH * 2 + VARI_CODE_LENGTH;
            u8 keys[keys_length];
            PBKDF2<HMAC<SHA1>>(keys, keys_length, password, length, _salt, SALT_LENGTH, 1000);

            u8 * aes_key = keys + 0;
            u8 * auth_key = keys + KEY_LENGTH;
            ::std::memcpy(_vari_code, keys + KEY_LENGTH * 2, VARI_CODE_LENGTH);

            _ctr.reset().set_key(aes_key);
            _auth.reset(auth_key, KEY_LENGTH);

            return *this;
        }

        virtual ZipAES & apply(u8 * data, u64 length) override
        {
            _ctr.apply(data, length);
            _auth.update(data, length);
            return *this;
        }
    };


    class AbstractCompression
    {
    public:
        static constexpr u64 BUFFER_LENGTH = 4 * 1024;

    protected:
        u8 _buffer[BUFFER_LENGTH];

    public:
        AbstractCompression()
        {}

        virtual ~AbstractCompression()
        {}

        u8 * buffer() noexcept
        {
            return _buffer;
        }
        u8 const* buffer() const noexcept
        {
            return _buffer;
        }
        static constexpr u64 buffer_length() noexcept
        {
            return BUFFER_LENGTH;
        }

        /// @brief compression method number
        virtual u8 method() const noexcept = 0;
        /// @brief the version of zip need to extract
        virtual u16 version() const noexcept = 0;

        /// @brief compress data into buffer
        /// @return a tuple of (the length of compressed data, compressed length in buffer)
        virtual ::std::tuple<u64, u64> compress(u8 const* data, u64 length) = 0;
    };

    class FileAttributes
    {
        /* from "https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants" */
    public:
        static constexpr u32 Readonly           = 0x00000001;
        static constexpr u32 Hidden             = 0x00000002;
        static constexpr u32 System             = 0x00000004;
        static constexpr u32 VolumeLable        = 0x00000008;
        static constexpr u32 Directory          = 0x00000010;
        static constexpr u32 Archive            = 0x00000020;
        static constexpr u32 Device             = 0x00000040;
        static constexpr u32 Normal             = 0x00000080;
        static constexpr u32 Temporary          = 0x00000100;
        static constexpr u32 Sparse             = 0x00000200;
        static constexpr u32 ReparsePoint       = 0x00000400;
        static constexpr u32 Compressed         = 0x00000800;
        static constexpr u32 Offline            = 0x00001000;
        static constexpr u32 NotContentIndexing = 0x00002000;
        static constexpr u32 Encrypted          = 0x00004000;
        static constexpr u32 IntegrityStream    = 0x00008000;
        static constexpr u32 Virtual            = 0x00010000;
        static constexpr u32 NoScrubData        = 0x00020000;
        static constexpr u32 EA                 = 0x00040000;
        static constexpr u32 Pinned             = 0x00080000;
        static constexpr u32 Unpinned           = 0x00100000;
        static constexpr u32 RecallOnOpen       = 0x00200000;
        static constexpr u32 RecallOnDataAccess = 0x00400000;
    };

    class exception : public ::std::exception
    {};

    enum WritingState : u8
    {
        Preparing,
        Writing,
        Closed
    };
    template<WritingState target_state> class ensure : public exception
    {
    public:
        static inline void check(WritingState state)
        {
            if (state == target_state){ return; }
            throw ensure();
        }

        virtual char const* what() const noexcept override
        {
            return "WritingState check failed";
        }
    };
    template<WritingState target_state> class ensure_not : public exception
    {
    public:
        static inline void check(WritingState state)
        {
            if (state != target_state){ return; }
            throw ensure_not();
        }

        virtual char const* what() const noexcept override
        {
            return "WritingState check failed";
        }
    };

    static constexpr u16 VersionMadeOf = 51;
    class VersionNeedToExtra
    {
    public:
        static constexpr u16 Default      = 10;
        static constexpr u16 VolumeLabel  = 11;
        static constexpr u16 Directory    = 20;
        static constexpr u16 Deflate      = 20;
        static constexpr u16 Deflate64    = 21;
        static constexpr u16 PatchDataSet = 27;
        static constexpr u16 Zip64        = 45;
        static constexpr u16 BZip2        = 46;
        static constexpr u16 AEx          = 51;
        static constexpr u16 CDEncryption = 62;
        static constexpr u16 LZMA         = 63;
        static constexpr u16 Blowfish     = 63;
        static constexpr u16 Twofish      = 63;
    };

    class Zip
    {
    public:
        static constexpr u64 BUFFER_LENGTH = 4 * 1024;  // 4KiB

        static Zip create(::std::string const& path)
        {
            auto output_ = new ::std::ofstream(path, ::std::ios::trunc | ::std::ios::binary);
            return Zip(*output_, true);
        }

    protected:
        friend class LocalFile;

        ::std::ostream * _output;
        bool _owned_output;
        WritingState _state;
        i64 _offset;        // zip start

        bool _zip64;
        ::std::list<LocalFile> _files;
        ::std::string _comment;
        PCG_XSH_RR _random;
        u8 * _buffer;       // all writing must pass through this buffer

        Zip(Zip const&) = delete;
        Zip & operator =(Zip const&) = delete;

        void _init()
        {
            _state = WritingState::Writing;
            _offset = _output->tellp();

            _zip64 = false;
            _comment = "";
            _buffer = new u8[BUFFER_LENGTH];
            if (_buffer == nullptr)
            {
                throw ::std::bad_alloc();
            }
        }

        void _gen_salt(u8 * salt, u64 length)
        {
            _random.gen(salt, length);
        }
        void _write(void const* data, u64 length)
        {
            _output->write((char const*)data, length);
        }
        void _write_buffer(u8 * const end)
        {
            _write_buffer(end - _buffer);
        }
        void _write_buffer(u64 length)
        {
            _output->write((char const*)_buffer, length);
        }
        i64 _tellp()
        {
            return _output->tellp() - _offset;
        }
        void _seekp(i64 offset_)
        {
            _output->seekp(_offset + offset_);
        }

        ::std::tuple<u64, u64> _write_central_direction();  // -> (cd_size, cd_offset)
        u64 _write_zip64_record(u64 cd_size, u64 cd_offset) // -> record offset
        {
            u64 record_offset = ::std::bit_cast<u64>(_tellp());

            u8 * record = _buffer;
            _write_into<u32>(record, 0x06064B50);
            _write_into<u64>(record, 44);
            _write_into<u16>(record, VersionMadeOf);
            _write_into<u16>(record, VersionNeedToExtra::Zip64);
            _write_into<u32>(record, 0 /* number of this disk */);
            _write_into<u32>(record, 0 /* number of the disk with the start of the central directory */);
            _write_into<u64>(record, _files.size() /* total number of entries in the central directory on this disk */);
            _write_into<u64>(record, _files.size() /* total number of entries in the central directory */);
            _write_into<u64>(record, cd_size);
            _write_into<u64>(record, cd_offset);
            _write_buffer(record);

            return record_offset;
        }
        void _write_zip64_locator(u64 record_offset)
        {
            u8 * locator = _buffer;
            _write_into<u32>(locator, 0x07064B50);
            _write_into<u32>(locator, 0 /* number of the disk with the start of the zip64 end of central directory */);
            _write_into<u64>(locator, record_offset);
            _write_into<u32>(locator, 1 /* total number of disks */);
            _write_buffer(locator);
        }
        void _write_end_of_central_direction(u64 cd_size, u64 cd_offset)
        {
            u16 total_files = ::std::min(static_cast<u64>(0xFFFF), _files.size());
            cd_size =     ::std::min(static_cast<u64>(0xFFFFFFFF), cd_size);
            cd_offset =   ::std::min(static_cast<u64>(0xFFFFFFFF), cd_offset);
            u16 comment_length = _comment.size() & 0xFFFF;

            u8 * end = _buffer;
            _write_into<u32>(end, 0x06054B50);
            _write_into<u16>(end, 0 /* number of this disk */);
            _write_into<u16>(end, 0 /* number of the disk with the start of the central directory */);
            _write_into<u16>(end, total_files /* total number of entries in the central directory on this disk */);
            _write_into<u16>(end, total_files /* total number of entries in the central directory */);
            _write_into<u32>(end, static_cast<u32>(cd_size));
            _write_into<u32>(end, static_cast<u32>(cd_offset));
            _write_into<u16>(end, comment_length);
            _write_buffer(end);

            _write(_comment.c_str(), comment_length);
        }

    public:
        Zip(::std::ostream & output_, bool owned_output_ = false) noexcept
        : _output(::std::addressof(output_)), _owned_output(owned_output_), _files(), _random() {
            _init();
        }

        ~Zip();

        ::std::ostream & output() noexcept
        {
            return *_output;
        }
        ::std::ostream const& output() const noexcept
        {
            return *_output;
        }

        /* from _output */

        bool good() const
        {
            return _output->good();
        }
        bool fail() const
        {
            return _output->fail();
        }
        bool bad() const
        {
            return _output->bad();
        }
        auto rdstate() const
        {
            return _output->rdstate();
        }

        WritingState state() const noexcept
        {
            return _state;
        }
        /// @brief only determined after the file is closed
        bool zip64() const noexcept
        {
            return _zip64;
        }
        bool central_directory_encrypt() const noexcept
        {
            return false;
        }

        ::std::string const& comment() const noexcept
        {
            return _comment;
        }
        Zip & comment(::std::string const& comment_)
        {
            ensure_not<WritingState::Closed>::check(_state);
            _comment = comment_;
            return *this;
        }

        /// @return return nullptr if no file or zip is closed
        LocalFile * current() noexcept
        {
            if (_state == WritingState::Closed || _files.empty()) { return nullptr; }
            return &_files.back();
        }
        Zip & close_current();

        LocalFile & add(::std::string const& file_name);

        Zip & close()
        {
            if (_state == WritingState::Closed) { return *this; }

            close_current();
            _state = WritingState::Closed;
            auto [cd_size, cd_offset] = _write_central_direction();
            if (_files.size() >= 0xFFFF
             || cd_size       >= 0xFFFFFFFF
             || cd_offset     >= 0xFFFFFFFF
            ) {
                _zip64 = true;
                u64 record_offset = _write_zip64_record(cd_size, cd_offset);
                _write_zip64_locator(record_offset);
            }
            _write_end_of_central_direction(cd_size, cd_offset);

            return *this;
        }
    };

    class LocalFile
    {
        friend class Zip;
    public:
        class SizeOverflowException : public exception
        {
        public:
            u64 compressed_size;
            u64 uncompressed_size;

            SizeOverflowException(u64 cmpr, u64 uncmpr)
            : compressed_size(cmpr), uncompressed_size(uncmpr) {}

            virtual char const* what() const noexcept override
            {
                return "size overflow in LocalFile with zip64 format disable";
            }

            static void check(bool zip64, u64 cmpr, u64 uncmpr)
            {
                if (zip64) { return; }
                if ((cmpr >= 0xFFFFFFFF) || (uncmpr >= 0xFFFFFFFF))
                {
                    throw SizeOverflowException(cmpr, uncmpr);
                }
            }
        };

        class InvalidFileNameException : public exception
        {
        public:
            ::std::string file_name;

            InvalidFileNameException(::std::string const& name)
            : file_name(name) {}

            virtual char const* what() const noexcept override
            {
                return "got a invalid file name";
            }
        };

        static ::std::string safe_file_name(::std::string const& str)
        {
            ::std::string res = str;
            ::std::replace(res.begin(), res.end(), '\\', '/');
            auto idx = res.find_first_not_of('/');
            return (idx == ::std::string::npos) ? "" : res.substr(idx, res.length() - idx);
        }

        class GeneralPurposeBitFlag
        {
        public:
            static constexpr u16 Encrypted         = 0x0001;
            static constexpr u16 DataDescriptor    = 0x0008;
            static constexpr u16 StrongEncryped    = 0x0040;
            static constexpr u16 Utf8              = 0x0800;
            static constexpr u16 MaskedLocalHeader = 0x2000;
        };

    protected:
        Zip & _zip;
        u64 _offset;    // the local file header offset from zip start

        WritingState _state;
        bool _zip64;
        u16 _cmpr_version;
        AbstractCompression * _cmpr;
        u8 _aes_mode;
        AbstractZipAES * _aes;

        u16 _flag;
        u16 _cmpr_method;
        MsDosTime _modified;
        u32 _crc;
        u64 _compressed;
        u64 _uncompressed;
        ::std::string _name;
        ::std::string _comment;
        u32 _external;

        void _init()
        {
            // the offset of LocalFile cannot smaller than that of the zip file
            _offset = ::std::bit_cast<u64>(_zip._tellp());

            _state = WritingState::Preparing;
            _zip64 = false;
            _cmpr_version = VersionNeedToExtra::Default;
            _cmpr = nullptr;
            _aes_mode = 0;
            _aes = nullptr;

            _flag = 0;
            _cmpr_method = 0;
            _modified = MsDosTime(time(nullptr));
            _crc = 0;
            _compressed = 0;
            _uncompressed = 0;
            _name = "";
            _comment = "";
            _external = 0;
        }

        void _rm_cmpr()
        {
            delete _cmpr;
            _cmpr = nullptr;
            _cmpr_method = 0;
            _cmpr_version = VersionNeedToExtra::Default;
        }

#ifdef NYASZIP_WARN
        static bool showed_aes_warn;
        static void _aes_warn()
        {
            if (!showed_aes_warn)
            {
                ::std::cout << "[warn] got an unsupported AES bits, ignore it and use AES-256 instead" << ::std::endl;
                showed_aes_warn = true;
            }
        }
#else
        static void _aes_warn()
        {}
#endif
        void _init_aes(u16 bits)
        {
            delete _aes;

            switch (bits)
            {
            case 128:
                _aes = new ZipAES<128>;
                break;
            case 192:
                _aes = new ZipAES<192>;
                break;
            case 256:
                _aes = new ZipAES<256>;
                break;
            default:
                _aes_warn();
                _aes = new ZipAES<256>;
                break;
            }
            _zip._gen_salt(_aes->salt(), _aes->salt_length());

            _aes_mode = (_aes->bits() >> 6) - 1; // (1|2|3) for AES-(128|192|256);
            _flag |= GeneralPurposeBitFlag::Encrypted;
        }
        void _rm_aes()
        {
            delete _aes;
            _aes = nullptr;
            _aes_mode = 0;
            _flag &= ~GeneralPurposeBitFlag::Encrypted;
        }

        LocalFile(LocalFile const&) = delete;
        LocalFile & operator =(LocalFile const&) = delete;

        /* Preparing => Writing */

        u16 _enpryption_version() const noexcept
        {
            u16 res = VersionNeedToExtra::Default;
            if (_aes_mode != 0)
            {
                res = VersionNeedToExtra::AEx;
            }
            return res;
        }
        u16 _functionality_version() const noexcept
        {
            u16 res = VersionNeedToExtra::Default;
            if (_external & FileAttributes::VolumeLable)
            {
                res = VersionNeedToExtra::VolumeLabel;
            }
            if (_external & FileAttributes::Directory)
            {
                res = VersionNeedToExtra::Directory;
            }
            // TODO: File is a patch data set (27)
            if (_zip64 || (_offset >= 0xFFFFFFFF /* offset is not stored in local header, so the version of local header is not affected by this ? */))
            {
                res = VersionNeedToExtra::Zip64;
            }
            if (_zip.central_directory_encrypt())
            {
                res = VersionNeedToExtra::CDEncryption;
            }
            return res;
        }
        u16 _version() const noexcept
        {
            return ::std::max(_cmpr_version, ::std::max(_enpryption_version(), _functionality_version()));
        }

        u32 _crc_in_header() const noexcept
        {
            return (_flag & GeneralPurposeBitFlag::DataDescriptor) ? 0 : crc();
        }
        ::std::tuple<u32, u32> _sizes_in_header() const noexcept // -> (compressed_size, uncompressed_size)
        {
            u32 cmpr   = static_cast<u32>(_compressed);
            u32 uncmpr = static_cast<u32>(_uncompressed);
            if (_flag & GeneralPurposeBitFlag::DataDescriptor)
            {
                cmpr = uncmpr = 0;
            }
            if (_zip64)
            {
                cmpr = uncmpr = 0xFFFFFFFF;
            }
            return {cmpr, uncmpr};
        }

        u16 _local_extra_length() const noexcept
        {
            u16 len = 0;
            if (_zip64) { len += 20; }
            if (_aes_mode != 0) { len += 11; }
            return len;
        }
        void _write_local_header() const
        {
            u16 cmpr_method = _aes_mode != 0 ? 99 : _cmpr_method;
            u16 file_name_length = _name.size() & 0xFFFF;
            auto [cmpr, uncmpr] = _sizes_in_header();

            u8 * header = _zip._buffer;
            _write_into<u32>(header, 0x04034B50);
            _write_into<u16>(header, _version());
            _write_into<u16>(header, _flag);
            _write_into<u16>(header, cmpr_method);
            _write_into<u16>(header, _modified.time);
            _write_into<u16>(header, _modified.date);
            _write_into<u32>(header, _crc_in_header());
            _write_into<u32>(header, cmpr);
            _write_into<u32>(header, uncmpr);
            _write_into<u16>(header, file_name_length);
            _write_into<u16>(header, _local_extra_length());
            _zip._write_buffer(header);

            _zip._write(_name.c_str(), file_name_length);
            _write_local_extra();
        }
        void _write_local_extra() const
        {
            u8 * fields = _zip._buffer;
            if (_zip64)
            {
                _write_into<u16>(fields, 0x0001);
                _write_into<u16>(fields, 0x0010);
                _write_into<u64>(fields, _uncompressed);
                _write_into<u64>(fields, _compressed);
            }
            if (_aes_mode != 0)
            {
                _write_into<u16>(fields, 0x9901);
                _write_into<u16>(fields, 0x0007);
                _write_into<u16>(fields, 0x0002);   // ZIP AE-2
                _write_into<u16>(fields, 0x4541);   // "AE"
                _write_into<u8 >(fields, _aes_mode);
                _write_into<u16>(fields, _cmpr_method);
            }
            _zip._write_buffer(fields);
        }

        /* Writing */

        void _write_aes_start_data()
        {
            _zip._write(_aes->salt(),      _aes->salt_length());
            _zip._write(_aes->vari_code(), _aes->vari_code_length());
            _compressed += _aes->salt_length() + _aes->vari_code_length();
        }
        void _flush_buffer(u64 length)
        {
            if (_aes_mode != 0)
            {
                // use the authentication code to verify the correctness of the encrypted data in ZIP AE-2,
                // instead of using crc32 to verify the correctness of the un-encrypted data
            }
            else
            {
                _crc = crc32(_crc, _zip._buffer, length);
            }
            _uncompressed += length;

            if (_cmpr == nullptr)
            {
                _compressed += length;

                if (_aes != nullptr)
                {
                    _aes->apply(_zip._buffer, length);
                }
                _zip._write_buffer(length);
            }
            else
            {
                u8 * buffer_ptr = _zip._buffer;
                while (length != 0)
                {
                    auto [consumed, cmpr_length] = _cmpr->compress(buffer_ptr, length);
                    buffer_ptr += consumed; length -= consumed;
                    _compressed += cmpr_length;

                    if (_aes != nullptr)
                    {
                        _aes->apply(_cmpr->buffer(), cmpr_length);
                    }
                    _zip._write(_cmpr->buffer(), cmpr_length);
                }
            }
        }
        void _write_aes_end_data()
        {
            _aes->finalize();
            _zip._write(_aes->auth_code(), _aes->auth_code_length());
            _compressed += _aes->auth_code_length();
        }

        /* Writing => Closed */

        void _update_local_header() const
        {
            auto pos = _zip._tellp();

            /* update static local header */
            u8 * tmp = _zip._buffer;
            auto [cmpr, uncmpr] = _sizes_in_header();
            _write_into<u32>(tmp, _crc_in_header());
            _write_into<u32>(tmp, cmpr);
            _write_into<u32>(tmp, uncmpr);

            _zip._seekp(_offset + 14);
            _zip._write_buffer(tmp);

            /* update zip64 extra field */
            if (_zip64)
            {
                tmp = _zip._buffer;
                _write_into<u64>(tmp, _uncompressed);
                _write_into<u64>(tmp, _compressed);

                u16 file_name_length = _name.size() & 0xFFFF;
                _zip._seekp(_offset + 34 + file_name_length);
                _zip._write_buffer(tmp);
            }

            _zip._seekp(pos);
        }
        void _write_data_descriptor() const
        {
            /* it seems that only a few software support data descriptor */
            if (_flag & GeneralPurposeBitFlag::DataDescriptor)
            {
                u8 * tmp = _zip._buffer;
                _write_into<u32>(tmp, 0x08074B50);
                _write_into<u32>(tmp, crc());

                if (_zip64)
                {
                    _write_into<u64>(tmp, _compressed);
                    _write_into<u64>(tmp, _uncompressed);
                }
                else
                {
                    _write_into<u32>(tmp, static_cast<u32>(_compressed));
                    _write_into<u32>(tmp, static_cast<u32>(_uncompressed));
                }
                _zip._write_buffer(tmp);
            }
        }

        u16 _central_extra_length() const
        {
            u16 len = 0;
            //if (_zip64)  must use zip64 format if offset is greater than 0xFFFFFFFE
            {
                u16 z64_len = 4;
                if (_uncompressed >= 0xFFFFFFFF) { z64_len += 8; }
                if (_compressed   >= 0xFFFFFFFF) { z64_len += 8; }
                if (_offset       >= 0xFFFFFFFF) { z64_len += 8; }
                if (z64_len > 4) { len += z64_len; }
            }
            if (_aes_mode != 0) { len += 11; }
            return len;
        }
        void _write_cd_header() const
        {
            u16 cmpr_method = _aes_mode != 0 ? 99 : _cmpr_method;
            u16 file_name_length = _name.size() & 0xFFFF;
            u16 comment_length = _comment.size() & 0xFFFF;
            /* zip64 */
            u32 offs_  = ::std::min(static_cast<u64>(0xFFFFFFFF), _offset);
            auto [cmpr, uncmpr] = _sizes_in_header();

            u8 * header = _zip._buffer;
            _write_into<u32>(header, 0x02014B50);
            _write_into<u16>(header, VersionMadeOf);
            _write_into<u16>(header, _version());
            _write_into<u16>(header, _flag);
            _write_into<u16>(header, cmpr_method);
            _write_into<u16>(header, _modified.time);
            _write_into<u16>(header, _modified.date);
            _write_into<u32>(header, _crc_in_header());
            _write_into<u32>(header, cmpr);
            _write_into<u32>(header, uncmpr);
            _write_into<u16>(header, file_name_length);
            _write_into<u16>(header, _central_extra_length());
            _write_into<u16>(header, comment_length);
            _write_into<u16>(header, 0 /* disk number start */);
            _write_into<u16>(header, 0 /* internal file attributes */);
            _write_into<u32>(header, _external);
            _write_into<u32>(header, offs_);
            _zip._write_buffer(header);

            _zip._write(_name.c_str(), file_name_length);
            _write_central_extra();
            _zip._write(_comment.c_str(), comment_length);
        }
        void _write_central_extra() const
        {
            u8 * fields = _zip._buffer;
            //if (_zip64)  must use zip64 format if offset is greater than 0xFFFFFFFE
            {
                u16 * counter = reinterpret_cast<u16 *>(fields + 2);

                _write_into<u16>(fields, 0x0001);
                _write_into<u16>(fields, 0 /* counter */);
                if (_uncompressed >= 0xFFFFFFFF) {
                    _write_into<u64>(fields, _uncompressed);
                    *counter += sizeof(u64);
                }
                if (_compressed >= 0xFFFFFFFF) {
                    _write_into<u64>(fields, _compressed);
                    *counter += sizeof(u64);
                }
                if (_offset >= 0xFFFFFFFF) {
                    _write_into<u64>(fields, _offset);
                    *counter += sizeof(u64);
                }
                if (*counter == 0)
                {
                    // nothing need to store in zip64 field, drop
                    fields = _zip._buffer;
                }
            }
            if (_aes_mode != 0)
            {
                _write_into<u16>(fields, 0x9901);
                _write_into<u16>(fields, 0x0007);
                _write_into<u16>(fields, 0x0002);   // ZIP AE-2
                _write_into<u16>(fields, 0x4541);   // "AE"
                _write_into<u8 >(fields, _aes_mode);
                _write_into<u16>(fields, _cmpr_method);
            }
            _zip._write_buffer(fields);
        }

    public:
        LocalFile(Zip & zip_)
        : _zip(zip_) {
            _init();
        }

        ~LocalFile()
        {
            delete _cmpr;
            delete _aes;
        }

        Zip & zip() noexcept
        {
            return _zip;
        }
        Zip const& zip() const noexcept
        {
            return _zip;
        }
        i64 offset() const noexcept
        {
            return _offset;
        }
        WritingState state() const noexcept
        {
            return _state;
        }
        bool zip64() const noexcept
        {
            return _zip64;
        }
        u16 version_need_to_extra() const noexcept
        {
            return _version();
        }
        u16 general_purpose_bit_flag() const noexcept
        {
            return _flag;
        }
        u16 compression_method() const noexcept
        {
            return _cmpr_method;
        }
        MsDosTime modified() const noexcept
        {
            return _modified;
        }
        u32 crc() const noexcept
        {
            return _aes_mode != 0 ? 0 : _crc;
        }
        u64 compressed_size() const noexcept
        {
            return _compressed;
        }
        u64 uncompressed_size() const noexcept
        {
            return _uncompressed;
        }
        ::std::string const& name() const noexcept
        {
            return _name;
        }
        ::std::string const& comment() const noexcept
        {
            return _comment;
        }
        u32 external_attribute() const noexcept
        {
            return _external;
        }

        /// @brief set the file enable zip64 format or not, will throw error if not enble (default) and write in more than 4GB data
        LocalFile & zip64(bool enable = true)
        {
            ensure<WritingState::Preparing>::check(_state);
            _zip64 = enable;
            return *this;
        }
        LocalFile & name(::std::string const& name_)
        {
            ensure<WritingState::Preparing>::check(_state);
            auto safe_name_ = safe_file_name(name_);
#ifndef NYASZIP_ALLOW_EMPTY_FILENAME
            if (safe_name_.empty())
            {
                // note that the zip file is not preventing users from entering an empty file name,
                // but the extraction behavior is not well defined.
                throw InvalidFileNameException(name_);
            }
#endif
            _name = safe_name_;
            return *this;
        }
        LocalFile & comment(::std::string const& comment_)
        {
            ensure_not<WritingState::Closed>::check(_zip.state());
            _comment = comment_;
            return *this;
        }
        /// @brief indicates that the file name & comment are utf-8 encoded
        LocalFile & utf8(bool is_utf8 = true)
        {
            ensure_not<WritingState::Closed>::check(_zip.state());
            if (is_utf8)
            {
                _flag |= GeneralPurposeBitFlag::Utf8;
            }
            else
            {
                _flag &= ~GeneralPurposeBitFlag::Utf8;
            }
            // update local file header
            if (_state != WritingState::Preparing)
            {
                auto pos = _zip._tellp();
                _zip._seekp(_offset + 6);
                _zip._write(&_flag, sizeof(u16));
                _zip._seekp(pos);
            }
            return *this;
        }
        /// @brief change the last modifird time
        LocalFile & modified(MsDosTime modified_)
        {
            ensure_not<WritingState::Closed>::check(_zip.state());
            _modified = modified_;
            // update local file header
            if (_state != WritingState::Preparing)
            {
                auto pos = _zip._tellp();
                _zip._seekp(_offset + 10);
                _zip._write(&_modified.time, sizeof(u16));
                _zip._write(&_modified.date, sizeof(u16));
                _zip._seekp(pos);
            }
            return *this;
        }
        /// @brief change the external attribute, i.e., file attribute in the file system
        LocalFile & external_attribute(u32 exter)
        {
            // should have some restriction, like,
            // if this local file is a directory (0x00000010), then its size must be 0.
            // may split this into smaller methods?
            ensure_not<WritingState::Closed>::check(_zip.state());
            _external = exter;
            return *this;
        }

        // clear the password
        LocalFile & password(nullptr_t)
        {
            ensure<WritingState::Preparing>::check(_state);
            _rm_aes();
            return *this;
        }
        // set the password
        LocalFile & password(::std::string const& password_, u16 AES_bits = 256)
        {
            return password(reinterpret_cast<u8 const*>(password_.c_str()), password_.size(), AES_bits);
        }
        // set the password
        LocalFile & password(u8 const* password_, u64 length, u16 AES_bits = 256)
        {
            ensure<WritingState::Preparing>::check(_state);
            if (_aes == nullptr || _aes->bits() != AES_bits)
            {
                _init_aes(AES_bits);
            }
            _aes->set(password_, length);
            return *this;
        }

        LocalFile & start()
        {
            if (_state != WritingState::Preparing) { return *this; }

            _write_local_header();
            _state = WritingState::Writing;
            if (_aes != nullptr) { _write_aes_start_data(); }

            return *this;
        }

        u8 * buffer() noexcept
        {
            // Zip & LocalFile are share the same buffer,
            // must write local header first before writing data into LocalFile
            start();
            return _zip._buffer;
        }
        static constexpr u64 buffer_length() noexcept
        {
            return Zip::BUFFER_LENGTH;
        }
        LocalFile & flush_buff(u64 length)
        {
            start();
            ensure_not<WritingState::Closed>::check(_state);
            _flush_buffer(length);
            SizeOverflowException::check(_zip64, _compressed, _uncompressed);
            return *this;
        }

        LocalFile & write(u8 const* data, u64 length)
        {
            start();
            ensure_not<WritingState::Closed>::check(_state);
            while (length != 0)
            {
                u64 flush_length = ::std::min(length, buffer_length());
                ::std::memcpy(buffer(), data, flush_length);
                _flush_buffer(flush_length);
                data += flush_length; length -= flush_length;
            }
            SizeOverflowException::check(_zip64, _compressed, _uncompressed);
            return *this;
        }
        LocalFile & write(::std::string const& data)
        {
            return write(reinterpret_cast<u8 const*>(data.c_str()), data.size());
        }

        LocalFile & close()
        {
            if (_state == WritingState::Closed) { return *this; }
            if (_state == WritingState::Preparing) {
                // zero-length file or directory cannot have compression and enpryption
                _zip64 = false;
                _rm_cmpr();
                _rm_aes();
                _write_local_header();
            }

            if (_aes != nullptr) { _write_aes_end_data(); }
            _state = WritingState::Closed;
            _update_local_header();
            _write_data_descriptor();

            delete _cmpr;
            delete _aes;
            _cmpr = nullptr;
            _aes = nullptr;

            return *this;
        }
    };

#ifdef NYASZIP_WARN
    bool LocalFile::showed_aes_warn = false;
#endif

    Zip::~Zip()
    {
        _files.clear();
        delete _buffer;
        if (_owned_output)
        {
            _output->flush();
            delete _output;
        }
    }
    ::std::tuple<u64, u64> Zip::_write_central_direction()
    {
        u64 cd_offset = ::std::bit_cast<u64>(_tellp());

        for (LocalFile const& file : _files)
        {
            file._write_cd_header();
        }

        u64 cd_size = ::std::bit_cast<u64>(_tellp()) - cd_offset;
        return {cd_size, cd_offset};
    }
    Zip & Zip::close_current()
    {
        auto curr = current();
        if (curr != nullptr) { curr->close(); }
        return *this;
    }
    LocalFile & Zip::add(::std::string const& file_name)
    {
        ensure<WritingState::Writing>::check(_state);
        close_current();

        _files.emplace_back(*this);

        LocalFile & local_file = _files.back();
        local_file.name(file_name);

        return local_file;
    }

}   // namespace nyaszip