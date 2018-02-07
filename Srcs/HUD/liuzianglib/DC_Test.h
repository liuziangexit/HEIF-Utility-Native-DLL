#pragma once
#ifndef liuzianglib_Test
#define liuzianglib_Test
#include <iostream>
#include "liuzianglib.h"
//Version 2.4.21V34
//20171021

namespace DC {

	class UniqueID {
	public:
		UniqueID() :uniqueid(DC::randomer(0, 65535)) {}

		UniqueID(const UniqueID&) = default;

	public:
		inline const int32_t& getUniqueID()const {
			return uniqueid;
		}

	private:
		const int32_t uniqueid;
	};

	class Test :public std::enable_shared_from_this<Test> {
	public:
		Test() {
			PrintInfo();
			std::cout << "默认构造\n";
		}

		Test(const Test& input) :uniqueid() {
			input.PrintInfo();
			std::cout << " 到 ";
			PrintInfo();
			std::cout << "拷贝构造\n";
		}

		Test(Test&& input) :uniqueid() {
			input.PrintInfo();
			std::cout << " 到 ";
			PrintInfo();
			std::cout << "移动构造\n";
		}

		~Test() {
			PrintInfo();
			std::cout << "析构\n";
		}

	public:
		Test& operator=(const Test& input) {
			input.PrintInfo();
			std::cout << " 到 ";
			PrintInfo();
			std::cout << "拷贝赋值运算符\n";
			return *this;
		}

		Test& operator=(Test&& input) {
			input.PrintInfo();
			std::cout << " 到 ";
			PrintInfo();
			std::cout << "移动赋值运算符\n";
			return *this;
		}

	public:
		inline void doNothing() {
			PrintInfo();
			std::cout << "什么都没有做\n";
		}

	private:
		inline void PrintInfo()const {
			std::cout << "Test(" << uniqueid.getUniqueID() << ") ";
		}

	private:
		UniqueID uniqueid;
	};

}

#endif