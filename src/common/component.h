#pragma once
#include <cstdint>

using uint128_t = unsigned __int128;

namespace common
{
	/* This doesn't serve any functional purpose (for now) aside from
	   providing a common interface to call function pointers */
	struct Component
	{
	};

	/* Nothing special about this, I just found this
		by carefully studying the memory map */
	constexpr uint32_t HANDLER_PAGE_SIZE = 0x80;

	/* Any class that inherits from Component can use these */
	template <typename T>
	using Reader = T(Component::*)(uint32_t);

	template <typename T>
	using Writer = void(Component::*)(uint32_t, T);

	struct HandlerBase {};

	template <typename T>
	struct Handler : public HandlerBase
	{
		Handler() = default;
		Handler(Component* comp, Reader<T> r, Writer<T> w) :
			c(comp), reader(r), writer(w) {}

		Component* c = nullptr;
		Reader<T> reader = nullptr;
		Writer<T> writer = nullptr;

		inline void operator()(uint32_t addr, T data) { (c->*writer)(addr, data); }
		inline T operator()(uint32_t addr) { return (c->*reader)(addr); }
	};
}