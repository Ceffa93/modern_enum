#include "modern_enum.h"
#include <iostream>

MODERN_ENUM(Week, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday);

int main()
{
    std::cout << "Name of enum class: " << Week::GetName();
    std::cout << "\nNumber of days: " << Week::GetCount();

    constexpr Week monday = Week::Monday;
    constexpr Week tuesday = Week::FromIndex(1);
    constexpr Week wednesday = Week::FromString("Wednesday");

    std::cout << "\nTuesday's name: " << tuesday.toString();
    std::cout << "\nTuesday's index: " << tuesday.index();

    std::cout << "\nDays list:";
    for (Week m : Week::GetElements()) {
        std::cout << " " << m.toString();
    }

    std::cout << "\nOn Tuesday I ";
    switch (tuesday) {
    case Week::Monday:
    case Week::Tuesday:
    case Week::Wednesday:
    case Week::Thursday:
    case Week::Friday:
        std::cout << "work"; 
        break;
    case Week::Saturday:
    case Week::Sunday:
        std::cout << "rest"; 
        break;
    }

    constexpr Week::Set weekend = Week::Saturday | Week::Sunday;
    constexpr Week::Set weekdays = ~weekend;
    Week::Set gymDays = Week::Tuesday | Week::Friday;

    std::cout << "\nHow many weekdays there are? " << weekdays.count();
    std::cout << "\nIs Tuesday a weekend day? " << (weekend.contains(Week::Tuesday) ? "Yes" : "No");
    std::cout << "\nDo I train over the weekend? " << ((weekend & gymDays).any() ? "Yes" : "No");

    gymDays ^= Week::Saturday;

    std::cout << "\nAnd now, do I train over the weekend? " << ((weekend & gymDays).any() ? "Yes" : "No");

    std::cout << std::endl;

    return 0;
}