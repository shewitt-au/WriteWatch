// CodeGen.h
//
#pragma once
#include <stdint.h>

class CodeGen
{
public:
	CodeGen(uint8_t* ip) : m_ip(ip), m_ipPrev(0) {}

	inline uint8_t* get_next() const;
	inline void set_next(uint8_t* pNext);
	inline uint8_t* get_prev() const;

	inline void byte(uint8_t v);
	inline void bytes(const uint8_t* vs, size_t n);

	template <typename T>
	void value(const T &v);

	template <typename T, size_t N>
	void values(const T(&array)[N]);

	void push_volatile_regs();
	void pop_volatile_regs();
	inline ULONG_PTR pointer(ULONG_PTR pFn);
	inline void call_indirect(ULONG_PTR pLoc);
	inline void param1(DWORD64 param);
	inline void param2(DWORD64 param);
	inline void param3(DWORD param);
	inline void param4(DWORD64 param);
	inline void jmp_indirect(ULONG_PTR pLoc);
	inline void breakpoint();

private:
	uint8_t* m_ip;
	uint8_t* m_ipPrev;
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

uint8_t* CodeGen::get_prev() const
{
	return m_ipPrev;
}

void CodeGen::byte(uint8_t v)
{
	m_ipPrev = m_ip;
	*(m_ip++) = v;
}

void CodeGen::bytes(const uint8_t* vs, size_t n)
{
	m_ipPrev = m_ip;
	memcpy(m_ip, vs, n);
	m_ip += n;
}

template <typename T>
void CodeGen::value(const T &v)
{
	m_ipPrev = m_ip;
	*(T*)m_ip = v;
	m_ip += sizeof(T);
}

template <typename T, size_t N>
void CodeGen::values(const T(&array)[N])
{
	m_ipPrev = m_ip;
	memcpy(m_ip, array, N * sizeof(T));
	m_ip += N * sizeof(T);
}

ULONG_PTR CodeGen::pointer(ULONG_PTR pFn)
{
	m_ipPrev = m_ip;
	*(ULONG_PTR*)m_ip = pFn;
	auto old = m_ip;
	m_ip += sizeof(ULONG_PTR);
	return (ULONG_PTR)old;
}

void CodeGen::call_indirect(ULONG_PTR pLoc)
{
	// call qword[32rel]; FF 15
	DWORD rel = (DWORD)((ULONG_PTR)pLoc-(ULONG_PTR)(m_ip+6));
	int sz = sizeof(rel);
	byte(0xff);
	byte(0x15);
	value(rel);
}

void CodeGen::param1(DWORD64 param)
{
	// move rcx, <imm64>; 48 B9
	byte(0x48);
	byte(0xb9);
	value(param);
}

void CodeGen::param2(DWORD64 param)
{
	// move rdx, <imm64>; 48 B9
	byte(0x48);
	byte(0xba);
	value(param);
}

void CodeGen::param3(DWORD param)
{
	// mov  r8d, <imm32>; 41 B8
	byte(0x41);
	byte(0xb8);
	value(param);
}

void CodeGen::param4(DWORD64 param)
{
	// mov r9, <imm64>; 49 B9
	byte(0x49);
	byte(0xb9);
	value(param);
}

void CodeGen::jmp_indirect(ULONG_PTR pLoc)
{
	// jmp qword[32rel]; FF 25
	DWORD rel = (DWORD)((ULONG_PTR)pLoc - (ULONG_PTR)(m_ip+6));
	int sz = sizeof(rel);
	byte(0xff);
	byte(0x25);
	value(rel);
}

void CodeGen::breakpoint()
{
	byte(0xcc);
}
