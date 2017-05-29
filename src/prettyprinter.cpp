/***********************************************************************
*    prettyprinter.cpp:                                                *
*    PrettyPrinter class, for printing colored text to a POSIX terminal*
*    Copyright (c) 2016 Tyler Lewis                                    *
************************************************************************
*    This is a header file for tjlutils:                               *
*    https://github.serial/tlewiscpp/tjlutils                         *
*    This file may be distributed with the entire tjlutils library,    *
*    but may also be distributed as a standalone file                  *
*    The source code is released under the GNU LGPL                    *
*    This file holds the implementation of a PrettyPrinter class       *
*    It is used to print colored text to a POSIX terminal              *
*                                                                      *
*    You should have received a copy of the GNU Lesser General         *
*    Public license along with tjlutils                                *
*    If not, see <http://www.gnu.org/licenses/>                        *
***********************************************************************/

#include "prettyprinter.h"

const char *PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE{"\033["};
const char *PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL{"m"};
const char *PrettyPrinter::s_DEFAULT_TERMINAL_FOREGROUND_COLOR{"\033[39m"};
const char *PrettyPrinter::s_DEFAULT_TERMINAL_BACKGROUND_COLOR{"\033[49m"};
const char *PrettyPrinter::s_DEFAULT_TERMINAL_FONT_ATTRIBUTES{"\033[0m"};
const int s_RESET_FONT_ATTRIBUTE_OFFSET{20};

const std::vector<ForegroundColor> PrettyPrinter::s_FOREGROUND_TERMINAL_COLORS_CONTAINER{ForegroundColor::FG_DEFAULT,
                                                                                        ForegroundColor::FG_BLACK,
                                                                                        ForegroundColor::FG_RED,
                                                                                        ForegroundColor::FG_GREEN, 
                                                                                        ForegroundColor::FG_YELLOW, 
                                                                                        ForegroundColor::FG_BLUE, 
                                                                                        ForegroundColor::FG_MAGENTA, 
                                                                                        ForegroundColor::FG_CYAN, 
                                                                                        ForegroundColor::FG_LIGHT_GRAY,
                                                                                        ForegroundColor::FG_DARK_GRAY, 
                                                                                        ForegroundColor::FG_LIGHT_RED, 
                                                                                        ForegroundColor::FG_LIGHT_GREEN, 
                                                                                        ForegroundColor::FG_LIGHT_YELLOW, 
                                                                                        ForegroundColor::FG_LIGHT_BLUE, 
                                                                                        ForegroundColor::FG_LIGHT_MAGENTA, 
                                                                                        ForegroundColor::FG_LIGHT_CYAN, 
                                                                                        ForegroundColor::FG_WHITE};

const std::vector<BackgroundColor> PrettyPrinter::s_BACKGROUND_TERMINAL_COLORS_CONTAINER{BackgroundColor::BG_DEFAULT,
                                                                                        BackgroundColor::BG_BLACK,
                                                                                        BackgroundColor::BG_RED, 
                                                                                        BackgroundColor::BG_GREEN, 
                                                                                        BackgroundColor::BG_YELLOW,
                                                                                        BackgroundColor::BG_BLUE,
                                                                                        BackgroundColor::BG_MAGENTA,
                                                                                        BackgroundColor::BG_CYAN,
                                                                                        BackgroundColor::BG_LIGHT_GRAY,
                                                                                        BackgroundColor::BG_DARK_GRAY,
                                                                                        BackgroundColor::BG_LIGHT_RED,
                                                                                        BackgroundColor::BG_LIGHT_GREEN,
                                                                                        BackgroundColor::BG_LIGHT_YELLOW,
                                                                                        BackgroundColor::BG_LIGHT_BLUE,
                                                                                        BackgroundColor::BG_LIGHT_MAGENTA,
                                                                                        BackgroundColor::BG_LIGHT_CYAN,
                                                                                        BackgroundColor::BG_WHITE};

PrettyPrinter::PrettyPrinter() :
    m_foregroundColor{ForegroundColor::FG_DEFAULT},
    m_backgroundColor{BackgroundColor::BG_DEFAULT},
    m_outputStream{&std::cout},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

PrettyPrinter::PrettyPrinter(ForegroundColor foregroundColor) :
    m_foregroundColor{foregroundColor},
    m_backgroundColor{BackgroundColor::BG_DEFAULT},
    m_outputStream{&std::cout},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

PrettyPrinter::PrettyPrinter(BackgroundColor backgroundColor) :
    m_foregroundColor{ForegroundColor::FG_DEFAULT},
    m_backgroundColor{backgroundColor},
    m_outputStream{&std::cout},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

PrettyPrinter::PrettyPrinter(std::ostream *outputStream) :
    m_foregroundColor{ForegroundColor::FG_DEFAULT},
    m_backgroundColor{BackgroundColor::BG_DEFAULT},
    m_outputStream{outputStream},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

PrettyPrinter::PrettyPrinter(ForegroundColor foregroundColor, std::ostream *outputStream) :
    m_foregroundColor{foregroundColor},
    m_backgroundColor{BackgroundColor::BG_DEFAULT},
    m_outputStream{outputStream},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

PrettyPrinter::PrettyPrinter(BackgroundColor backgroundColor, std::ostream *outputStream) :
    m_foregroundColor{ForegroundColor::FG_DEFAULT},
    m_backgroundColor{backgroundColor},
    m_outputStream{outputStream},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

PrettyPrinter::PrettyPrinter(ForegroundColor foregroundColor, BackgroundColor backgroundColor, std::ostream *outputStream) :
    m_foregroundColor{foregroundColor},
    m_backgroundColor{backgroundColor},
    m_outputStream{outputStream},
    m_fontAttributes(FontAttribute::FA_DEFAULT)
{

}

void PrettyPrinter::setForegroundColor(ForegroundColor foregroundColor)
{
    this->m_foregroundColor = foregroundColor;
}

void PrettyPrinter::setBackgroundColor(BackgroundColor backgroundColor)
{
    this->m_backgroundColor = backgroundColor;
}

void PrettyPrinter::setFontAttributes(int fontAttributes)
{
    this->m_fontAttributes = fontAttributes;
}

void PrettyPrinter::setOutputStream(std::ostream *outputStream)
{
    this->m_outputStream = outputStream;
}

ForegroundColor PrettyPrinter::foregroundColor() const
{
    return this->m_foregroundColor;
}

BackgroundColor PrettyPrinter::backgroundColor() const
{
    return this->m_backgroundColor;
}

std::ostream *PrettyPrinter::outputStream()
{
    return this->m_outputStream;
}

int PrettyPrinter::fontAttributes() const
{
    return this->m_fontAttributes;
}

void PrettyPrinter::resetStreamToDefault(std::ostream *oStream)
{
    *oStream << PrettyPrinter::s_DEFAULT_TERMINAL_FOREGROUND_COLOR;
    *oStream << PrettyPrinter::s_DEFAULT_TERMINAL_BACKGROUND_COLOR;
    *oStream << PrettyPrinter::s_DEFAULT_TERMINAL_FONT_ATTRIBUTES;
}

void PrettyPrinter::resetBackgroundColor()
{
    this->m_backgroundColor = BackgroundColor::BG_DEFAULT;
    *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << this->m_backgroundColor << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
}

void PrettyPrinter::resetForegroundColor()
{
    this->m_foregroundColor = ForegroundColor::FG_DEFAULT;
    *this->m_outputStream << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_BASE << this->m_foregroundColor << PrettyPrinter::s_TERMINAL_ESCAPE_SEQUENCE_TAIL;
}

void PrettyPrinter::resetFontAttributes()
{
    this->m_fontAttributes = FontAttribute::FA_DEFAULT;
    *this->m_outputStream << PrettyPrinter::s_DEFAULT_TERMINAL_FONT_ATTRIBUTES;
}

std::vector<int> PrettyPrinter::getFontAttributes(int attr)
{
    std::vector<int> returnVector;
    if (attr & FontAttribute::FA_DEFAULT) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_DEFAULT);
        return returnVector;
    }
    if (attr & FontAttribute::FA_HIDDEN) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_HIDDEN);
        return returnVector;
    }

    if (attr & FontAttribute::FA_BOLD) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_BOLD);
    }
    if (attr & FontAttribute::FA_DIM) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_DIM);
    }
    if (attr & FontAttribute::FA_UNDERLINED) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_UNDERLINED);
    }
    if (attr & FontAttribute::FA_BLINK) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_BLINK);
    }
    if (attr & FontAttribute::FA_REVERSE) {
        returnVector.push_back(UnderlyingFontAttribute::UFA_REVERSE);
    }
    return returnVector;
}