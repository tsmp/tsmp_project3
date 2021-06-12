#pragma once

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

class trivial_encryptor
{
private:
	typedef u8 type;
	typedef void* pvoid;
	typedef const void* pcvoid;

private:
	enum
	{
		alphabet_size = u32(1 << (8 * sizeof(type)))
	};

private:
	class random32
	{
	private:
		u32 m_seed;

	public:
		IC void seed(const u32& seed)
		{
			m_seed = seed;
		}

		IC u32 random(const u32& range)
		{
			m_seed = 0x08088405 * m_seed + 1;
			return (u32(u64(m_seed) * u64(range) >> 32));
		}
	};

private:

	struct ArchiveFormat
	{
		u32 m_table_iterations;
		u32 m_table_seed;
		u32 m_encrypt_seed;
	};

	static inline ArchiveFormat m_FormatRussian;
	static inline ArchiveFormat m_FormatWorldWide;
	static inline ArchiveFormat m_CurrentFormat;

	static inline bool m_CurrentFormatWW;
	static inline type m_alphabet_back[alphabet_size];
	static inline type m_alphabet[alphabet_size];
		
public:
	IC static void FirstInitialize()
	{
		m_FormatRussian.m_table_iterations = 2048;
		m_FormatRussian.m_table_seed = 20091958;
		m_FormatRussian.m_encrypt_seed = 20031955;
				
		m_FormatWorldWide.m_table_iterations = 1024;
		m_FormatWorldWide.m_table_seed = 6011979;
		m_FormatWorldWide.m_encrypt_seed = 24031979;

		Initialize(false);
	}
private:
	IC static void Initialize(bool worldWide)
	{
		if (worldWide)
			m_CurrentFormat = m_FormatWorldWide;
		else
			m_CurrentFormat = m_FormatRussian;

		m_CurrentFormatWW = worldWide;
		
		for (u32 i = 0; i < alphabet_size; ++i)
			m_alphabet[i] = (type)i;

		random32 temp;
		temp.seed(m_CurrentFormat.m_table_seed);

		for (u32 i = 0; i < m_CurrentFormat.m_table_iterations; ++i)
		{
			u32 j = temp.random(alphabet_size);
			u32 k = temp.random(alphabet_size);

			while (j == k)
				k = temp.random(alphabet_size);

			std::swap(m_alphabet[j], m_alphabet[k]);
		}

		for (u32 i = 0; i < alphabet_size; ++i)
			m_alphabet_back[m_alphabet[i]] = (type)i;
	}

public:

	IC static void encode(pcvoid source, const u32& source_size, pvoid destination)
	{
		random32 temp;
		temp.seed(m_CurrentFormat.m_encrypt_seed);
		const u8* I = (const u8*)source;
		const u8* E = (const u8*)source + source_size;
		u8* J = (u8*)destination;
		for (; I != E; ++I, ++J)
			*J = m_alphabet[*I] ^ type(temp.random(256) & 0xff);
	}

	IC static void decode(pcvoid source, const u32& source_size, pvoid destination,bool worldWide)
	{
		if (m_CurrentFormatWW != worldWide)
			Initialize(worldWide);
		
		random32 temp;
		temp.seed(m_CurrentFormat.m_encrypt_seed);
		const u8* I = (const u8*)source;
		const u8* E = (const u8*)source + source_size;
		u8* J = (u8*)destination;

		for (; I != E; ++I, ++J)
			*J = m_alphabet_back[(*I) ^ type(temp.random(256) & 0xff)];
	}
};
