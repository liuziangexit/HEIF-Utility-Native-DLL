#pragma once
#ifndef liuzianglib_jsonBuidler
#define liuzianglib_jsonBuilder
#include <vector>
#include <string>
#include <type_traits>
#include "liuzianglib.h"
#include "DC_STR.h"
#include "DC_Exception.h"
//Version 2.4.21V30
//20170914

namespace DC {

	namespace Web {

		namespace jsonBuilder {

			namespace jsonBuilderSpace {

				enum DataType {
					string_t,
					int_t,
					double_t,
					null_t,
					bool_t,
					empty_t
				};

				class NV_base {//number和value的基类
				public:
					NV_base() : type(jsonBuilderSpace::DataType::empty_t) {}

					NV_base(const NV_base&) = default;

					NV_base(NV_base&& input) {
						this->rawStr = std::move(input.rawStr);
						this->type = input.type;
						input.type = jsonBuilderSpace::DataType::empty_t;
					}

					virtual ~NV_base() = default;

				public:
					inline void fromNull(void) {
						rawStr = "null";
						type = jsonBuilderSpace::DataType::null_t;
					}

					inline jsonBuilderSpace::DataType getType()const noexcept {
						return this->type;
					}

					inline std::string toString()const {
						switch (type) {
						case jsonBuilderSpace::DataType::bool_t: {
							if (rawStr == "1") return "true";
							return "false";
						}break;
						case jsonBuilderSpace::DataType::double_t: {
							return rawStr;
						}break;
						case jsonBuilderSpace::DataType::empty_t: {
							throw DC::Exception("jsonBuilder::jsonBuilderSpace::NV_base", "empty value");
						}break;
						case jsonBuilderSpace::DataType::int_t: {
							return rawStr;
						}break;
						case jsonBuilderSpace::DataType::null_t: {
							return rawStr;
						}break;
						case jsonBuilderSpace::DataType::string_t: {
							return '"' + rawStr + '"';
						}break;
						default: {
							throw DC::Exception("jsonBuilder::jsonBuilderSpace::NV_base", "bad type");
						}
						}
					}

					inline void clear() {
						rawStr.clear();
						type = DataType::empty_t;
					}

				protected:
					std::string rawStr;
					DataType type;
				};

				class JSKeyValuePair final :public DC::KeyValuePair {
				public:
					JSKeyValuePair() :DC::KeyValuePair::KeyValuePair() {}

					template <typename T, typename T2>
					JSKeyValuePair(T&& inputname, T2&& inputvalue) {
						static_assert(std::is_same<std::string, typename std::decay<T>::type>::value, "input type should be std::string");
						static_assert(std::is_same<std::string, typename std::decay<T2>::type>::value, "input type should be std::string");
						this->name = std::forward<T>(inputname);
						this->value = std::forward<T2>(inputvalue);
					}

					JSKeyValuePair(const JSKeyValuePair&) = default;

					JSKeyValuePair(JSKeyValuePair&& input) :DC::KeyValuePair::KeyValuePair(std::move(input)) {}

				public:
					JSKeyValuePair& operator=(const JSKeyValuePair&) = default;

					JSKeyValuePair& operator=(JSKeyValuePair&&) = default;

				public:
					template <typename T>
					inline void SetName(T&& input) {
						this->name = std::forward<T>(input);
					}

					template <typename T>
					inline void SetValue(T&& input) {
						this->value = std::forward<T>(input);
					}

					virtual inline char GetSeparator()const noexcept override {
						return ':';
					}

					virtual void Set(const std::string& input)override {}

					inline bool isSetOK()const = delete;
				};

				template<typename T>
				struct has_member_function_toString {
				private:
					using true_t = char;
					using false_t = char[2];

					template<typename T2, std::string(T::*)(void)const>
					struct func_matcher;

					template<typename T2>
					static true_t& test(func_matcher<T2, &T2::toString>*);

					template<typename T2>
					static false_t& test(...);

				public:
					enum
					{
						result = (sizeof(test<T>(NULL)) == sizeof(true_t))
					};
				};

				inline std::string GetJsStr(const std::string& input) {
					return "\"" + input + "\"";
				}

			}

			class value final :public jsonBuilderSpace::NV_base {
			public:
				value() :NV_base() {}

				template <typename T, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, value>::value, T>::type, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, bool>::value, T>::type>
				value(T&& inputstr) {
					this->fromString(std::forward<T>(inputstr));
				}

				value(const char* const inputstr) {
					this->fromString(std::string(inputstr));
				}

				value(const bool& inputbool) {
					this->fromBool(inputbool);
				}

				value(const value& input) :NV_base(input) {}

				value(value&& input) :NV_base(std::move(input)) {}

			public:
				template <typename T>
				inline void fromString(T&& input) {
					static_assert(std::is_same<std::string, typename std::decay<T>::type>::value, "input type should be std::string");
					rawStr = std::forward<T>(input);
					type = jsonBuilderSpace::DataType::string_t;
				}

				inline void fromBool(const bool& input) {
					rawStr = DC::STR::toString(input);
					type = jsonBuilderSpace::DataType::bool_t;
				}
			};

			class number final :public jsonBuilderSpace::NV_base {
			public:
				number() :NV_base() {}

				number(const int32_t& input) {
					this->fromInt32(input);
				}

				number(const double& input) {
					this->fromDouble(input);
				}

				number(const number& input) :NV_base(input) {}

				number(number&& input) :NV_base(std::move(input)) {}

			public:
				inline void fromInt32(const int32_t& input) {
					rawStr = DC::STR::toString(input);
					type = jsonBuilderSpace::DataType::int_t;
				}

				inline void fromDouble(const double& input) {
					rawStr = DC::STR::toString(input);
					type = jsonBuilderSpace::DataType::double_t;
				}
			};

			template <typename T>
			inline T getNull() {//获得一个内含null的value或number(类型通过模板参数指定)
				static_assert(std::is_same<number, typename std::decay<T>::type>::value || std::is_same<value, typename std::decay<T>::type>::value, "input type should be jsonBuilder::value or jsonBuilder::number");
				T rv;
				rv.fromNull();
				return rv;
			}

			class object {
			public:
				object() = default;

				object(const object&) = default;

				object(object&&) = default;

				virtual ~object() = default;

			public:
				object& operator=(const object&) = default;

				object& operator=(object&&) = default;

			public:
				virtual std::string toString()const {//带有不规范缩进
					std::string returnthis("{\n");
					for (const auto& p : m_data)
						returnthis += ' ' + jsonBuilderSpace::GetJsStr(p.GetName()) + ":" + p.GetValue() + ",\n";
					if (!m_data.empty() && returnthis.size() > 2) returnthis.erase((returnthis.rbegin() + 2).base());
					returnthis += "}";
					return returnthis;
				}

				template <typename T>
				void add(const std::string& name, const T& v) {//任何带有成员函数toString的类型
															   //add时不会检查内部是否有重复的name
															   //static_assert(jsonBuilderSpace::has_member_function_toString<std::decay_t<T>>::result, "value type doesnt have member function \"std::string toString(void)const\"");
					m_data.emplace_back(name, v.toString());
				}

				inline bool erase(const std::string& name) {//input key/name
					decltype(m_data.begin()) fres = std::find_if(m_data.begin(), m_data.end(), [&name](const jsonBuilderSpace::JSKeyValuePair& item) {
						if (item.GetName() == name) return true;
						return false;
					});
					if (fres == m_data.end()) return false;
					m_data.erase(fres);
					return true;
				}

				inline jsonBuilderSpace::JSKeyValuePair& get(const std::string& name) {
					decltype(m_data.begin()) fres = std::find_if(m_data.begin(), m_data.end(), [&name](const jsonBuilderSpace::JSKeyValuePair& item) {
						if (item.GetName() == name) return true;
						return false;
					});
					if (fres == m_data.end()) throw DC::Exception("jsonBuilder::object::get", "cannot find name");
					return *fres;
				}

				inline void clear() {
					m_data.clear();
				}

			protected:
				std::vector<jsonBuilderSpace::JSKeyValuePair> m_data;
			};

			class array final :public object{
			public:
				array() = default;

				array(const array& input) : object(input) {}

				array(array&& input) : object(std::move(input)) {}

			public:
				array& operator=(const array&) = default;

				array& operator=(array&&) = default;

			public:
				virtual std::string toString()const override {//带有不规范缩进
					std::string returnthis("[\n");
					for (const auto& p : m_data)
						returnthis += ' ' + p.GetValue() + ",\n";
					if (!m_data.empty() && returnthis.size() > 2) returnthis.erase((returnthis.rbegin() + 2).base());
					returnthis += "]";
					return returnthis;
				}

				template <typename T>
				void add(const T& v) {//任何带有成员函数toString的类型
									  //static_assert(jsonBuilderSpace::has_member_function_toString<std::decay_t<T>>::result, "value type doesnt have member function \"std::string toString(void)const\"");
					m_data.emplace_back(std::string(), v.toString());
				}

				template <typename T>
				void add(const std::string& name, const T& v) {//任何带有成员函数toString的类型
															   //add时不会检查内部是否有重复的name
															   //static_assert(jsonBuilderSpace::has_member_function_toString<std::decay_t<T>>::result, "value type doesnt have member function \"std::string toString(void)const\"");
					m_data.emplace_back(name, v.toString());
				}
			};

		}

	}

}

#endif
