#pragma once
#include <cstdint>

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
	using Reader = uint64_t(Component::*)(uint32_t);
	using Writer = void(Component::*)(uint32_t, uint64_t);

	struct Handler
	{
		Handler() = default;
		Handler(Component* comp, Reader r, Writer w) :
			c(comp), reader(r), writer(w) {}

		Component* c = nullptr;
		Reader reader = nullptr;
		Writer writer = nullptr;

		inline void operator()(uint32_t addr, uint64_t data) { (c->*writer)(addr, data); }
		inline uint64_t operator()(uint32_t addr) { return (c->*reader)(addr); }
	};
}