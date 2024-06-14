#pragma once
#include <array>
#include <string_view>
#include <cassert>

// Both Enum and Enum::Set classes have Natvis support - otherwise they would be very hard to read during debug.
// Supporting this requires a little extra static memory - disable this macro if you don't care about natvis.
#define MODERN_ENUM_NATVIS_SUPPORT_ENABLED 1

#define MODERN_ENUM(Enum, ...) \
    class Enum\
    {\
        _MODERN_ENUM_PRIVATE_MEMBERS(Enum, __VA_ARGS__)\
\
        class Set\
        {\
            _MODERN_SET_PRIVATE_MEMBERS(Enum, __VA_ARGS__)\
\
            constexpr Set();\
\
            constexpr uint32_t count()    const;\
\
            constexpr bool all()    const;\
            constexpr bool none()   const;\
            constexpr bool any()    const;\
\
            constexpr bool contains(Set) const;\
\
            friend constexpr bool operator == (Set, Set);\
            friend constexpr bool operator != (Set, Set);\
\
            friend constexpr Set operator |  (Set, Set);\
            friend constexpr Set operator &  (Set, Set);\
            friend constexpr Set operator ^  (Set, Set);\
            friend constexpr Set operator ~  (Set);\
\
            constexpr inline Set& operator |= (Set);\
            constexpr inline Set& operator &= (Set);\
            constexpr inline Set& operator ^= (Set);\
        };\
\
        _MODERN_ENUM_DECLARE_ELEMENTS(Enum, __VA_ARGS__)\
\
        constexpr Enum();\
\
        constexpr bool operator == (Enum) const;\
        constexpr bool operator != (Enum) const;\
\
        constexpr static uint32_t                GetCount();\
        constexpr static std::string_view        GetName();\
        constexpr static const EnumArray&        GetElements();\
\
        constexpr static Enum                    FromString(std::string_view);\
        constexpr static Enum                    FromIndex(uint32_t);\
\
        constexpr std::string_view toString() const; \
        constexpr uint32_t index() const; \
\
        constexpr operator Set() const;\
\
        constexpr operator _InternalRawEnumToSupportSwitchStatement() const;\
    }; \
\
    _MODERN_ENUM_AND_SET_EXTERNAL_DEFINITIONS(Enum, __VA_ARGS__)




namespace internal::modern_enum
{
    template<uint64_t BitCount>
    class BitSet
    {
    public:
        constexpr BitSet() : m_bits{} {}

        constexpr BitSet(uint32_t idx) : m_bits{}
        {
            assert(idx < BitCount);

            m_bits[idx / s_BucketSize] = (1ull << (idx % s_BucketSize));
        }

        constexpr bool operator ==(const BitSet& o) const
        {
            for (int i = 0; i < s_BucketCount; i++)
                if (m_bits[i] ^ o.m_bits[i])
                    return false;
            return true;
        }

        constexpr bool operator !=(const BitSet& o) const { return !(*this == o); }

        constexpr uint32_t count() const
        {
            uint32_t count = 0;
            for (uint64_t i = 0; i < BitCount; i++)
                if (m_bits[i/s_BucketSize] & (1llu << (i%s_BucketSize)))
                    count++;
            return count;
        }

        constexpr bool all() const
        {
            for (int i = 0; i < s_BucketCount; i++)
                if (m_bits[i] != s_masks[i])
                    return false;
            return true;
        }

        constexpr bool none() const
        {
            for (int i = 0; i < s_BucketCount; i++)
                if (m_bits[i] != 0)
                    return false;
            return true;
        }

        constexpr bool any() const { return !none(); }


        constexpr BitSet operator |(const BitSet& o) const
        {
            BitSet res;
            for (int i = 0; i < s_BucketCount; i++)
                res.m_bits[i] = m_bits[i] | o.m_bits[i];
            return res;
        }

        constexpr BitSet operator &(const BitSet& o) const
        {
            BitSet res;
            for (int i = 0; i < s_BucketCount; i++)
                res.m_bits[i] = m_bits[i] & o.m_bits[i];
            return res;
        }

        constexpr BitSet operator ^(const BitSet& o) const
        {
            BitSet res;
            for (int i = 0; i < s_BucketCount; i++)
                res.m_bits[i] = m_bits[i] ^ o.m_bits[i];
            return res;
        }

        constexpr BitSet operator ~() const
        {
            BitSet res;
            for (int i = 0; i < s_BucketCount; i++)
                res.m_bits[i] = ~m_bits[i] & s_masks[i];
            return res;
        }

    private:
        constexpr static uint64_t s_BucketSize = 64;
        constexpr static uint64_t s_BucketCount = (BitCount - 1) / s_BucketSize + 1;

        using Buckets = std::array<uint64_t, s_BucketCount>;

        constexpr static Buckets MakeMasks()
        {
            constexpr uint64_t LastBucketBitCount = BitCount - (s_BucketCount - 1) * s_BucketSize;
            constexpr uint64_t LastBucketMask = (1ull << (LastBucketBitCount % s_BucketSize)) - 1ull;
            Buckets masks{};
            for (int i = 0; i < s_BucketCount - 1; i++) masks[i] = 0xFFFFFFFFFFFFFFFF;
            masks[s_BucketCount - 1] = LastBucketMask;
            return masks;
        }

        Buckets m_bits;
        Buckets s_masks = MakeMasks();
    };


    template <class Enum>
    struct NatvisIndex
    {
        constexpr NatvisIndex(uint32_t idx) : idx(idx) {}
        uint32_t idx;
    };

    template <class Enum, uint32_t Count>
    struct NatvisBitSet
    {
        constexpr NatvisBitSet(BitSet<Count> set) : set(set) {}
        BitSet<Count> set;
    };

    template<class T>
    const T& ForceGenerationForNatvis(const T& t)
    {
        return t;
    }


    template <uint32_t Count>
    constexpr static uint32_t CreateFromString(std::string_view className, std::string_view name, const std::array<std::string_view, Count>& names)
    {
        for (int i = 0; i < Count; i++) if (names[i] == name) return i;
        assert(false);
    }
}

// Modify the following code to increase the maximum number of elements supported:
// 1) _MODERN_ENUM_COUNT_ELEMENTS_MACRO
// 2) _MODERN_ENUM_COUNT_ELEMENTS
// 3) _MODERN_ENUM_RECURSIVE_**
// Note that if you are using Msvc, in order to have more than 127 elements, you should use the /Zc:preprocessor flag

#define _MODERN_ENUM_COUNT_ELEMENTS_MACRO(\
          _001, _002, _003, _004, _005, _006, _007, _008, _009,\
    _010, _011, _012, _013, _014, _015, _016, _017, _018, _019,\
    _020, _021, _022, _023, _024, _025, _026, _027, _028, _029,\
    _030, _031, _032, _033, _034, _035, _036, _037, _038, _039,\
    _040, _041, _042, _043, _044, _045, _046, _047, _048, _049,\
    _050, _051, _052, _053, _054, _055, _056, _057, _058, _059,\
    _060, _061, _062, _063, _064, _065, _066, _067, _068, _069,\
    _070, _071, _072, _073, _074, _075, _076, _077, _078, _079,\
    _080, _081, _082, _083, _084, _085, _086, _087, _088, _089,\
    _090, _091, _092, _093, _094, _095, _096, _097, _098, _099,\
    _100, _101, _102, _103, _104, _105, _106, _107, _108, _109,\
    _110, _111, _112, _113, _114, _115, _116, _117, _118, _119,\
    _120, _121, _122, _123, _124, _125, _126, _127, _128, _129,\
    _130, _131, _132, _133, _134, _135, _136, _137, _138, _139,\
    _140, _141, _142, _143, _144, _145, _146, _147, _148, _149,\
    _150, _151, _152, _153, _154, _155, _156, _157, _158, _159,\
    _160, _161, _162, _163, _164, _165, _166, _167, _168, _169,\
    _170, _171, _172, _173, _174, _175, _176, _177, _178, _179,\
    _180, _181, _182, _183, _184, _185, _186, _187, _188, _189,\
    _190, _191, _192, _193, _194, _195, _196, _197, _198, _199,\
    Count, ...) Count

#define _MODERN_ENUM_COUNT_ELEMENTS(...) \
    _MODERN_ENUM_COUNT_ELEMENTS_MACRO(__VA_ARGS__, \
    199, 198, 197, 196, 195, 194, 193, 192, 191, 190,\
    189, 188, 187, 186, 185, 184, 183, 182, 181, 180,\
    179, 178, 177, 176, 175, 174, 173, 172, 171, 170,\
    169, 168, 167, 166, 165, 164, 163, 162, 161, 160,\
    159, 158, 157, 156, 155, 154, 153, 152, 151, 150,\
    149, 148, 147, 146, 145, 144, 143, 142, 141, 140,\
    139, 138, 137, 136, 135, 134, 133, 132, 131, 130,\
    129, 128, 127, 126, 125, 124, 123, 122, 121, 120,\
    119, 118, 117, 116, 115, 114, 113, 112, 111, 110,\
    109, 108, 107, 106, 105, 104, 103, 102, 101, 100,\
     99,  98,  97,  96,  95,  94,  93,  92,  91,  90,\
     89,  88,  87,  86,  85,  84,  83,  82,  81,  80,\
     79,  78,  77,  76,  75,  74,  73,  72,  71,  70,\
     69,  68,  67,  66,  65,  64,  63,  62,  61,  60,\
     59,  58,  57,  56,  55,  54,  53,  52,  51,  50,\
     49,  48,  47,  46,  45,  44,  43,  42,  41,  40,\
     39,  38,  37,  36,  35,  34,  33,  32,  31,  30,\
     29,  28,  27,  26,  25,  24,  23,  22,  21,  20,\
     19,  18,  17,  16,  15,  14,  13,  12,  11,  10,\
      9,   8,   7,   6,   5,   4,   3,   2,   1,  )

#define _MODERN_ENUM_RECURSIVE_1(Macro, Separator, GlobalParam, Param)           Macro(GlobalParam,   0, Param) 
#define _MODERN_ENUM_RECURSIVE_2(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   1, Param)   Separator()   _MODERN_ENUM_RECURSIVE_1(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_3(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   2, Param)   Separator()   _MODERN_ENUM_RECURSIVE_2(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_4(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   3, Param)   Separator()   _MODERN_ENUM_RECURSIVE_3(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_5(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   4, Param)   Separator()   _MODERN_ENUM_RECURSIVE_4(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_6(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   5, Param)   Separator()   _MODERN_ENUM_RECURSIVE_5(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_7(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   6, Param)   Separator()   _MODERN_ENUM_RECURSIVE_6(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_8(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   7, Param)   Separator()   _MODERN_ENUM_RECURSIVE_7(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_9(Macro, Separator, GlobalParam, Param, ...)      Macro(GlobalParam,   8, Param)   Separator()   _MODERN_ENUM_RECURSIVE_8(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_10(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,   9, Param)   Separator()   _MODERN_ENUM_RECURSIVE_9(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_11(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  10, Param)   Separator()   _MODERN_ENUM_RECURSIVE_10(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_12(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  11, Param)   Separator()   _MODERN_ENUM_RECURSIVE_11(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_13(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  12, Param)   Separator()   _MODERN_ENUM_RECURSIVE_12(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_14(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  13, Param)   Separator()   _MODERN_ENUM_RECURSIVE_13(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_15(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  14, Param)   Separator()   _MODERN_ENUM_RECURSIVE_14(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_16(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  15, Param)   Separator()   _MODERN_ENUM_RECURSIVE_15(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_17(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  16, Param)   Separator()   _MODERN_ENUM_RECURSIVE_16(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_18(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  17, Param)   Separator()   _MODERN_ENUM_RECURSIVE_17(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_19(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  18, Param)   Separator()   _MODERN_ENUM_RECURSIVE_18(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_20(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  19, Param)   Separator()   _MODERN_ENUM_RECURSIVE_19(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_21(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  20, Param)   Separator()   _MODERN_ENUM_RECURSIVE_20(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_22(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  21, Param)   Separator()   _MODERN_ENUM_RECURSIVE_21(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_23(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  22, Param)   Separator()   _MODERN_ENUM_RECURSIVE_22(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_24(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  23, Param)   Separator()   _MODERN_ENUM_RECURSIVE_23(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_25(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  24, Param)   Separator()   _MODERN_ENUM_RECURSIVE_24(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_26(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  25, Param)   Separator()   _MODERN_ENUM_RECURSIVE_25(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_27(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  26, Param)   Separator()   _MODERN_ENUM_RECURSIVE_26(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_28(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  27, Param)   Separator()   _MODERN_ENUM_RECURSIVE_27(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_29(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  28, Param)   Separator()   _MODERN_ENUM_RECURSIVE_28(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_30(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  29, Param)   Separator()   _MODERN_ENUM_RECURSIVE_29(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_31(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  30, Param)   Separator()   _MODERN_ENUM_RECURSIVE_30(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_32(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  31, Param)   Separator()   _MODERN_ENUM_RECURSIVE_31(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_33(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  32, Param)   Separator()   _MODERN_ENUM_RECURSIVE_32(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_34(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  33, Param)   Separator()   _MODERN_ENUM_RECURSIVE_33(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_35(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  34, Param)   Separator()   _MODERN_ENUM_RECURSIVE_34(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_36(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  35, Param)   Separator()   _MODERN_ENUM_RECURSIVE_35(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_37(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  36, Param)   Separator()   _MODERN_ENUM_RECURSIVE_36(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_38(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  37, Param)   Separator()   _MODERN_ENUM_RECURSIVE_37(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_39(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  38, Param)   Separator()   _MODERN_ENUM_RECURSIVE_38(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_40(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  39, Param)   Separator()   _MODERN_ENUM_RECURSIVE_39(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_41(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  40, Param)   Separator()   _MODERN_ENUM_RECURSIVE_40(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_42(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  41, Param)   Separator()   _MODERN_ENUM_RECURSIVE_41(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_43(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  42, Param)   Separator()   _MODERN_ENUM_RECURSIVE_42(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_44(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  43, Param)   Separator()   _MODERN_ENUM_RECURSIVE_43(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_45(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  44, Param)   Separator()   _MODERN_ENUM_RECURSIVE_44(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_46(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  45, Param)   Separator()   _MODERN_ENUM_RECURSIVE_45(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_47(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  46, Param)   Separator()   _MODERN_ENUM_RECURSIVE_46(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_48(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  47, Param)   Separator()   _MODERN_ENUM_RECURSIVE_47(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_49(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  48, Param)   Separator()   _MODERN_ENUM_RECURSIVE_48(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_50(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  49, Param)   Separator()   _MODERN_ENUM_RECURSIVE_49(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_51(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  50, Param)   Separator()   _MODERN_ENUM_RECURSIVE_50(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_52(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  51, Param)   Separator()   _MODERN_ENUM_RECURSIVE_51(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_53(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  52, Param)   Separator()   _MODERN_ENUM_RECURSIVE_52(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_54(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  53, Param)   Separator()   _MODERN_ENUM_RECURSIVE_53(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_55(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  54, Param)   Separator()   _MODERN_ENUM_RECURSIVE_54(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_56(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  55, Param)   Separator()   _MODERN_ENUM_RECURSIVE_55(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_57(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  56, Param)   Separator()   _MODERN_ENUM_RECURSIVE_56(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_58(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  57, Param)   Separator()   _MODERN_ENUM_RECURSIVE_57(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_59(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  58, Param)   Separator()   _MODERN_ENUM_RECURSIVE_58(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_60(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  59, Param)   Separator()   _MODERN_ENUM_RECURSIVE_59(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_61(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  60, Param)   Separator()   _MODERN_ENUM_RECURSIVE_60(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_62(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  61, Param)   Separator()   _MODERN_ENUM_RECURSIVE_61(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_63(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  62, Param)   Separator()   _MODERN_ENUM_RECURSIVE_62(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_64(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  63, Param)   Separator()   _MODERN_ENUM_RECURSIVE_63(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_65(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  64, Param)   Separator()   _MODERN_ENUM_RECURSIVE_64(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_66(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  65, Param)   Separator()   _MODERN_ENUM_RECURSIVE_65(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_67(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  66, Param)   Separator()   _MODERN_ENUM_RECURSIVE_66(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_68(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  67, Param)   Separator()   _MODERN_ENUM_RECURSIVE_67(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_69(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  68, Param)   Separator()   _MODERN_ENUM_RECURSIVE_68(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_70(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  69, Param)   Separator()   _MODERN_ENUM_RECURSIVE_69(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_71(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  70, Param)   Separator()   _MODERN_ENUM_RECURSIVE_70(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_72(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  71, Param)   Separator()   _MODERN_ENUM_RECURSIVE_71(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_73(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  72, Param)   Separator()   _MODERN_ENUM_RECURSIVE_72(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_74(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  73, Param)   Separator()   _MODERN_ENUM_RECURSIVE_73(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_75(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  74, Param)   Separator()   _MODERN_ENUM_RECURSIVE_74(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_76(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  75, Param)   Separator()   _MODERN_ENUM_RECURSIVE_75(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_77(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  76, Param)   Separator()   _MODERN_ENUM_RECURSIVE_76(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_78(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  77, Param)   Separator()   _MODERN_ENUM_RECURSIVE_77(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_79(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  78, Param)   Separator()   _MODERN_ENUM_RECURSIVE_78(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_80(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  79, Param)   Separator()   _MODERN_ENUM_RECURSIVE_79(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_81(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  80, Param)   Separator()   _MODERN_ENUM_RECURSIVE_80(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_82(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  81, Param)   Separator()   _MODERN_ENUM_RECURSIVE_81(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_83(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  82, Param)   Separator()   _MODERN_ENUM_RECURSIVE_82(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_84(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  83, Param)   Separator()   _MODERN_ENUM_RECURSIVE_83(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_85(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  84, Param)   Separator()   _MODERN_ENUM_RECURSIVE_84(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_86(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  85, Param)   Separator()   _MODERN_ENUM_RECURSIVE_85(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_87(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  86, Param)   Separator()   _MODERN_ENUM_RECURSIVE_86(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_88(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  87, Param)   Separator()   _MODERN_ENUM_RECURSIVE_87(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_89(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  88, Param)   Separator()   _MODERN_ENUM_RECURSIVE_88(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_90(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  89, Param)   Separator()   _MODERN_ENUM_RECURSIVE_89(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_91(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  90, Param)   Separator()   _MODERN_ENUM_RECURSIVE_90(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_92(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  91, Param)   Separator()   _MODERN_ENUM_RECURSIVE_91(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_93(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  92, Param)   Separator()   _MODERN_ENUM_RECURSIVE_92(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_94(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  93, Param)   Separator()   _MODERN_ENUM_RECURSIVE_93(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_95(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  94, Param)   Separator()   _MODERN_ENUM_RECURSIVE_94(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_96(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  95, Param)   Separator()   _MODERN_ENUM_RECURSIVE_95(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_97(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  96, Param)   Separator()   _MODERN_ENUM_RECURSIVE_96(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_98(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  97, Param)   Separator()   _MODERN_ENUM_RECURSIVE_97(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_99(Macro, Separator, GlobalParam, Param, ...)     Macro(GlobalParam,  98, Param)   Separator()   _MODERN_ENUM_RECURSIVE_98(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_100(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam,  99, Param)   Separator()   _MODERN_ENUM_RECURSIVE_99(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_101(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 100, Param)   Separator()   _MODERN_ENUM_RECURSIVE_100(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_102(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 101, Param)   Separator()   _MODERN_ENUM_RECURSIVE_101(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_103(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 102, Param)   Separator()   _MODERN_ENUM_RECURSIVE_102(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_104(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 103, Param)   Separator()   _MODERN_ENUM_RECURSIVE_103(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_105(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 104, Param)   Separator()   _MODERN_ENUM_RECURSIVE_104(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_106(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 105, Param)   Separator()   _MODERN_ENUM_RECURSIVE_105(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_107(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 106, Param)   Separator()   _MODERN_ENUM_RECURSIVE_106(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_108(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 107, Param)   Separator()   _MODERN_ENUM_RECURSIVE_107(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_109(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 108, Param)   Separator()   _MODERN_ENUM_RECURSIVE_108(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_110(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 109, Param)   Separator()   _MODERN_ENUM_RECURSIVE_109(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_111(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 110, Param)   Separator()   _MODERN_ENUM_RECURSIVE_110(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_112(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 111, Param)   Separator()   _MODERN_ENUM_RECURSIVE_111(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_113(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 112, Param)   Separator()   _MODERN_ENUM_RECURSIVE_112(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_114(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 113, Param)   Separator()   _MODERN_ENUM_RECURSIVE_113(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_115(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 114, Param)   Separator()   _MODERN_ENUM_RECURSIVE_114(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_116(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 115, Param)   Separator()   _MODERN_ENUM_RECURSIVE_115(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_117(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 116, Param)   Separator()   _MODERN_ENUM_RECURSIVE_116(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_118(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 117, Param)   Separator()   _MODERN_ENUM_RECURSIVE_117(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_119(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 118, Param)   Separator()   _MODERN_ENUM_RECURSIVE_118(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_120(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 119, Param)   Separator()   _MODERN_ENUM_RECURSIVE_119(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_121(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 120, Param)   Separator()   _MODERN_ENUM_RECURSIVE_120(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_122(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 121, Param)   Separator()   _MODERN_ENUM_RECURSIVE_121(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_123(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 122, Param)   Separator()   _MODERN_ENUM_RECURSIVE_122(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_124(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 123, Param)   Separator()   _MODERN_ENUM_RECURSIVE_123(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_125(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 124, Param)   Separator()   _MODERN_ENUM_RECURSIVE_124(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_126(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 125, Param)   Separator()   _MODERN_ENUM_RECURSIVE_125(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_127(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 126, Param)   Separator()   _MODERN_ENUM_RECURSIVE_126(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_128(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 127, Param)   Separator()   _MODERN_ENUM_RECURSIVE_127(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_129(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 128, Param)   Separator()   _MODERN_ENUM_RECURSIVE_128(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_130(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 129, Param)   Separator()   _MODERN_ENUM_RECURSIVE_129(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_131(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 130, Param)   Separator()   _MODERN_ENUM_RECURSIVE_130(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_132(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 131, Param)   Separator()   _MODERN_ENUM_RECURSIVE_131(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_133(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 132, Param)   Separator()   _MODERN_ENUM_RECURSIVE_132(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_134(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 133, Param)   Separator()   _MODERN_ENUM_RECURSIVE_133(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_135(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 134, Param)   Separator()   _MODERN_ENUM_RECURSIVE_134(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_136(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 135, Param)   Separator()   _MODERN_ENUM_RECURSIVE_135(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_137(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 136, Param)   Separator()   _MODERN_ENUM_RECURSIVE_136(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_138(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 137, Param)   Separator()   _MODERN_ENUM_RECURSIVE_137(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_139(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 138, Param)   Separator()   _MODERN_ENUM_RECURSIVE_138(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_140(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 139, Param)   Separator()   _MODERN_ENUM_RECURSIVE_139(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_141(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 140, Param)   Separator()   _MODERN_ENUM_RECURSIVE_140(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_142(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 141, Param)   Separator()   _MODERN_ENUM_RECURSIVE_141(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_143(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 142, Param)   Separator()   _MODERN_ENUM_RECURSIVE_142(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_144(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 143, Param)   Separator()   _MODERN_ENUM_RECURSIVE_143(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_145(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 144, Param)   Separator()   _MODERN_ENUM_RECURSIVE_144(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_146(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 145, Param)   Separator()   _MODERN_ENUM_RECURSIVE_145(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_147(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 146, Param)   Separator()   _MODERN_ENUM_RECURSIVE_146(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_148(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 147, Param)   Separator()   _MODERN_ENUM_RECURSIVE_147(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_149(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 148, Param)   Separator()   _MODERN_ENUM_RECURSIVE_148(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_150(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 149, Param)   Separator()   _MODERN_ENUM_RECURSIVE_149(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_151(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 150, Param)   Separator()   _MODERN_ENUM_RECURSIVE_150(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_152(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 151, Param)   Separator()   _MODERN_ENUM_RECURSIVE_151(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_153(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 152, Param)   Separator()   _MODERN_ENUM_RECURSIVE_152(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_154(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 153, Param)   Separator()   _MODERN_ENUM_RECURSIVE_153(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_155(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 154, Param)   Separator()   _MODERN_ENUM_RECURSIVE_154(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_156(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 155, Param)   Separator()   _MODERN_ENUM_RECURSIVE_155(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_157(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 156, Param)   Separator()   _MODERN_ENUM_RECURSIVE_156(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_158(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 157, Param)   Separator()   _MODERN_ENUM_RECURSIVE_157(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_159(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 158, Param)   Separator()   _MODERN_ENUM_RECURSIVE_158(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_160(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 159, Param)   Separator()   _MODERN_ENUM_RECURSIVE_159(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_161(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 160, Param)   Separator()   _MODERN_ENUM_RECURSIVE_160(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_162(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 161, Param)   Separator()   _MODERN_ENUM_RECURSIVE_161(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_163(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 162, Param)   Separator()   _MODERN_ENUM_RECURSIVE_162(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_164(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 163, Param)   Separator()   _MODERN_ENUM_RECURSIVE_163(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_165(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 164, Param)   Separator()   _MODERN_ENUM_RECURSIVE_164(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_166(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 165, Param)   Separator()   _MODERN_ENUM_RECURSIVE_165(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_167(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 166, Param)   Separator()   _MODERN_ENUM_RECURSIVE_166(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_168(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 167, Param)   Separator()   _MODERN_ENUM_RECURSIVE_167(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_169(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 168, Param)   Separator()   _MODERN_ENUM_RECURSIVE_168(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_170(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 169, Param)   Separator()   _MODERN_ENUM_RECURSIVE_169(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_171(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 170, Param)   Separator()   _MODERN_ENUM_RECURSIVE_170(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_172(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 171, Param)   Separator()   _MODERN_ENUM_RECURSIVE_171(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_173(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 172, Param)   Separator()   _MODERN_ENUM_RECURSIVE_172(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_174(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 173, Param)   Separator()   _MODERN_ENUM_RECURSIVE_173(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_175(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 174, Param)   Separator()   _MODERN_ENUM_RECURSIVE_174(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_176(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 175, Param)   Separator()   _MODERN_ENUM_RECURSIVE_175(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_177(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 176, Param)   Separator()   _MODERN_ENUM_RECURSIVE_176(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_178(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 177, Param)   Separator()   _MODERN_ENUM_RECURSIVE_177(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_179(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 178, Param)   Separator()   _MODERN_ENUM_RECURSIVE_178(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_180(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 179, Param)   Separator()   _MODERN_ENUM_RECURSIVE_179(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_181(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 180, Param)   Separator()   _MODERN_ENUM_RECURSIVE_180(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_182(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 181, Param)   Separator()   _MODERN_ENUM_RECURSIVE_181(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_183(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 182, Param)   Separator()   _MODERN_ENUM_RECURSIVE_182(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_184(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 183, Param)   Separator()   _MODERN_ENUM_RECURSIVE_183(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_185(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 184, Param)   Separator()   _MODERN_ENUM_RECURSIVE_184(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_186(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 185, Param)   Separator()   _MODERN_ENUM_RECURSIVE_185(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_187(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 186, Param)   Separator()   _MODERN_ENUM_RECURSIVE_186(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_188(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 187, Param)   Separator()   _MODERN_ENUM_RECURSIVE_187(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_189(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 188, Param)   Separator()   _MODERN_ENUM_RECURSIVE_188(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_190(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 189, Param)   Separator()   _MODERN_ENUM_RECURSIVE_189(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_191(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 190, Param)   Separator()   _MODERN_ENUM_RECURSIVE_190(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_192(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 191, Param)   Separator()   _MODERN_ENUM_RECURSIVE_191(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_193(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 192, Param)   Separator()   _MODERN_ENUM_RECURSIVE_192(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_194(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 193, Param)   Separator()   _MODERN_ENUM_RECURSIVE_193(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_195(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 194, Param)   Separator()   _MODERN_ENUM_RECURSIVE_194(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_196(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 195, Param)   Separator()   _MODERN_ENUM_RECURSIVE_195(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_197(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 196, Param)   Separator()   _MODERN_ENUM_RECURSIVE_196(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_198(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 197, Param)   Separator()   _MODERN_ENUM_RECURSIVE_197(Macro, Separator, GlobalParam, __VA_ARGS__)
#define _MODERN_ENUM_RECURSIVE_199(Macro, Separator, GlobalParam, Param, ...)    Macro(GlobalParam, 198, Param)   Separator()   _MODERN_ENUM_RECURSIVE_198(Macro, Separator, GlobalParam, __VA_ARGS__)

#define _MODERN_ENUM_RECURSIVE_NAME(Count) _MODERN_ENUM_RECURSIVE_ ## Count
#define _MODERN_ENUM_RECURSIVE_MACRO(Count) _MODERN_ENUM_RECURSIVE_NAME(Count)
#define _MODERN_ENUM_RECURSIVE(Macro, Separator, GlobalParam, ...) _MODERN_ENUM_RECURSIVE_MACRO(_MODERN_ENUM_COUNT_ELEMENTS(__VA_ARGS__))(Macro, Separator, GlobalParam, __VA_ARGS__)


#define _MODERN_ENUM_SEPARATOR_COMMA() ,
#define _MODERN_ENUM_SEPARATOR_NONE()

#define _MODERN_ENUM_LIST_NAME(_0, _1, Element) #Element
#define _MODERN_ENUM_LIST_NAMES(...) _MODERN_ENUM_RECURSIVE(_MODERN_ENUM_LIST_NAME, _MODERN_ENUM_SEPARATOR_COMMA,, __VA_ARGS__)

#define _MODERN_ENUM_LIST_ELEMENT(_0, _1, Element) Element
#define _MODERN_ENUM_LIST_ELEMENTS(...) _MODERN_ENUM_RECURSIVE(_MODERN_ENUM_LIST_ELEMENT, _MODERN_ENUM_SEPARATOR_COMMA,, __VA_ARGS__)

#define _MODERN_ENUM_DECLARE_ELEMENT(Enum, _0, Element) static const Enum Element;
#define _MODERN_ENUM_DECLARE_ELEMENTS(Enum, ...) _MODERN_ENUM_RECURSIVE(_MODERN_ENUM_DECLARE_ELEMENT, _MODERN_ENUM_SEPARATOR_NONE, Enum, __VA_ARGS__)

#define _MODERN_ENUM_DEFINE_ELEMENT(Enum, Index, Element) inline constexpr Enum Enum::Element{Enum::s_count-Index-1};
#define _MODERN_ENUM_DEFINE_ELEMENTS(Enum, ...) _MODERN_ENUM_RECURSIVE(_MODERN_ENUM_DEFINE_ELEMENT, _MODERN_ENUM_SEPARATOR_NONE, Enum, __VA_ARGS__)


#if MODERN_ENUM_NATVIS_SUPPORT_ENABLED
#define _MODERN_ENUM_NATVIS_SUPPORT() \
        inline static const auto& s_nameRefForNatvis     = internal::modern_enum::ForceGenerationForNatvis(s_name);\
        inline static const auto& s_namesRefForNatvis    = internal::modern_enum::ForceGenerationForNatvis(s_names);
#else
#define _MODERN_ENUM_NATVIS_SUPPORT()
#endif


#define _MODERN_ENUM_PRIVATE_MEMBERS(Enum, ...)\
    private:\
        constexpr Enum(uint32_t idx)  : m_idx{idx} {}\
\
        enum class _InternalRawEnumToSupportSwitchStatement { _MODERN_ENUM_LIST_ELEMENTS(__VA_ARGS__) }; \
\
        constexpr static uint32_t                               s_count { _MODERN_ENUM_COUNT_ELEMENTS(__VA_ARGS__) };\
        constexpr static std::string_view                       s_name  { #Enum };\
        constexpr static std::array<std::string_view, s_count>  s_names { _MODERN_ENUM_LIST_NAMES(__VA_ARGS__) };\
\
        _MODERN_ENUM_NATVIS_SUPPORT();\
\
        using EnumArray = std::array<Enum, s_count>;\
        static const EnumArray s_elements;\
\
        internal::modern_enum::NatvisIndex<Enum> m_idx;\
\
    public:


#define _MODERN_SET_PRIVATE_MEMBERS(Enum, ...)\
    private:\
        friend class Enum;\
\
        using BitSet = internal::modern_enum::BitSet<s_count>;\
\
        constexpr Set(BitSet value) : m_value{value} {}\
\
        internal::modern_enum::NatvisBitSet<Enum, s_count> m_value;\
\
    public:


#define _MODERN_ENUM_AND_SET_EXTERNAL_DEFINITIONS(Enum, ...)\
\
    constexpr Enum::Enum() : m_idx{0} {};\
\
    constexpr uint32_t               Enum::GetCount() { return s_count; }\
    constexpr std::string_view       Enum::GetName()  { return s_name; }\
    constexpr const Enum::EnumArray& Enum::GetElements() { return s_elements; }\
\
    constexpr Enum Enum::FromString(std::string_view name) { return internal::modern_enum::CreateFromString<s_count>(s_name, name, s_names);} \
    constexpr Enum Enum::FromIndex(uint32_t index) { return index; } \
\
    constexpr std::string_view Enum::toString() const { return s_names[m_idx.idx]; } \
\
    constexpr uint32_t Enum::index() const { return m_idx.idx; }\
\
    constexpr bool Enum::operator == (Enum o) const { return m_idx.idx == o.m_idx.idx; }\
    constexpr bool Enum::operator != (Enum o) const { return !(*this == o); }\
\
    constexpr Enum::operator Enum::Set() const { return Set{m_idx.idx}; }\
\
    constexpr Enum::operator Enum::_InternalRawEnumToSupportSwitchStatement() const { return static_cast<_InternalRawEnumToSupportSwitchStatement>(m_idx.idx); }\
\
    constexpr Enum::Set::Set() : Set{{}} {}\
\
    constexpr uint32_t Enum::Set::count() const { return m_value.set.count(); } \
\
    constexpr bool Enum::Set::all()     const { return m_value.set.all(); } \
    constexpr bool Enum::Set::none()    const { return m_value.set.none(); }\
    constexpr bool Enum::Set::any()     const { return m_value.set.any(); }\
\
    constexpr bool Enum::Set::contains(Set o) const { return (*this & o) == o; }\
\
    constexpr bool operator == (Enum::Set a, Enum::Set b)  { return a.m_value.set == b.m_value.set; }\
    constexpr bool operator != (Enum::Set a, Enum::Set b)  { return a.m_value.set != b.m_value.set;; }\
\
    constexpr Enum::Set operator |  (Enum::Set a, Enum::Set b) { return a.m_value.set | b.m_value.set; }\
    constexpr Enum::Set operator &  (Enum::Set a, Enum::Set b) { return a.m_value.set & b.m_value.set; }\
    constexpr Enum::Set operator ^  (Enum::Set a, Enum::Set b) { return a.m_value.set ^ b.m_value.set; }\
    constexpr Enum::Set operator ~  (Enum::Set a)              { return ~a.m_value.set; }\
\
    constexpr inline Enum::Set& Enum::Set::operator |= (Enum::Set o) { *this = *this | o; return *this; }\
    constexpr inline Enum::Set& Enum::Set::operator &= (Enum::Set o) { *this = *this & o; return *this; }\
    constexpr inline Enum::Set& Enum::Set::operator ^= (Enum::Set o) { *this = *this ^ o; return *this; }\
\
    _MODERN_ENUM_DEFINE_ELEMENTS(Enum, __VA_ARGS__);\
\
    inline constexpr Enum::EnumArray Enum::s_elements {_MODERN_ENUM_LIST_ELEMENTS(__VA_ARGS__)};


