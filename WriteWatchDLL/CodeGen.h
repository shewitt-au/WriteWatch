// CodeGen.h
//
#pragma once
#include <stdint.h>

class CodeGen
{
public:
	CodeGen(uint8_t* ip) : m_ip(ip) {}

	inline uint8_t* get_next() const;
	inline void set_next(uint8_t* pNext);

	inline void byte(uint8_t v);
	inline void bytes(const uint8_t* vs, size_t n);

	template <typename T>
	void value(const T &v);

	template <typename T, size_t N>
	void values(const T(&array)[N]);

	void push_volatile_regs();
	void pop_volatile_regs();
	inline ULONG_PTR pointer(ULONG_PTR pFn);
	inline void call_indirect(ULONG_PTR pFn);
	

private:
	uint8_t* m_ip;
	static const uint8_t m_push[];
	static const uint8_t m_pop[];
};

//
// Inline function definitions
//

uint8_t* CodeGen::get_next() const
{
	return m_ip;
}

void CodeGen::set_next(uint8_t* pNext)
{
	m_ip = pNext;
}

void CodeGen::byte(uint8_t v)
{
	*(m_ip++) = v;
}

void CodeGen::bytes(const uint8_t* vs, size_t n)
{
	memcpy(m_ip, vs, n);
	m_ip += n;
}

template <typename T>
void CodeGen::value(const T &v)
{
	*(T*)m_ip = v;
	m_ip += sizeof(T);
}

template <typename T, size_t N>
void CodeGen::values(const T(&array)[N])
{
	memcpy(m_ip, array, N * sizeof(T));
	m_ip += N * sizeof(T);
}

ULONG_PTR CodeGen::pointer(ULONG_PTR pFn)
{
	*(ULONG_PTR*)m_ip = pFn;
	auto old = m_ip;
	m_ip += sizeof(ULONG_PTR);
	return (ULONG_PTR)old;
}

void CodeGen::call_indirect(ULONG_PTR pFn)
{
	// call qword[32rel]; FF 15
	WORD rel = (WORD)((ULONG_PTR)pFn-(ULONG_PTR)(m_ip+6));
	byte(0xff);
	byte(0x15);
	value(rel);
}
