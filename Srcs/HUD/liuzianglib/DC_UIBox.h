#pragma once
#ifndef liuzianglib_UIBox
#define liuzianglib_UIBox
#include <iostream>
#include <string>
#include <conio.h>
#include "DC_STR.h"
//Version 2.4.21V30
//20170914

namespace DC {

	namespace UIBox {//UIBox 是为 Windows 控制台程序设计的简易用户界面
		
		std::string::size_type HowmuchLine(std::string &str) {
			if (str.empty() != 0) return 0;
			std::string::size_type count = 1, count2 = 0;
			for (std::string::iterator strp = str.begin(); strp != str.end(); strp++) {
				if (*strp == '\n') { count++; count2 = 0; continue; }
				if (count2 == 80) { count++; count2 = 0; continue; }
				count2++;
			}
			return count;
		}

		static void inline SetWindow() {
			system("mode con cols=80 lines=25");
		}

		void inline SetWindow(int32_t inputX, int32_t inputY) {
			std::string cmd("mode con cols=");
			cmd += STR::toString(inputX);
			cmd += " lines=";
			cmd += STR::toString(inputY);
			system(cmd.c_str());
		}

		void inline SetWindow(std::string &str, int32_t MoreLine) {
			std::string TEMP_system("mode con cols=80 lines=");
			TEMP_system += STR::toString(HowmuchLine(str) + MoreLine);
			system(TEMP_system.c_str());
		}

		void inline ShowTitle(const std::string& str) {
			system("cls");
			SetWindow();
			std::cout << str << "\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
		}

		void ScanfDyna(const std::string Title, std::string &ch, int32_t firstword) {//firstword 为 1 时，开启首字母大写(如果它是字母的话)
			SetWindow();
			std::string input;
			ch.clear();
			while (1) {
				ShowTitle(Title);
				std::cout << ch;
				getline(std::cin, input);
				if (ch.empty() == 1 && input.empty() == 1) continue;
				if (input == "exit" || input == "Exit" || input == "EXIT") { if (ch.empty() != 1 && ch.size() != 1) ch.resize(ch.size() - 1); return; }
				if (ch.empty() != 0 && input.empty() != 1 && firstword == 1) { if (input[0] >= 97 && input[0] <= 122) input[0] -= 32; }
				input += "\n";
				ch += input;
				input.clear();
			}
		}

		void ScanfDyna(const std::string Title, std::string &ch, int32_t firstword, const std::string Finishword) {//firstword 为 1 时，开启首字母大写(如果它是字母的话)
			SetWindow();
			std::string input;
			ch.clear();
			while (1) {
				ShowTitle(Title);
				std::cout << ch;
				getline(std::cin, input);
				if (ch.empty() == 1 && input.empty() == 1) continue;
				if (input == Finishword) { if (ch.empty() != 1 && ch.size() != 1) ch.resize(ch.size() - 1); return; }
				if (ch.empty() != 0 && input.empty() != 1 && firstword == 1) { if (input[0] >= 97 && input[0] <= 122) input[0] -= 32; }
				input += "\n";
				ch += input;
				input.clear();
			}
		}

		void DelDyna(std::string Title, std::string &ch) {
			if (ch.size() >= 1600) SetWindow(ch, 2); else SetWindow();
			char TEMP_input;
			while (1) {
				system("cls");
				ShowTitle(Title);
				std::cout << ch;
				TEMP_input = _getch();
				switch (TEMP_input) {
				case '\r': {return; }break;
				case '\b': {
					if (ch.size() == 0) return;
					if ((ch[ch.size() - 1] < 0 || ch[ch.size() - 1]>127) && (ch[ch.size() - 2] < 0 || ch[ch.size() - 2]>127)) {
						ch.resize(ch.size() - 1);
						ch.resize(ch.size() - 1);
						continue;
					}
					ch.resize(ch.size() - 1);
				}break;
				}
			}
		}

		void inline ConBox() {
			char ch;
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			std::cout << "输入任意字符来继续...\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			ch = _getch();
			system("cls");
		}

		void inline ConBox(const std::string& str) {
			char ch;
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			std::cout << str << "\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			std::cout << "输入任意字符来继续...\n";
			ch = _getch();
			system("cls");
		}

		template<typename ...ARGS>
		int32_t ConBox2(char title[], ARGS&& ...args) {
			auto items = DC::GetArgs(args...);
			int32_t index;
			std::string output;
			while (1) {
				system("cls");
				std::cout << title << "\n";
				for (int32_t i = 0; i < 80; ++i) std::cout << "*";
				output = "";
				index = 1;
				for (auto& p : items) {
					try {
						output += DC::STR::toString(index);
					}
					catch (bool) {
						return ConBox2(title, std::forward<ARGS>(args)...);//我感觉你们呐还是要迭代一个
					}
					output += ".";
					output += p.as_cstr();
					output += "  ";
					index++;
				}
				if (index != 1) output.resize(output.size() - 2);//删掉最后多余的俩空格,ifindex != 1的意思是限定此语句只在args不为空的情况下执行
				std::cout << output << "\n";
				for (int32_t i = 0; i < 80; ++i) std::cout << "*";
				std::cout << "\n";
				output = (char)_getch();
				try {
					if (output != "0" && !(DC::STR::toTYPE<int32_t>(output) > items.size())) return DC::STR::toTYPE<int32_t>(output);
				}
				catch (bool) {
					//什么都不做，自动开始下一次循环
				}
			}
		}

		int32_t Menu(char *title, int32_t Tcount, int32_t Chocount, char *Cho1, char *Cho2, char *Cho3, char *Cho4) {
			system("cls");
			int32_t ch = 0;
			char c;
			for (int32_t i = 0; i < Tcount; ++i) std::cout << " ";
			std::cout << title << "\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			for (int32_t i = 0; i < Chocount; ++i) std::cout << " ";
			std::cout << "1." << Cho1 << "  2." << Cho2 << "  3." << Cho3 << "  4." << Cho4 << "\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			std::cout << "\n";
			c = _getch();
			switch (c) {
			case '1': {ch = 1; }break;
			case '2': {ch = 2; }break;
			case '3': {ch = 3; }break;
			case '4': {ch = 4; }break;
			default: {ch = 0; }break;
			}
			system("cls");
			return ch;
		}

		//第一个参数是软件版本，第二个参数是开发者信息
		void inline About(std::string Verison, std::string Developer, std::string Des) {
			system("cls");
			std::cout << "                                    关于\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			std::cout << "开发人员:" << Developer << "  软件版本:" << Verison << "\n\n附加信息:" << Des << "\n";
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
			system("ver");
			ConBox();
		}

		void inline DrawLine() {
			for (int32_t i = 0; i < 80; ++i) std::cout << "*";
		}

	}

}

#endif
