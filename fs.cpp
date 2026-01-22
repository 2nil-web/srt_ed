
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "fs.h"
#include "is_utf8.h"

std::string file2s(std::filesystem::path p)
{
  std::ifstream ifs(p);
  std::string s;

  if (ifs.good())
  {
    s = std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    ifs.close();
  }
  else
    s = "Unable to open the file '" + p.string() + "'";

  if (s.length() == 0)
    s = "File " + p.string() + " is empty.";
  return s;
}

std::string iso_to_utf(std::string &si)
{
  std::string so;
  for (std::string::iterator it = si.begin(); it != si.end(); ++it)
  {
    uint8_t ch = *it;
    if (ch < 0x80)
      so.push_back(ch);
    else
    {
      so.push_back(0xc0 | ch >> 6);
      so.push_back(0x80 | (ch & 0x3f));
    }
  }

  return so;
}

std::string fread_txt(std::filesystem::path p)
{
  std::string s = file2s(p);
  if (is_utf8(s.c_str(), s.size()))
  {
    return s;
  }
  else
  {
    return iso_to_utf(s);
  }
}

bool fwrite_txt(std::filesystem::path p, const std::string &s, std::ios_base::openmode omod)
{
  std::ofstream of(p, omod);

  if (of)
  {
    of << s;
    of.close();
    return true;
  }
  return false;
}
