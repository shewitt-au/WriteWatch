// CodeGen.cpp
//
#include "pch.h"
#include "CodeGen.h"

const uint8_t CodeGen::m_push[] =
{
	0x9c,		// pushfq
	0x50,		// push rax
	0x51,		// push rcx
	0x52,		// push rdx
	0x56,		// push rsi
	0x57,		// push rdi
	0x41, 0x50,	// push r8
	0x41, 0x51, // push r9
	0x41, 0x52, // push r10
	0x41, 0x53  // push r11
};

void CodeGen::push_volatile_regs()
{
	bytes(m_push, sizeof(m_push));
}

const uint8_t CodeGen::m_pop[] =
{
	0x41, 0x5b,   // pop r11
	0x41, 0x5a,   // pop r10
	0x41, 0x59,   // pop r9
	0x41, 0x58,   // pop r8
	0x5f,         // pop rdi
	0x5e,         // pop rsi
	0x5a,         // pop rdx
	0x59,         // pop rcx
	0x58,		  // pop eax
	0x9d		  // popfq
};

void CodeGen::pop_volatile_regs()
{
	bytes(m_pop, sizeof(m_pop));
}
