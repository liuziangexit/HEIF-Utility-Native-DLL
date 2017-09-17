#pragma once
#ifndef liuzianglib_var
#define liuzianglib_var
#include "DC_STR.h"
//Version 2.4.21V30
//20170914

namespace DC {

	class var {
	public:
		var() = default;

		var(const var& input) {
			this->data = input.data;
		}

		var(var&& input)noexcept {
			this->data = std::move(input.data);
		}

		template<typename T>
		var(const T& input) {
			this->data = STR::toString(input);
		}

	public:
		var& operator=(const var& input) {
			this->data = input.data;
			return *this;
		}

		var& operator=(var&& input)noexcept {
			this->data = std::move(input.data);
			return *this;
		}

		template<typename T>
		var& operator=(const T& input) {
			this->data = STR::toString(input);
			return *this;
		}

	public:
		template<typename T>
		T as_any() {
			return STR::toType<T>(data);
		}

		std::string as_string() {
			return as_any<std::string>();
		}

		const char* as_cstr() {
			return as_any<const char*>();
		}

		int32_t as_int() {
			return as_any<int32_t>();
		}

		double as_double() {
			return as_any<double>();
		}

		bool as_bool() {
			return as_any<bool>();
		}

		void clear() {
			data.clear();
		}

	private:
		std::string data;
	};

}

#endif
