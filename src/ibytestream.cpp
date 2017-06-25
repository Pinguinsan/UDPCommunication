/***********************************************************************
*    ibytestream.cpp:                                                  *
*    IByteStream, base class for simple read and write operations      *
*    Copyright (c) 2017 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.serial/tlewiscpp/tjlutils                          *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the implementaiton for the methods used in        *
*    IByteStreamScriptReader and IByteStreamScriptExecutor             * 
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include <fstream>
#include <algorithm>
#include "ibytestream.h"

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

namespace IByteStreamUtilities {

    std::string trimWhitespaceFromBeginning(const std::string &str)
    {
        if (str.length() == 0) {
            return str;
        }
        std::string copyString{str};
        while ((copyString.length() != 0) && (copyString[0] == ' ')) {
            copyString = copyString.substr(1);
        }
        return copyString;
    }

    std::string trimWhitespaceFromEnd(const std::string &str)
    {
        if (str.length() == 0) {
            return str;
        }
        std::string copyString{str};
        while ((copyString.length() != 0) && (copyString[copyString.length()-1] == ' ')) {
            copyString = copyString.substr(0, copyString.length()-1);
        }
        return copyString;
    }

    std::string trimWhitespace(const std::string &str)
    {
        if (str.length() == 0) {
            return str;
        }
        std::string copyString{str};
        copyString = trimWhitespaceFromBeginning(copyString);
        copyString = trimWhitespaceFromEnd(copyString);
        return copyString;
    }

    std::vector<std::string> trimWhitespaceFromBeginning (const std::vector<std::string> &vec)
    {
        std::vector<std::string> returnVec{};
        for (auto &it : vec) {
            returnVec.push_back(trimWhitespaceFromBeginning(it));
        }
        return returnVec;
    }

    std::vector<std::string> trimWhitespaceFromEnd (const std::vector<std::string> &vec)
    {
        std::vector<std::string> returnVec{};
        for (auto &it : vec) {
            returnVec.push_back(trimWhitespaceFromEnd(it));
        }
        return returnVec;
    }

    std::vector<std::string> trimWhitespace (const std::vector<std::string> &vec)
    {
        std::vector<std::string> returnVec{};
        for (auto &it : vec) {
            returnVec.push_back(trimWhitespace(it));
        }
        return returnVec;
    }


    bool isWhitespace(const std::string &stringToCheck)
    {
        return std::all_of(stringToCheck.begin(), stringToCheck.end(), [](char c) { return c == ' '; });
    }

    bool isWhitespace(char charToCheck)
    {
        return isWhitespace(std::string(1, charToCheck));
    }

    std::string stripFromString(const std::string &stringToStrip, const std::string &whatToStrip)
    {
        std::string returnString{stringToStrip};
        if (returnString.find(whatToStrip) == std::string::npos) {
            return returnString;
        }
        size_t foundPosition{stringToStrip.find(whatToStrip)};
        if (foundPosition == 0) {
            returnString = returnString.substr(whatToStrip.length());
        } else if (foundPosition == (returnString.length() - whatToStrip.length())) {
            returnString = returnString.substr(0, foundPosition);
        } else {
            returnString = returnString.substr(0, foundPosition) + returnString.substr(foundPosition+whatToStrip.length());
        }
        return returnString;
    }

    std::string stripFromString(const std::string &stringToStrip, char whatToStrip)
    {
        return stripFromString(stringToStrip, std::string(1, whatToStrip));
    }

    std::string stripAllFromString(const std::string &stringToStrip, const std::string &whatToStrip)
    {
        std::string returnString = stringToStrip;
        if (returnString.find(whatToStrip) == std::string::npos) {
            return returnString;
        }
        while (returnString.find(whatToStrip) != std::string::npos) {
            returnString = stripFromString(returnString, whatToStrip);
        }
        return returnString;
    }

    std::string stripAllFromString(const std::string &stringToStrip, char whatToStrip)
    {
        return stripAllFromString(stringToStrip, std::string(1, whatToStrip));
    }


    std::string getBetween(const std::string &beginning, const std::string &ending, const std::string &findString)
    {
        size_t foundPosition{0};
        size_t foundEndPosition{0};
        if (beginning == ending) {
            size_t tempFound{findString.find(beginning)};
            foundPosition = findString.find(beginning);
            foundEndPosition = foundPosition + (findString.substr(tempFound+1).find(ending));
        } else {
            foundPosition = findString.find(beginning);
            foundEndPosition = findString.find(ending);
        }
        if (foundPosition == std::string::npos) {
            if (ending.length() == 0) {
                return findString;
            } else {
                return "";
            }
        } else if (foundEndPosition == std::string::npos) {
            if (ending.length() == 0) {
                return findString.substr(foundPosition+1);
            } else {
                return "";
            }
        } else {
            return findString.substr(foundPosition+1, foundEndPosition-foundPosition-1);
        }
    }

    std::vector<std::string> getAllBetween(const std::string &beginning, const std::string &ending, const std::string &findString)
    {
        std::vector<std::string> returnVector{};
        std::string copyString{findString};
        do {
            std::string gotString{getBetween(beginning, ending, copyString)};
            if (gotString.length() != 0) {
                returnVector.push_back(gotString);
            }
            copyString = copyString.substr(copyString.find(ending)+1);
        } while (copyString != "");
        return returnVector;
    }

    std::string stripBetween(const std::string &beginning, const std::string &ending, const std::string &findString)
    {
        std::string removeString{getBetween(beginning, ending, findString)};
        if (removeString == "") {
            return "";
        }
        size_t foundPosition{findString.find(removeString)};
        long long unsigned int offsetPosition{foundPosition + (removeString.length())};
        return (findString.substr(0, foundPosition) + findString.substr(offsetPosition));
    }

    std::string stripAllBetween(const std::string &beginning, const std::string &ending, const std::string &findString)
    {
        std::string returnString{""};
        std::string copyString{findString};
        while (getBetween(beginning, ending, copyString).length() != 0) {
            std::string gotString{beginning + getBetween(beginning, ending, copyString) + ending};
            returnString += (beginning + ending);
            copyString = stripFromString(copyString, gotString);
        }
        return returnString;
    }

    std::string tWhitespace(int howMuch)
    {
        std::string returnString{""};
        for (int i = 0; i < howMuch; i++) {
            returnString += " ";
        }
        return returnString;
    }
}

using namespace IByteStreamUtilities;

IByteStreamScriptReader::IByteStreamScriptReader(const std::string &scriptFilePath) :
    m_scriptFilePath{scriptFilePath},
    m_commands{std::make_shared<std::vector<IByteStreamCommand>>()}
{
    if (!fileExists(this->m_scriptFilePath)) {
        throw std::runtime_error(SCRIPT_FILE_DOES_NOT_EXISTS_STRING + tQuoted(this->m_scriptFilePath));
    }
    std::vector<std::string> buffer{};
    std::ifstream readFromFile{};
    readFromFile.open(this->m_scriptFilePath);
    if (readFromFile.is_open()) {
        std::string tempString{""};
        while (std::getline(readFromFile, tempString)) {
            buffer.emplace_back(tempString);
        }
        readFromFile.close();
    } else {
        throw std::runtime_error(UNABLE_TO_OPEN_SCRIPT_FILE_STRING + tQuoted(this->m_scriptFilePath));
    }
    buffer = trimWhitespace(buffer);
    int loops{0};
    int loopCount{0};
    for (std::vector<std::string>::const_iterator iter = buffer.begin(); iter != buffer.end(); iter++) {
        try {
            std::string copyString{*iter};
            std::transform(copyString.begin(), copyString.end(), copyString.begin(), ::tolower);
            //TODO: Replace with regex for searching
            size_t foundDelayPosition{copyString.find(DELAY_IDENTIFIER)};
            size_t foundWritePosition{copyString.find(WRITE_IDENTIFIER)};
            size_t foundReadPosition{copyString.find(READ_IDENTIFIER)};
            size_t foundFlushPosition{copyString.find(FLUSH_IDENTIFIER)};
            size_t foundLoopPosition{copyString.find(LOOP_IDENTIFIER)};
            size_t foundClosingLoopPosition{copyString.find(CLOSING_LOOP_IDENTIFIER)};
            if (copyString.length() != 0) {
                std::string otherCopy{copyString};
                unsigned int numberOfWhitespace{0};
                while (otherCopy.length() > 1 && isWhitespace(otherCopy[0])) {
                    stripFromString(otherCopy, ' ');
                    numberOfWhitespace++;
                }
                if (copyString.length() > numberOfWhitespace) {
                    if (copyString[numberOfWhitespace] == '#') {
                        continue;
                    }
                }
            } else {
                continue;
            }
            if (isWhitespace(copyString)) {
                continue;
            }

            long int currentLine{std::distance<std::vector<std::string>::const_iterator>(buffer.begin(), iter)+1};
            if (foundLoopPosition != std::string::npos) {
                std::string targetLoopCount{0};
                if (copyString.find(")") == std::string::npos) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << NO_CLOSING_PARENTHESIS_FOUND_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                }
                targetLoopCount = getBetween("(", ")", copyString);
                if (trimWhitespace(targetLoopCount) == "") {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << NO_LOOP_COUNT_SPECIFIED_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                } else {
                    try {
                        int temp{std::stoi(targetLoopCount)};
                        loopCount = temp > 0 ? temp : temp*-1;
                        loops++;
                        this->m_commands->emplace_back(IByteStreamCommandType::LOOP_START, toStdString(loopCount));
                    } catch (std::exception &e) {
                        std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                        std::cout << LOOP_COUNT_PARAMETER_NOT_AN_INTEGER_STRING << std::endl;
                        std::cout << *iter << std::endl;
                        std::cout << tWhitespace(iter->find(")")-1) << EXPECTED_HERE_STRING << std::endl;
                    }
                }
            } else if ((foundClosingLoopPosition != std::string::npos) && (trimWhitespace(copyString).length() == 1)) {
                if (loops == 0) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << UNEXPECTED_LOOP_CLOSING_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                } else {
                    loops--;
                    this->m_commands->emplace_back(IByteStreamCommandType::LOOP_END, "");
                } 
            } else if (foundDelayPosition != std::string::npos) {
                std::string targetDelay{""};
                std::string identifier{""};
                size_t foundDelaySecondsPosition{copyString.find(DELAY_SECONDS_IDENTIFIER)};
                size_t foundDelayMillisecondsPosition{copyString.find(DELAY_MILLISECONDS_IDENTIFIER)};
                size_t foundDelayMicrosecondsPosition{copyString.find(DELAY_MICROSECONDS_IDENTIFIER)};
                IByteStreamCommandType commandType{IByteStreamCommandType::COMMAND_UNSPECIFIED};
                if (foundDelaySecondsPosition != std::string::npos) {
                    identifier = static_cast<std::string>(DELAY_SECONDS_IDENTIFIER);
                    commandType = IByteStreamCommandType::DELAY_SECONDS;
                } else if (foundDelayMillisecondsPosition != std::string::npos) {
                    identifier = static_cast<std::string>(DELAY_MILLISECONDS_IDENTIFIER);
                    commandType = IByteStreamCommandType::DELAY_MILLISECONDS;
                } else if (foundDelayMicrosecondsPosition != std::string::npos) {
                    identifier = static_cast<std::string>(DELAY_MICROSECONDS_IDENTIFIER);
                    commandType = IByteStreamCommandType::DELAY_MICROSECONDS;
                } else {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << CONFIG_EXPRESSION_MALFORMED_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                }
                if (copyString.find(")") == std::string::npos) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << NO_CLOSING_PARENTHESIS_FOUND_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                }
                targetDelay = getBetween("(", ")", copyString);
                try {
                    long long int delay{std::stol(targetDelay)};
                    this->m_commands->emplace_back(commandType, toStdString(delay));
                } catch (std::exception &e) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    if (commandType == IByteStreamCommandType::DELAY_SECONDS) {
                        std::cout << DELAY_SECONDS_PARAMETER_NOT_AN_INTEGER_STRING << std::endl;
                    } else if (commandType == IByteStreamCommandType::DELAY_MILLISECONDS) {
                        std::cout << DELAY_MILLISECONDS_PARAMETER_NOT_AN_INTEGER_STRING << std::endl;
                    } else if (commandType == IByteStreamCommandType::DELAY_MICROSECONDS) {
                        std::cout << DELAY_MICROSECONDS_PARAMETER_NOT_AN_INTEGER_STRING << std::endl;
                    }
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->find(")")-1) << EXPECTED_HERE_STRING << std::endl;
                }
            } else if (foundWritePosition != std::string::npos) {
                std::string targetString{""};
                if (copyString.find(")") == std::string::npos) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << NO_CLOSING_PARENTHESIS_FOUND_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                }
                targetString = getBetween("(", ")", *iter);
                if (targetString.find_last_of("\"") == targetString.find("\"")) {
                    if (targetString.find(targetString.find("\"")) != std::string::npos) {
                        std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                        std::cout << NO_CLOSING_QUOTATION_MARKS_FOUND_STRING << std::endl;
                        std::cout << *iter << std::endl;
                        std::cout << tWhitespace(iter->find("\"")) << EXPECTED_HERE_STRING << std::endl;
                        continue;
                    } else {
                        std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                        std::cout << WRITE_PARAMETER_MUST_BE_IN_QUOTATIONS_STRING << std::endl;
                        std::cout << *iter << std::endl;
                        std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                        continue;
                    }
                }
                targetString = getBetween("\"", "\"", targetString);
                this->m_commands->emplace_back(IByteStreamCommandType::WRITE, targetString);
            } else if (foundReadPosition != std::string::npos) {
                if (copyString.find(")") == std::string::npos) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << NO_CLOSING_PARENTHESIS_FOUND_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                }
                this->m_commands->emplace_back(IByteStreamCommandType::READ, "");
            } else if (foundFlushPosition != std::string::npos) {
                if (copyString.find(")") == std::string::npos) {
                    std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                    std::cout << NO_CLOSING_PARENTHESIS_FOUND_STRING << std::endl;
                    std::cout << *iter << std::endl;
                    std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
                    continue;
                }
                this->m_commands->emplace_back(IByteStreamCommandType::FLUSH_RX, "");
            } else {
                std::cout << GENERIC_CONFIG_WARNING_BASE_STRING << currentLine << GENERIC_CONFIG_WARNING_TAIL_STRING << std::endl;
                std::cout << CONFIG_EXPRESSION_MALFORMED_STRING << std::endl;
                std::cout << *iter << std::endl;
                std::cout << tWhitespace(iter->length()-1) << HERE_STRING << std::endl;
            }
        } catch (std::exception &e) {
            std::cout << EXCEPTION_IN_CONSTRUCTOR_STRING << e.what() << std::endl;
            this->m_commands->clear();
            return;
        }
    }
    if (loops != 0) {
        std::cout << UNTERMINATED_LOOP_STRING << std::endl;
        this->m_commands->clear();
        return; 
    }
}


bool IByteStreamScriptReader::fileExists(const std::string &fileToCheck)
{
    #if defined(_WIN32)
        return (PathFileExists(directoryToCheck.c_str()) == 1);
    #else
        return (access(fileToCheck.c_str(),F_OK) != -1);
    #endif
}

bool IByteStreamScriptReader::fileExists(const char *fileToCheck)
{
    return fileExists(static_cast<std::string>(fileToCheck));
}

std::string IByteStreamScriptReader::scriptFilePath() const
{
    return this->m_scriptFilePath;
}

std::shared_ptr<std::vector<IByteStreamCommand>> IByteStreamScriptReader::commands() const
{
    return this->m_commands;
}

IByteStreamScriptExecutor::IByteStreamScriptExecutor(const std::string &iByteStreamScriptFilePath) :
    m_iByteStreamScriptReader{std::make_shared<IByteStreamScriptReader>(iByteStreamScriptFilePath)}
{

}

std::string IByteStreamScriptExecutor::scriptFilePath() const
{
    return this->m_iByteStreamScriptReader->scriptFilePath();
}

bool IByteStreamScriptExecutor::hasCommands() const
{
    return (this->m_iByteStreamScriptReader->commands()->size() > 0);
}

size_t IByteStreamScriptExecutor::numberOfCommands() const
{
    return this->m_iByteStreamScriptReader->commands()->size();
}

void IByteStreamScriptExecutor::setScriptFilePath(const std::string &iByteStreamScriptFilePath)
{
    this->m_iByteStreamScriptReader.reset();
    this->m_iByteStreamScriptReader = std::make_shared<IByteStreamScriptReader>(iByteStreamScriptFilePath);
}

std::vector<IByteStreamCommand> IByteStreamScriptExecutor::doUnrollLoopCommands(const std::vector<IByteStreamCommand> &IByteStreamCommands)
{
    std::vector<IByteStreamCommand> copyCommands{IByteStreamCommands};
    if (!IByteStreamScriptExecutor::containsLoopStart(copyCommands)) {
        return copyCommands;
    }
    while (IByteStreamScriptExecutor::containsLoopStart(copyCommands)) {
        std::pair<int, int> innerLoopPositions = findInnerLoopIndexes(copyCommands);
        std::vector<IByteStreamCommand> toUnroll{};
        std::vector<IByteStreamCommand> temp{};
        int numberOfLoops{std::stoi(copyCommands.at(innerLoopPositions.first).commandArgument())};
        for (int loops = 0; loops < numberOfLoops; loops++) {
            for (int i = innerLoopPositions.first+1; i < innerLoopPositions.second; i++) {
                toUnroll.emplace_back(copyCommands.at(i));
            }
        }
        for (int i = 0; i < innerLoopPositions.first; i++) {
            temp.emplace_back(copyCommands.at(i));
        }
        for (auto &it : toUnroll) {
            temp.emplace_back(it);
        }
        for (unsigned int i = innerLoopPositions.second+1; i < copyCommands.size(); i++) {
            temp.emplace_back(copyCommands.at(i));
        }
        copyCommands = temp; 
    }
    return copyCommands;
}
std::pair<int, int> IByteStreamScriptExecutor::findInnerLoopIndexes(const std::vector<IByteStreamCommand> &IByteStreamCommands)
{
    int lastLoopStartPosition{0};
    int lastLoopEndPosition{0};
    for (unsigned int i = 0; i < IByteStreamCommands.size(); i++) {
        if (IByteStreamCommands.at(i).commandType() == IByteStreamCommandType::LOOP_START) {
            lastLoopStartPosition = i;
        }
    }

    for (unsigned int i = lastLoopStartPosition; i < IByteStreamCommands.size(); i++) {
        if (IByteStreamCommands.at(i).commandType() == IByteStreamCommandType::LOOP_END) {
            if (lastLoopStartPosition == 0) {
                lastLoopEndPosition = lastLoopStartPosition + i;
            } else {
                lastLoopEndPosition = lastLoopStartPosition + i - 1;
            }
            break;
        }
    }
    return std::make_pair(lastLoopStartPosition, lastLoopEndPosition);
}

bool IByteStreamScriptExecutor::containsLoopStart(const std::vector<IByteStreamCommand> &commands) 
{
    for (auto &it : commands) {
        if (it.commandType() == IByteStreamCommandType::LOOP_START) {
            return true;
        }
    }
    return false;
}


