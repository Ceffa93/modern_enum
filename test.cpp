#include "modern_enum.h"

// This files contains unit tests for the modern enum library.
// The library is fully constexpr, so all unit tests can be run at compile time.
// No need to run a test executable - if this file compiles, all tests passed.
namespace modern_enum::test
{
    MODERN_ENUM(Color, Red, Green, Blue);

    constexpr Color red = Color::Red;
    constexpr Color::Set redSet = Color::Red;
    constexpr auto magentaSet = Color::Red | Color::Blue;

    void StaticGetters_CompileTimeTest()
    {
        static_assert(Color::GetCount() == 3);
        static_assert(Color::GetName() == "Color");
        static_assert(Color::GetElements()[1] == Color::Green);
        static_assert(Color::GetElements().size() == Color::GetCount());
    }

    void DefaultContructor_CompileTimeTest()
    {
        static_assert(Color{} == Color::Red);
        static_assert(Color::Set{}.none());
    }

    void Assignmenet_CompileTimeTest()
    {
        Color::Set s = Color::Red;
        s = Color::Green;
        Color c = Color::Red;
        c = Color::Green;
    }

    void StatusCheck_CompileTimeTest()
    {
        constexpr auto c = magentaSet.count();
        static_assert(magentaSet.count() == 2);
        static_assert(magentaSet.all() == false);
        static_assert(magentaSet.none() == false);
        static_assert(magentaSet.any() == true);
    }

    void EqualityOperators_CompileTimeTest()
    {
        static_assert(Color::Red == red);
        static_assert(redSet != Color::Green);
        static_assert(Color::Red != magentaSet);
        static_assert(magentaSet == magentaSet);
    }

    void BooleanOperators_CompileTimeTest()
    {
        static_assert((red ^ magentaSet) == Color::Blue);
        static_assert((red & magentaSet) == Color::Red);
        static_assert((magentaSet ^ magentaSet).none() == true);
        static_assert((magentaSet | Color::Green).all() == true);
    }

    void NegateOperator_CompileTimeTest()
    {
        static_assert(~Color::Green == magentaSet);
        static_assert(~magentaSet == Color::Green);
        static_assert(~~magentaSet == magentaSet);
    }

    void Contains_CompileTimeTest()
    {
        static_assert(magentaSet.contains(redSet) == true);
        static_assert(magentaSet.contains(Color::Green) == false);
    }

    void String_CompileTimeTest()
    {
        static_assert(Color::FromString("Green") == Color::Green);
        static_assert(Color::Green.toString() == "Green");
    }

    void Index_CompileTimeTest()
    {
        static_assert(Color::Green.index() == 1);
        static_assert(Color::Green == Color::FromIndex(1));
    }

    void Switch_CompileTimeTest()
    {
        switch (red)
        {
            case Color::Blue: break;
            default: break;
        }
    }


    MODERN_ENUM(ShortEnum, X);


    MODERN_ENUM(
        LongEnum,
        e000, e001, e002, e003, e004, e005, e006, e007, e008, e009,
        e010, e011, e012, e013, e014, e015, e016, e017, e018, e019,
        e020, e021, e022, e023, e024, e025, e026, e027, e028, e029,
        e030, e031, e032, e033, e034, e035, e036, e037, e038, e039,
        e040, e041, e042, e043, e044, e045, e046, e047, e048, e049,
        e050, e051, e052, e053, e054, e055, e056, e057, e058, e059,
        e060, e061, e062, e063, e064, e065, e066, e067, e068, e069,
        e070, e071, e072, e073, e074, e075, e076, e077, e078, e079,
        e080, e081, e082, e083, e084, e085, e086, e087, e088, e089,
        e090, e091, e092, e093, e094, e095, e096, e097, e098, e099,
        e100, e101, e102, e103, e104, e105, e106, e107, e108, e109,
        e110, e111, e112, e113, e114, e115, e116, e117, e118, e119,
        e120, e121, e122, e123, e124, e125, e126, e127, e128, e129,
        e130, e131, e132, e133, e134, e135, e136, e137, e138, e139,
        e140, e141, e142, e143, e144, e145, e146, e147, e148, e149,
        e150, e151, e152, e153, e154, e155, e156, e157, e158, e159,
        e160, e161, e162, e163, e164, e165, e166, e167, e168, e169,
        e170, e171, e172, e173, e174, e175, e176, e177, e178, e179,
        e180, e181, e182, e183, e184, e185, e186, e187, e188, e189,
        e190, e191, e192, e193, e194, e195, e196, e197, e198);

    void LongEnum_CompileTimeTest()
    {
        static_assert(LongEnum::GetCount() == 199);
        constexpr LongEnum::Set e63_64_65 = LongEnum::e063 | LongEnum::e064 | LongEnum::e065;
        static_assert(e63_64_65.contains(LongEnum::e063) );
        static_assert(e63_64_65.count() == 3);
        static_assert((~e63_64_65).count() == 196);
        static_assert(e63_64_65.contains(LongEnum::e063 | LongEnum::e065) );

    }

    constexpr Color::Set makeBlueWithBitwiseAnd() {
        Color::Set c = magentaSet;
        c &= Color::Blue;
        return c;
    }
    constexpr Color::Set makeMagentaWithBitwiseOr() {
        Color::Set c = Color::Red;
        c |= Color::Blue;
        return c;
    }
    constexpr Color::Set makeRedWithBitwiseXor() {
        Color::Set c = Color::Blue;
        c ^= magentaSet;
        return c;
    }

    void BitwiseOperators_CompileTimeTest()
    {
        static_assert(makeBlueWithBitwiseAnd() == Color::Blue);
        static_assert(makeMagentaWithBitwiseOr() == magentaSet);
        static_assert(makeRedWithBitwiseXor() == Color::Red);
    }
}
