#pragma once

#include "../platform.hpp"

#include <string>
#include <vector>

namespace utilities {
    std::vector<std::string> explodeString(const std::string& source, const std::string& separator);
    void                     replace(std::string& source, const std::string& oldStr, const std::string& newStr);
    std::string              replace(const std::string& source, const std::string& oldStr, const std::string& newStr);
    std::string				 sprintf(const char* format, ...);
    void                     vsprintf(const char* format, va_list ap, std::string& result);

    std::string              getRelativePath(const std::string& basePath, std::string path);
    std::string              getFileName(const std::string& path);
    std::string              getBaseName(const std::string& path);   /// The complete extension is removed
    std::string              getCompleteBaseName(const std::string& path);   /// Only the last extension is removed
    std::string              getExtension(const std::string& path);  /// doesn't contain the dot (extension is after the last dot character)
    std::string              getCompleteExtension(const std::string& path);  /// doesn't contain the dot
    std::string              getParentDirPath(const std::string& path);

    std::string              toLowerCase(const std::string& text);
    std::string&             toLowerCase(std::string& text);
    std::string              toUpperCase(const std::string& text);
    std::string&             toUpperCase(std::string& text);
    std::string              toLowerCamelCase(const std::string& text);
}
