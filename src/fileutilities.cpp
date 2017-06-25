/***********************************************************************
*    fileutilities.cpp:                                                *
*    Namespace FileUtilities, for file/argv IO abstraction             *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.com/tlewiscpp/tjlutils                            *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the implementation of a FileUtilities namespace   *
*    It is used to abstract file/argv IO into high level functions     *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/


#include <fstream>
#include <algorithm>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <exception>
#include <climits>

#if defined(_WIN32) && !defined(__CYGWIN__)
    #include <Windows.h>
    #include "Shlwapi.h"
    #include <direct.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
	#include <dirent.h>
    #include <unistd.h>
#endif

#include "fileutilities.h"

namespace FileUtilitiesUtilities
{
    template <typename Container, typename InputIter>
    Container parseToContainer(InputIter first, InputIter last, typename std::remove_reference<decltype(*first)>::type delimiter)
    {
        Container returnContainer;
        InputIter it;
        do {
            it = std::find(first, last, delimiter);
            typename Container::value_type tempContainer;
            std::copy(first, it, std::inserter(tempContainer, tempContainer.end()));
            if (!tempContainer.empty()) {
                returnContainer.insert(returnContainer.end(), tempContainer);
            }
            first = it+1;
        } while (it != last);
        return returnContainer;
    }
}

using namespace FileUtilitiesUtilities;

namespace FileUtilities
{

    std::vector<std::string> getFileListAsVector(const char *directory, const char *mask, bool caseSensitive)
    {
        #if defined(_WIN32) && !defined(__CYGWIN__)
            return std::vector<std::string>{};
        #else
        if (!directoryExists(directory)) {
            return std::vector<std::string>{};
        }
        std::vector<std::string> returnVector;
        DIR *targetDirectory;
        struct dirent *entity;
        if ((targetDirectory = opendir(directory)) != nullptr) {
            while ((entity = readdir(targetDirectory)) != nullptr) {
                if (!directoryExists(entity->d_name)) {
                    std::string maskCopy{static_cast<std::string>(mask)};
                    std::string nameCopy{static_cast<std::string>(entity->d_name)};
                    if (maskCopy != "") {
                        if (!caseSensitive) {
                            std::transform(maskCopy.begin(), maskCopy.end(), maskCopy.begin(), ::tolower);
                            std::transform(nameCopy.begin(), nameCopy.end(), nameCopy.begin(), ::tolower);
                        }
                        if (nameCopy.find(maskCopy) != std::string::npos) {
                            returnVector.emplace_back(entity->d_name);
                        }
                    } else { 
                        returnVector.emplace_back(entity->d_name);
                    }
                }
            }
            closedir (targetDirectory);
        } else {
            return std::vector<std::string>{};
        }
        return returnVector;
        #endif
    }

    std::vector<std::string> getDirectoryListAsVector(const char *directory, const char *mask, bool caseSensitive)
    {

        #if defined(_WIN32) && !defined(__CYGWIN__)
            return std::vector<std::string>{};
        #else
        if (!directoryExists(directory)) {
            return std::vector<std::string>{};
        }
        std::vector<std::string> returnVector;
        DIR *targetDirectory;
        struct dirent *entity;
        if ((targetDirectory = opendir(directory)) != nullptr) {
            while ((entity = readdir(targetDirectory)) != nullptr) {
                if (directoryExists(entity->d_name)) {
                    std::string maskCopy{static_cast<std::string>(mask)};
                    std::string nameCopy{static_cast<std::string>(entity->d_name)};
                    if (maskCopy != "") {
                        if (!caseSensitive) {
                            std::transform(maskCopy.begin(), maskCopy.end(), maskCopy.begin(), ::tolower);
                            std::transform(nameCopy.begin(), nameCopy.end(), nameCopy.begin(), ::tolower);
                        }
                        if (nameCopy.find(maskCopy) != std::string::npos) {
                            returnVector.emplace_back(entity->d_name);
                        }
                    } else { 
                        returnVector.emplace_back(entity->d_name);
                    }
                }
            }
            closedir (targetDirectory);
        } else {
            return std::vector<std::string>{};
        }
        return returnVector;
        #endif
    }

    std::vector<std::string> getPathDirectoriesAsVector()
    {
        std::string temp{static_cast<std::string>(getenv("PATH"))};
        return parseToContainer<std::vector<std::string>>(temp.begin(), temp.end(), ':');
    }

    std::list<std::string> getPathDirectories()
    {
        std::list<std::string> returnList;
        for (auto &it : FileUtilities::getPathDirectoriesAsVector()) {
            returnList.emplace_back(it);
        }
        returnList.sort();
        return returnList;
    }

    std::vector<std::string> getFileListAsVector(const std::string &directory, const std::string &mask, bool caseSensitive)
    {
        return getFileListAsVector(directory.c_str(), mask.c_str(), caseSensitive);
    }

    std::vector<std::string> getFileListAsVector(const char *directory, const std::string &mask, bool caseSensitive)
    {
        return getFileListAsVector(directory, mask.c_str(), caseSensitive);
    }

    std::vector<std::string> getDirectoryListAsVector(const std::string &directory, const std::string &mask, bool caseSensitive)
    {
        return getDirectoryListAsVector(directory.c_str(), mask.c_str(), caseSensitive);
    }

    std::vector<std::string> getDirectoryListAsVector(const char *directory, const std::string &mask, bool caseSensitive)
    {
        return getDirectoryListAsVector(directory, mask.c_str(), caseSensitive);
    }

    std::list<std::string> getFileList(const char *directory, const char *mask, bool caseSensitive)
    {
        std::list<std::string> returnList;
        for (auto &it : FileUtilities::getFileListAsVector(directory, mask, caseSensitive)) {
            returnList.emplace_back(it);
        }
        returnList.sort();
        return returnList;
    }

    std::list<std::string> getDirectoryList(const char *directory, const char *mask, bool caseSensitive)
    {
        std::list<std::string> returnList;
        for (auto &it : FileUtilities::getDirectoryListAsVector(directory, mask, caseSensitive)) {
            returnList.emplace_back(it);
        }
        returnList.sort();
        return returnList;
    }

    std::list<std::string> getFileList(const std::string &directory, const std::string &mask, bool caseSensitive)
    {
        return getFileList(directory.c_str(), mask.c_str(), caseSensitive);
    }

    std::list<std::string> getFileList(const char *directory, const std::string &mask, bool caseSensitive)
    {
        return getFileList(directory, mask.c_str(), caseSensitive);
    }

    std::list<std::string> getDirectoryList(const std::string &directory, const std::string &mask, bool caseSensitive)
    {
        return getDirectoryList(directory.c_str(), mask.c_str(), caseSensitive);
    }

    std::list<std::string> getDirectoryList(const char *directory, const std::string &mask, bool caseSensitive)
    {
        return getDirectoryList(directory, mask.c_str(), caseSensitive);
    }

    std::string getCurrentDirectory()
    {
        #if defined(_WIN32) || defined (__MINGW32__)
            char* buffer{_getcwd(NULL, 0)};
            if (!buffer) {
                throw std::runtime_error("Could not allocate memory for buffer to determine current working directory");
            } else {
                return std::string{buffer};
            }
        #else
            char *currentWorkingDirectory;
            char filePathBuffer[PATH_MAX + 1];

            currentWorkingDirectory = getcwd(filePathBuffer, PATH_MAX + 1 );
            //Check if cwd is null
            if (currentWorkingDirectory) {
                return std::string{currentWorkingDirectory};
            } else {
                throw std::runtime_error("Could not allocate memory for buffer to determine current working directory");
            }
        #endif
    }

    bool directoryExists(const std::string &directoryToCheck)
    { 
        #if defined(_WIN32) && !defined(__CYGWIN__)
            DWORD dwAttrib{GetFileAttributes(directoryToCheck.c_str())};
            return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
        #else
            struct stat info;
            if(stat(directoryToCheck.c_str(), &info) != 0) {
                return false;
            } else if(info.st_mode & S_IFDIR)  { 
                return true;
            } else {
                return false;
            }

        #endif
    }

    bool directoryExists(const char *directoryToCheck)
    {
        return directoryExists(static_cast<std::string>(directoryToCheck));
    }

    bool fileExists(const std::string &fileToCheck)
    {
        #if defined(_WIN32)
            return (PathFileExists(directoryToCheck.c_str()) == 1);
            /*
            std::ifstream readFromFile;
            readFromFile.open(fileToCheck);
            if (readFromFile.good()) {
                readFromFile.close();
                return true;
            } else {
                return false;
            }
            */
        #else
            return (access(fileToCheck.c_str(),F_OK) != -1);
        #endif
    }

    bool fileExists(const char *fileToCheck)
    {
        return fileExists(static_cast<std::string>(fileToCheck));
    }

    
    unsigned int getInstanceCount(const char *str)
    {
        return getInstanceCount(static_cast<std::string>(str));
    }

    unsigned int getInstanceCount(const std::string &str)
    {
        //std::unique_ptr<SystemCommand> systemCommand{new SystemCommand()};
        //systemCommand->setCommand("ps aux | grep " + str);
        //systemCommand->execute();
        //if (systemCommand->hasError()) {
        //    return 0;
        //} else {
        //    return systemCommand->outputAsVector().size()-2;
        //}
        return 0;
    }

    std::vector<std::string> parseArgsToVector(int argcIn,char **argvIn) 
    {
        if (argcIn == 0) {
            throw std::logic_error("argc can never be zero due to the c++ standard, but zero was passed as an argument to parseArgsToList variant");
        }
        std::vector<std::string> returnVector;
        //No need to check, c++ standard ensures argv[0..argc-1] is not null
        for (int i = 0; i < argcIn; i++) {
            returnVector.push_back(argvIn[i]);
        }
        return returnVector;
    }

    std::list<std::string> parseArgsToList(int argcIn, char **argvIn)
    {
        if (argcIn == 0) {
            throw std::logic_error("argc can never be zero due to the c++ standard, but zero was passed as an argument to parseArgsToList variant");
        }
        std::list<std::string> returnList;
        //No need to check, c++ standard ensures argv[0..argc-1] is not null
        for (int i = 0; i < argcIn; i++) {
            returnList.push_back(argvIn[i]);
        }
        return returnList;
    }
}
