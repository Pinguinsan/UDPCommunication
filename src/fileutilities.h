/***********************************************************************
*    fileutilities.h:                                                  *
*    Namespace FileUtilities, for file/argv IO abstraction             *
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.com/tlewiscpp/tjlutils                            *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the declarations of a FileUtilities namespace     *
*    It is used to abstract file/argv IO into high level functions     *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#ifndef TJLUTILS_FILEUTILITIES_H
#define TJLUTILS_FILEUTILITIES_H

#if defined(_WIN32) && defined(_MSC_VER)
    #if _MSC_VER < 1700
        #error This library needs at least a C++11 compliant compiler
    #endif
#else
    #if __cplusplus <= 199711L
        #error This library needs at least a C++11 compliant compiler
    #endif
#endif

#include <string>
#include <list>
#include <vector>
#include <algorithm>

namespace FileUtilities
{
    std::string getCurrentDirectory();
    
    std::vector<std::string> getFileListAsVector(const std::string &directory, const std::string &mask = "", bool caseSensitive = true);
    std::vector<std::string> getFileListAsVector(const char *directory, const char *mask = "", bool caseSensitive = true);
    std::vector<std::string> getFileListAsVector(const char *directory, const std::string &mask = "", bool caseSensitive = true);    
    
    std::vector<std::string> getDirectoryListAsVector(const std::string &directory, const std::string &mask = "", bool caseSensitive = true);
    std::vector<std::string> getDirectoryListAsVector(const char *directory, const char *mask = "", bool caseSensitive = true);
    std::vector<std::string> getDirectoryListAsVector(const char *directory, const std::string &mask = "", bool caseSensitive = true);
    
    std::list<std::string> getFileList(const std::string &directory, const std::string &mask = "", bool caseSensitive = true);
    std::list<std::string> getFileList(const char *directory, const char *mask = "", bool caseSensitive = true);
    std::list<std::string> getFileList(const char *directory, const std::string &mask = "", bool caseSensitive = true);
    
    std::list<std::string> getDirectoryList(const std::string &directory, const std::string &mask = "", bool caseSensitive = true);
    std::list<std::string> getDirectoryList(const char *directory, const char *mask = "", bool caseSensitive = true);
    std::list<std::string> getDirectoryList(const char *directory, const std::string &mask = "", bool caseSensitive = true);
    
    std::vector<std::string> getPathDirectoriesAsVector();
    std::list<std::string> getPathDirectories();

    bool directoryExists(const std::string &directoryToCheck);
    bool directoryExists(const char *directoryToCheck);
    bool fileExists(const std::string &fileToCheck);
    bool fileExists(const char *fileToCheck);
    unsigned int getInstanceCount(const char *str);
    unsigned int getInstanceCount(const std::string &str);
    std::vector<std::string> parseArgsToVector(int argcIn,char **argvIn);
    std::list<std::string> parseArgsToList(int argcIn, char **argvIn);
}

#endif //TJLUTILS_FILEUTILITIES_H
