#pragma once
#ifndef liuzianglib_ERROR
#define liuzianglib_ERROR
#include <string>
#include <time.h>
//Version 2.4.21V45
//20171030

namespace DC {

	class Exception {
	public:
		Exception() = default;

		template <typename T, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Exception>::value, std::string>::type>
		Exception(T&& inputTitle) {
			Title = std::forward<T>(inputTitle);
		}

		template <typename T, typename T2>
		Exception(T&& inputTitle, T2&& inputDescription) {
			Title = std::forward<T>(inputTitle);
			Description = std::forward<T2>(inputDescription);
		}

		Exception(const Exception&) = default;

		Exception(Exception&&) = default;

		virtual ~Exception() = default;

	public:
		Exception& operator=(const Exception& input) {
			Title = input.Title;
			Description = input.Description;
			return *this;
		}

		Exception& operator=(Exception&& input) {
			Title = std::move(input.Title);
			Description = std::move(input.Description);
			return *this;
		}

	public:
		template <typename T>
		inline void SetTitle(T&& input) {
			Title = std::forward<T>(input);
		}

		template <typename T>
		inline void SetDescription(T&& input) {
			Description = std::forward<T>(input);
		}

		inline std::string GetTitle()const {
			return Title;
		}

		inline std::string GetDescription()const {
			return Description;
		}

	private:
		std::string Title, Description;
	};

	using DC_ERROR = Exception;

}

#endif
