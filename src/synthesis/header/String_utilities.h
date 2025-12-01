//
// Created by Shufang Zhu on 06/11/2025.
//

#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <functional>


namespace Syft {

  std::vector<std::string> split(const std::string& str, const std::string& delimiter = " ");
  std::string trim(const std::string& str);
  std::string to_lower_copy(const std::string& str);
  std::string to_upper_copy(const std::string& str);
  // --- replace_all ---
  inline std::string replace_all(const std::string& input,
                                 const std::string& from,
                                 const std::string& to)
  {
    if (from.empty()) return input;
    std::string result = input;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
      result.replace(pos, from.length(), to);
      pos += to.length();
    }
    return result;
  }

  // // --- split ---
  // inline std::vector<std::string> split(const std::string& input,
  //                                       const std::string& delimiters)
  // {
  //   std::vector<std::string> tokens;
  //   size_t start = 0, end = 0;
  //   while ((end = input.find_first_of(delimiters, start)) != std::string::npos) {
  //     if (end != start)
  //       tokens.emplace_back(input.substr(start, end - start));
  //     start = end + 1;
  //   }
  //   if (start < input.size())
  //     tokens.emplace_back(input.substr(start));
  //   return tokens;
  // }

  // --- is_any_of ---
  inline std::function<bool(char)> is_any_of(const std::string& chars)
  {
    return [chars](char c) {
      return chars.find(c) != std::string::npos;
    };
  }

  // --- trim_if ---
  inline std::string trim_if(const std::string& input,
                             const std::function<bool(char)>& pred)
  {
    if (input.empty()) return input;

    size_t start = 0;
    while (start < input.size() && pred(input[start]))
      ++start;

    size_t end = input.size();
    while (end > start && pred(input[end - 1]))
      --end;

    return input.substr(start, end - start);
  }

  // --- starts_with ---
  inline bool starts_with(const std::string& input, const std::string& prefix)
  {
    if (prefix.size() > input.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), input.begin());
  }
}


#endif //SYFT4FOND_STRING_UTILITIES_H