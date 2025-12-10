#pragma once
#include <string>
#include <vector>

////////////////////////////////////
/// \brief base16编码
/// \param data 需要编码的二进制数据
/// \return 返回base16编码的字符串
std::string Base16Encode(const std::vector<unsigned char> &data);


//////////////////////////////////////
/// \brief base16解码
/// \param str 需要解码的base16字符串，必须二的倍数
/// \return 返回原始的二进制数据
std::vector<unsigned char> Base16Decode(const std::string &str);
