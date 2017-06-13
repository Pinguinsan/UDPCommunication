#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <functional>
#include <cctype>
#include <generalutilities.h>

bool isTargetChar(char checkChar)
{
    return ((std::isalnum(checkChar) || std::ispunct(checkChar)) && (checkChar != '*'));
}

class TargetChar
{
public:
    bool operator()(char checkChar)
    {
        return ((std::isalnum(checkChar) || std::ispunct(checkChar)) && (checkChar != '*'));
    }
};

template<class T>
struct is_iterator
{   
    static T makeT();
    typedef void * twoptrs[2];  // sizeof(twoptrs) > sizeof(void *)
    static twoptrs & test(...); // Common case
    template<class R> static typename R::iterator_category * test(R); // Iterator
    template<class R> static void * test(R *); // Pointer

    static const bool value = sizeof(test(makeT())) == sizeof(void *); 
};

const size_t generalnpos = std::string::npos;

template <typename BeginningIterator, typename EndingIterator, typename Function>
static size_t findLastOfIf(const BeginningIterator &bit, const EndingIterator &eit, Function function)
{
    size_t foundPosition{std::string::npos};
    for (auto iter = bit; iter != eit; iter++) {
        if (function(*iter)) {
            foundPosition = std::distance(bit, iter);
        }
    }
    return foundPosition;
}

std::string whitespace(unsigned int howMuch) 
{
    std::string returnString{""};
    for (unsigned int i = 0; i < howMuch; i++) {
        returnString += " ";
    }
    return returnString;
}

int main()
{
    std::vector<std::string> str{ "loldjhvjrbb     nren         *", "******************************", "lmao ...!!!     *"};
    for (std::vector<std::string>::const_iterator iter = str.begin(); iter != str.end(); iter++) {
        size_t found = findLastOfIf(std::begin(*iter), std::end(*iter), isTargetChar);
        if (found != generalnpos) {
            std::cout << "Last found position for " << std::quoted(*iter) << ": " << found << std::endl; 
            std::cout << *iter << std::endl;
            std::cout << whitespace(found) << "^ here" << std::endl << std::endl;
        } else {
            std::cout << "No found position for " << std::quoted(*iter) << std::endl;
        }
    }
    
    std::cout << std::endl << std::endl;
    std::cout << "***************************************" << std::endl << std::endl;
    for (std::vector<std::string>::const_iterator iter = str.begin(); iter != str.end(); iter++) {
        TargetChar tc;
        size_t found = findLastOfIf(std::begin(*iter), std::end(*iter), tc);
        if (found != generalnpos) {
            std::cout << "Last found position for " << std::quoted(*iter) << ": " << found << std::endl; 
            std::cout << *iter << std::endl;
            std::cout << whitespace(found) << "^ here" << std::endl << std::endl;
        } else {
            std::cout << "No found position for " << std::quoted(*iter) << std::endl;
        }
    }

    std::cout << std::endl << std::endl;
    std::cout << "***************************************" << std::endl << std::endl;
    std::function<bool (char) > tcf = isTargetChar;
    for (std::vector<std::string>::const_iterator iter = str.begin(); iter != str.end(); iter++) {
        size_t found = findLastOfIf(std::begin(*iter), std::end(*iter), tcf);
        if (found != generalnpos) {
            std::cout << "Last found position for " << std::quoted(*iter) << ": " << found << std::endl; 
            std::cout << *iter << std::endl;
            std::cout << whitespace(found) << "^ here" << std::endl << std::endl;
        } else {
            std::cout << "No found position for " << std::quoted(*iter) << std::endl;
        }
    }

    std::cout << std::endl << std::endl;
    std::cout << "***************************************" << std::endl << std::endl;
    for (std::vector<std::string>::const_iterator iter = str.begin(); iter != str.end(); iter++) {
        size_t found = findLastOfIf(std::begin(*iter), std::end(*iter), [](char checkChar) {
                                                                            return ((std::isalnum(checkChar) || std::ispunct(checkChar)) && (checkChar != '*'));
                                                                        });
        if (found != generalnpos) {
            std::cout << "Last found position for " << std::quoted(*iter) << ": " << found << std::endl; 
            std::cout << *iter << std::endl;
            std::cout << whitespace(found) << "^ here" << std::endl << std::endl;
        } else {
            std::cout << "No found position for " << std::quoted(*iter) << std::endl;
        }
    }
    if(GeneralUtilities::endsWith("hello", 'o')) {
        std::cout << std::endl;
    }

}
