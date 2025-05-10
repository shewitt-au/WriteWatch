// CodeGen.h
//
#pragma once
#include <stdint.h>
#include <vector>
#include <optional>
#include <cassert>
#include <unordered_map>

struct Label
{
	Label(uint8_t* pAddress=nullptr): address(pAddress) {}

	uint8_t* address;
	std::vector<uint8_t*> branches;
};

class Labels
{
public:
	Labels() {}

	void add_label(int label, uint8_t* pAddress)
	{
		m_labels[label].address = pAddress;
	}

	void add_branch(int label, uint8_t* pBranch)
	{
		m_labels[label].branches.push_back(pBranch);
	}

	std::optional<Label> find(int label)
	{
		auto i = m_labels.find(label);
			return std::optional<Label>();
		return std::optional<Label>(i->second);
	}

	void patch()
	{
		for (auto l : m_labels)
		{
			assert(l.second.address); // No label for branch

			for (auto b : l.second.branches)
			{
				int64_t offset = l.second.address-(b+2);
				assert(offset>=-128 && offset<=127); // Branch out of range (THIS CHECK IS FLAWED).
				*(b+1) = (uint8_t)offset;
			}
		}
	}

private:
	std::unordered_map<int, Label> m_labels;
};

class CodeGen
{
public:
	CodeGen(uint8_t* ip) : m_ip(ip) {}

	void jne(int label)
	{
		m_labels.add_branch(label, m_ip);
		// 75 <8-bit rel>; jne <8-bit rel>
		byte(0x75);
		byte((uint8_t)0);
	}

	void label(int label, uint8_t* pAddress)
	{
		m_labels.add_label(label, pAddress);
	}

	void patch()
	{
		m_labels.patch();
	}

	inline uint8_t* get_next() const;
	inline void set_next(uint8_t* pNext);

	inline void byte(uint8_t v);
	inline void bytes(const uint8_t* vs, size_t n);

	template <typename T>
	ULONG_PTR value(const T &v);

	template <typename T, size_t N>
	void values(const T(&array)[N]);

	void push_volatile_regs();
	void pop_volatile_regs();
	inline ULONG_PTR pointer(ULONG_PTR pFn);
	inline void call_indirect(ULONG_PTR pLoc);
	inline void param1(DWORD64 param);
	inline void param2(DWORD64 param);
	inline void param2_rel_addr(DWORD64 param);
	inline void param3(DWORD param);
	inline void param4(DWORD64 param);
	inline void jmp_indirect(ULONG_PTR pLoc);
	inline void breakpoint();
	inline ULONG_PTR variable(size_t sz);

private:
	uint8_t* m_ip;
	static const uint8_t m_push[];
	static const uint8_t m_pop[];

	Labels m_labels;
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
ULONG_PTR CodeGen::value(const T &v)
{
	*(T*)m_ip = v;
	auto old = m_ip;
	m_ip += sizeof(T);
	return (ULONG_PTR)old;
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

inline void CodeGen::param2_rel_addr(ULONG_PTR address)
{
	// lea edx, [rip + my_label]; 8D 15 <32-bit pc rel>
	DWORD rel = (DWORD)(address - (ULONG_PTR)(m_ip + 6));
	byte(0x8d);
	byte(0x15);
	value(rel);
}

void CodeGen::param3(DWORD param)
{
	// mov  r8d, <imm64>; 41 B8
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
	DWORD rel = (DWORD)(pLoc - (ULONG_PTR)(m_ip+6));
	byte(0xff);
	byte(0x25);
	value(rel);
}

void CodeGen::breakpoint()
{
	byte(0xcc);
}

ULONG_PTR CodeGen::variable(size_t sz)
{
	memset((void*)m_ip, 0, sz);
	auto old = m_ip;
	m_ip += sizeof(ULONG_PTR);
	return (ULONG_PTR)old;
}

// cmp dword [rip + 0], 0x12345678; 81 3D 00 00 00 00 78 56 34 12
