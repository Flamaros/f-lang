#include "string.hpp"

#include "logger.hpp"

#include "../platform.hpp"

#include <algorithm>
#include <stdarg.h>
#include <stdlib.h>

/*! Methode pour parser un std::string (0,0,0) */
std::vector<std::string> utilities::explodeString(const std::string& source, const std::string& separator)
{
    size_t  i = 0;
    size_t  uPos = 0;
    std::vector<std::string> result;

    while (i < 1)
    {
        size_t pos = source.find(separator, uPos);
        if (pos == source.npos)
        {
            result.push_back(source.substr(uPos, source.length() - uPos));
            i = 1;
        }
        else
        {
            result.push_back(source.substr(uPos,pos-uPos));
            uPos = pos + 1;
        }
    }
    
    return result;
}

void utilities::replace(std::string& source, const std::string& oldStr, const std::string& newStr)
{
    size_t  pos = 0;

    while ((pos = source.find(oldStr, pos)) != std::string::npos)
    {
        source.replace(pos, oldStr.size(), newStr);
        pos += newStr.length();
    }
}

std::string utilities::replace(const std::string& source, const std::string& oldStr, const std::string& newStr)
{
    std::string result = source;
    utilities::replace(result, oldStr, newStr);
    return result;
}

std::string	utilities::sprintf(const char* format, ...)
{
    va_list     vlist;
    std::string text;

    va_start(vlist, format);
    utilities::vsprintf(format, vlist, text);
    va_end(vlist);
    return text;
}

void utilities::vsprintf(const char* format, va_list vlist, std::string& result)
{
#if defined(FL_WIN32)
	int     bufferSize = _vscprintf(format, vlist) + 1;
    char*   buffer = new char[bufferSize];

	int length = ::vsnprintf(buffer, bufferSize, format, vlist);
	if (length < 0)
	{
		result = "vsnprintf ERROR";
	}

	if (length < bufferSize)
	{
		buffer[length] = 0;
		result = buffer;
	}

	delete[] buffer;
#else
    char* str;
    if (vasprintf(&str, format, vlist) != -1)
    {
        result = str;
        free(str);
    }
    return;
#endif
}

std::string utilities::getRelativePath(const std::string& basePath, std::string path)
{
    size_t  pos;

    if ((pos = path.find(basePath)) != std::string::npos)
        path = path.substr(pos + basePath.length());
    if ((pos = path.find("/")) == 0) // Remove the starting '/'
        path = path.substr(1);
    return path;
}

std::string utilities::getFileName(const std::string& path)
{
    size_t      pos;
    std::string result;

    if ((pos = path.rfind("/")) != std::string::npos)
        return path.substr(pos + 1);
    return path;
}

std::string utilities::getBaseName(const std::string& path)
{
    size_t      pos;
    std::string result;

    result = getFileName(path);
    if ((pos = result.find_first_of(".")) != std::string::npos)
        result = result.substr(0, pos);
    return result;
}

std::string utilities::getCompleteBaseName(const std::string& path)
{
    size_t      pos;
    std::string result;

    result = getFileName(path);
    if ((pos = result.rfind(".")) != std::string::npos)
        result = result.substr(0, pos);
    return result;
}

std::string utilities::getExtension(const std::string& path)
{
    size_t      pos;
    std::string result;

    result = getFileName(path);
    if ((pos = result.rfind(".")) != std::string::npos)
        result = result.substr(pos + 1);
    return result;
}

std::string utilities::getCompleteExtension(const std::string& path)
{
    size_t      pos;
    std::string result;

    result = getFileName(path);
    if ((pos = result.find_first_of(".")) != std::string::npos)
        result = result.substr(pos + 1);
    return result;
}

std::string utilities::getParentDirPath(const std::string& path)
{
    size_t      pos;
    std::string result;

    if ((pos = path.rfind("/")) != std::string::npos)
        result = path.substr(0, pos);
    return result;
}

std::string utilities::toLowerCase(const std::string& text)
{
    std::string data = text;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    return data;
}

std::string& utilities::toLowerCase(std::string& text)
{
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    return text;
}

std::string utilities::toUpperCase(const std::string& text)
{
    std::string data = text;
    std::transform(data.begin(), data.end(), data.begin(), ::toupper);
    return data;
}

std::string& utilities::toUpperCase(std::string& text)
{
    std::transform(text.begin(), text.end(), text.begin(), ::toupper);
    return text;
}

std::string utilities::toLowerCamelCase(const std::string& text)
{
    return utilities::toLowerCase(text.substr(0, 1)) + text.substr(1);
}
