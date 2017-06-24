#include <iostream>
#include <string>
#include <vector>
#include <generalutilities.h>

int main()
{
    namespace gu = GeneralUtilities;
    std::vector<std::string> strVec{"{dwrite:2:0:1}", "{dwrite:2:0:1}{dread:2:0}", "{dwrite:2:0:1}{dread:2:0}{dwrite:2:0:1}"};
    for (auto &str : strVec) {
        std::cout << "str = " << str << ", getBetween(\"{\", \"}\") = " << gu::getBetween("{", "}", str) << std::endl;
        
        std::cout << "str = " << str << ", getAllBetween(\"{\", \"}\") = ";
        auto item = gu::getAllBetween("{", "}", str);
        for (auto it = item.begin(); it != item.end(); it++) {
            std::cout << *it;
            if (it+1 != item.end()) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
      
        std::cout << "str = " << str << ", stripBetween(\"{\", \"}\") = " << gu::stripBetween("{", "}", str) << std::endl;
        std::cout << "str = " << str << ", stripAllBetween(\"{\", \"}\") = " << gu::stripAllBetween("{", "}", str) << std::endl;
        std::cout << std::endl;
    }
    return 0;
}
