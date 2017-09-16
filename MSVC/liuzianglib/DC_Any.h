#pragma once
#ifndef liuzianglib_Any
#define liuzianglib_Any
#include <iostream>
#include <string>
#include <memory>
#include <typeindex>
#include "DC_Exception.h"
//Version 2.4.21V30
//20170914

namespace DC {
	
	class Any {
	public:
		Any(void) : m_tpIndex(typeid(void)) {}

		template<typename U, class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type>//这一句用了SFINAE方法。原理是当U是Any的时候，第二个class将会无法推导(enable_if条件不成立时将没有成员typename enable_if::type)，从而诱导编译器转向另外两个构造函数
		Any(U&& value) :
			m_ptr(new Derived<typename std::decay<U>::type>(std::forward<U>(value))), m_tpIndex(typeid(typename std::decay<U>::type)) {}

		Any(const Any& that) : m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex) {}

		Any(Any&& that)noexcept : m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex) {
			that.m_tpIndex = typeid(void);
		}

	public:
		Any& operator=(const Any& input) {
			if (m_ptr == input.m_ptr)
				return *this;
			m_ptr = input.Clone();
			m_tpIndex = input.m_tpIndex;
			return *this;
		}

		Any& operator=(Any&& input)noexcept {
			if (this == &input)
				return *this;
			m_ptr = std::move(input.m_ptr);
			m_tpIndex = input.m_tpIndex;
			input.m_tpIndex = typeid(void);
			return *this;
		}

		template<typename T>
		Any& operator=(T&& input) {
			m_ptr.reset(new Derived<typename std::decay<T>::type>(std::forward<T>(input)));
			m_tpIndex = typeid(typename std::decay<T>::type);
			return *this;
		}

	public:
		inline bool has_value() const {
			return (bool)m_ptr;
		}

		inline std::type_index type() const {
			return m_tpIndex;
		}

		template<typename T>
		inline void set(T&& value) {
			m_ptr.reset(new Derived<typename std::decay<T>::type>(std::forward<T>(value)));
			m_tpIndex = typeid(typename std::decay<T>::type);
		}

		template<typename U>
		inline U& get()const {
			auto ptr_with_type = dynamic_cast<Derived<U>*> (m_ptr.get());
			if (!ptr_with_type) throw DC::Exception("DC::Any::get", "bad cast");

			return ptr_with_type->m_value;
		}

	private:
		class Base;
		typedef std::unique_ptr<Base> BasePtr;

		class Base {
		public:
			virtual ~Base() = default;

			virtual BasePtr Clone() const = 0;
		};

		template<typename T>
		struct Derived final : public Base {
		public:
			template<typename U>
			Derived(U&& value) :m_value(std::forward<U>(value)) {}

			Derived(const Derived& input) :m_value(input.m_value) {}

			Derived(Derived&& input) :m_value(std::move(input.m_value)) {}

		public:
			virtual BasePtr Clone() const override {
				return BasePtr(new Derived<T>(m_value));
			}

		public:
			T m_value;
		};

	private:
		BasePtr Clone() const {
			if (m_ptr != nullptr)
				return m_ptr->Clone();
			return nullptr;
		}

	private:
		BasePtr m_ptr;
		std::type_index m_tpIndex;
	};

}

#endif
