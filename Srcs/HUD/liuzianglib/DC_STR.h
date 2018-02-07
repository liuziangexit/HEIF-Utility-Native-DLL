#pragma once
#ifndef liuzianglib_STR
#define liuzianglib_STR
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <sstream>
#include "DC_Exception.h"
#include "DC_type.h"
//Version 2.4.21V45
//20171030

namespace DC {

	namespace STR {

		namespace STRSpace {

			template <typename numtype>
			std::size_t Howmuchdig(numtype num) {//返回num的位数，比如num=1000时，返回4
				int32_t i = 0;
				while (num > 1) { num /= 10; i++; }
				if (num == 1) return i + 1; else return i;
			}

			std::vector<std::size_t> KMPSearch(const char *pat, const char *txt) {
				std::size_t M = strlen(pat), N = strlen(txt), j = 0, i = 0;
				std::unique_ptr<std::size_t[]> lps(reinterpret_cast<std::size_t*>(malloc(sizeof(std::size_t)*M)));
				if (!lps.get()) return std::vector<std::size_t>();
				auto computeLPSArray = [&pat, &M, &lps] {
					std::size_t len = 0, i = 1;  // 记录前一个[最长匹配的前缀和后缀]的长度
					lps[0] = 0; // lps[0] 必须是 0
					while (i < M)
					{
						if (pat[i] == pat[len])
						{
							len++;
							lps[i] = len;
							i++;
						}
						else // (pat[i] != pat[len])
						{
							if (len != 0)
							{
								// 这个地方有陷阱. 考虑这个例子 AAACAAAA ,i = 7.
								len = lps[len - 1];

								// 另外, 注意 i 在这个地方并没有增加
							}
							else // 如果 (len == 0)
							{
								lps[i] = 0; //没有一个匹配的
								i++;
							}
						}
					}
				};
				computeLPSArray();
				std::vector<std::size_t> returnvalue;
				while (i < N) {
					if (pat[j] == txt[i])
					{
						j++;
						i++;
					}
					if (j == M)
					{
						returnvalue.emplace_back(i - j);
						j = lps[j - 1];
					}
					else if (pat[j] != txt[i])
					{
						if (j != 0)
							j = lps[j - 1];
						else
							i = i + 1;
					}
				}
				return returnvalue;
			}

			struct ReplaceInfo final {
				std::vector<std::size_t> places;
				std::size_t remove_length = 0;
			};

			class SetLocal final {
			public:
				SetLocal() :curLocale(setlocale(LC_ALL, NULL)) {
					setlocale(LC_ALL, "chs");
				}

				~SetLocal() {
					setlocale(LC_ALL, curLocale.c_str());
				}

				SetLocal(const SetLocal&) = delete;

				SetLocal& operator=(const SetLocal&) = delete;

			private:
				std::string curLocale;
			};

			inline std::size_t getsomezero(const int& howmuch) {
				if (howmuch == 0) return 1;
				if (howmuch < 0) return 0;
				std::size_t returnvalue(1);
				for (int i = 0; i < howmuch; returnvalue *= 10, i++);
				return returnvalue;
			}

			inline int getbitvalue(const int& num, const int& which) {//从右往左
				if (which <= 0) return -1;
				if (which > Howmuchdig(num)) return -1;
				return static_cast<int>(num / getsomezero(which - 1)) % 10;
			}

			inline bool isNegative(const int& num) {
				if (num < 0) return true;
				return false;
			}

			inline int getAbs(const int& num) {
				if (num == 0) return 0;
				if (isNegative(num)) return 0 - num;
				return num;
			}

			inline char getCharFromNum(const int& num) {
				switch (num) {
				case 0:return '0';
				case 1: return '1';
				case 2: return '2';
				case 3: return '3';
				case 4: return '4';
				case 5: return '5';
				case 6: return '6';
				case 7: return '7';
				case 8: return '8';
				case 9: return '9';
				}
			}

		}

		std::string toUpper(std::string str) {//将 std::string str 中的小写字符转为大写
			transform(str.begin(), str.end(), str.begin(), ::toupper);
			return str;
		}

		std::string toLower(std::string str) {//将 std::string str 中的大写字符转为小写
			transform(str.begin(), str.end(), str.begin(), ::tolower);
			return str;
		}

		inline STRSpace::ReplaceInfo find(const std::string& str, const std::string& findstr) {//使用 KMP 字符串匹配算法找到所有存在于 std::string str 中的 std::string find，并返回它们的位置		
			if (str.empty() || findstr.empty()) return STRSpace::ReplaceInfo();
			STRSpace::ReplaceInfo rv;
			rv.remove_length = findstr.size();
			rv.places = STRSpace::KMPSearch(findstr.c_str(), str.c_str());
			return rv;
		}

		std::string replace(const std::string& str, const STRSpace::ReplaceInfo& info, const std::string& rpword) {//请确保 whererp 中的元素(表示位置的数字)是从小到大有序排列的。
																												   //不能查找替换循环的字符串，比如查找hh，把hh替换为h，这个不work
			if (info.places.empty() || str.empty()) return str;
			const std::size_t endsize = str.size() + info.places.size()*(rpword.size() - info.remove_length);
			auto wherepit = info.places.begin();
			std::string TEMP_str;
			for (std::size_t index = 0; TEMP_str.size() < endsize;) {
				if (wherepit != info.places.end()) {
					if (index == *wherepit) {
						TEMP_str += rpword;
						index += info.remove_length;
						wherepit++;
						continue;
					}
				}
				TEMP_str += str.at(index);
				index++;
			}
			return TEMP_str;
		}

		inline std::string getSub(const std::string& str, const DC::pos_type& startpos, const DC::pos_type& endpos) {//获取子串
																													 //从startpos+1到endpos-1
			if (endpos < startpos || endpos>str.size() || endpos < 0) throw DC::DC_ERROR("pos illegal", 0);
			std::string returnvalue;
			returnvalue.reserve(endpos - startpos);
			for (std::size_t i = startpos + 1; i <= endpos - 1 && i < str.size(); i++)
				returnvalue.push_back(str[i]);
			return returnvalue;
		}

		template <typename TYPE>
		TYPE toType(const std::string &str) {
			TYPE rv;
			std::stringstream sstr(str);
			sstr >> rv;
			if (sstr.fail()) {
				//不需要clear，因为sstr不会再次使用
				throw DC::DC_ERROR("std::stringstream::fail()==true", 0);
			}
			return rv;
		}

		template <>
		const char* toType<const char*>(const std::string &str) {
			return str.c_str();
		}

		template <>
		char* toType<char*>(const std::string &str) = delete;

		template <>
		std::wstring toType<std::wstring>(const std::string& s) {
			std::unique_ptr<wchar_t[]> toWide(new wchar_t[s.size() + 1]);
			STRSpace::SetLocal setlocal;
			mbstowcs(toWide.get(), s.c_str(), s.size() + 1);
			return std::wstring(toWide.get());
		}

		template <typename T>
		std::string toString(const T& value) {
			std::stringstream sstr;
			sstr << value;
			if (sstr.fail()) {
				//不需要clear，因为sstr不会再次使用
				throw DC::DC_ERROR("std::stringstream::fail()==true", 0);
			}
			return sstr.str();
		}

		template <>
		std::string toString<std::wstring>(const std::wstring& ws) {
			std::unique_ptr<char[]> toStr(new char[ws.size() + 1]);
			STRSpace::SetLocal setlocal;
			wcstombs(toStr.get(), ws.c_str(), ws.size() + 1);
			return std::string(toStr.get());
		}

		template <>
		std::string toString<int>(const int& num) {
			if (num == 0) return "0";
			if (STRSpace::isNegative(num)) {
				std::string rv("-");
				auto abs = STRSpace::getAbs(num);
				for (std::size_t i = STRSpace::Howmuchdig(abs); i > 0; i--)
					rv.push_back(STRSpace::getCharFromNum(STRSpace::getbitvalue(abs, i)));
				return rv;
			}
			std::string rv;
			for (std::size_t i = STRSpace::Howmuchdig(num); i > 0; i--)
				rv.push_back(STRSpace::getCharFromNum(STRSpace::getbitvalue(num, i)));
			return rv;
		}

	}

}

#endif
