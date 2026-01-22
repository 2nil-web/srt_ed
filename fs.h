#ifndef FS_H
#define FS_H

std::string file2s(std::filesystem::path);
std::string iso_to_utf(std::string &);
std::string fread_txt(std::filesystem::path p);
bool fwrite_txt(std::filesystem::path p, const std::string &s, std::ios_base::openmode omod = std::ios::trunc);

#endif /* FS_H */
