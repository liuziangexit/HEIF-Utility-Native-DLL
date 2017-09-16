#pragma once
#ifndef liuzianglib_TypeInfo
#define liuzianglib_TypeInfo
#include <iostream>
//Version 2.4.21V30
//20170914

namespace DC {

	namespace TypeInfoSpace {

		enum class Detail
		{
			Type,
			Pointer,
			LvalueReference,
			RvalueReference,
			Array
		};

	}

	template <typename T>
	class TypeInfo {
	public:
		TypeInfo() {
			Detail_ = TypeInfoSpace::Detail::Type;
		}

		typedef T BaseType;
		typedef T BottomType;
		TypeInfoSpace::Detail Detail_;
	};

	template <typename T>
	class TypeInfo<T*> {
	public:
		TypeInfo() {
			Detail_ = TypeInfoSpace::Detail::Pointer;
		}

		typedef T BaseType;
		typedef typename TypeInfo<BaseType>::BottomType BottomType;
		TypeInfoSpace::Detail Detail_;
	};

	template <typename T, std::size_t SizeTy>
	class TypeInfo<T[SizeTy]> {
	public:
		TypeInfo() {
			Detail_ = TypeInfoSpace::Detail::Array;
		}

		typedef T BaseType;
		typedef typename TypeInfo<BaseType>::BottomType BottomType;
		TypeInfoSpace::Detail Detail_;
	};

	template <typename T>
	class TypeInfo<T&> {
	public:
		TypeInfo() {
			Detail_ = TypeInfoSpace::Detail::LvalueReference;
		}

		typedef T BaseType;
		typedef typename TypeInfo<BaseType>::BottomType BottomType;
		TypeInfoSpace::Detail Detail_;
	};

	template <typename T>
	class TypeInfo<T&&> {
	public:
		TypeInfo() {
			Detail_ = TypeInfoSpace::Detail::RvalueReference;
		}

		typedef T BaseType;
		typedef typename TypeInfo<BaseType>::BottomType BottomType;
		TypeInfoSpace::Detail Detail_;
	};

}

#endif
