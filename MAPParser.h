#pragma once
#include <vector>

#pragma pack( push, 1 )
namespace MAPParser
{
	typedef unsigned TimeStampType;
	typedef unsigned OffsetType;
	typedef unsigned LengthType;
	typedef unsigned SectionType;

	const int MEMPOOLSIZE = 10000000;

	enum ParseResult
	{
		Parse_Success = 0,
		Invalid_Parameter = -1,
		FileLoadFail = -2,
		FileMappingFail = -3,
		MapViewingFail = -4
	};

	enum CBasePosType
	{
		DATA,
		CODE
	};

	enum CSymbolFlag
	{
		SYMBOLFLAG_NONE = 0,
		FUNCTION = 1,
		INLINE_FUNC = 2
	};

	struct CBasePos
	{
		SectionType section;
		OffsetType offset;
		LengthType length;
		char *name;
		CBasePosType classtype;
	};

	struct CSymbolPos
	{
		SectionType section;
		OffsetType offset;
		char *name;
		OffsetType rvabase;
		char *object;
		CSymbolFlag flag;
	};

	class CMemoryPool
	{
		char *base;
		char *last;
		size_t allocsize;
		CMemoryPool *next;
	public:
		CMemoryPool(size_t poolsize = MEMPOOLSIZE)
		{
			base = (char*)malloc(poolsize);
			last = base;
			allocsize = poolsize;
			next = 0;
		}
		~CMemoryPool()
		{
			free(base);
			if(next)
			{
				delete next;
			}
		}

		char *alloc(size_t size)
		{
			if(allocsize < size)
			{
				if(next == nullptr)
				{
					next = new CMemoryPool(MEMPOOLSIZE < size ? size : MEMPOOLSIZE);
				}
				return next->alloc(size);
			}

			char *mem = last;
			allocsize -= size;
			last += size;
			return mem;
		}
	};

	class CMapInfo
	{
	public:
		CMapInfo();
		~CMapInfo();
		CMemoryPool *mempool;
		char *title;
		char *timestampstr;
		TimeStampType timestamp;
		OffsetType preferredaddress;
		std::vector<CBasePos*> basepos;
		std::vector<CSymbolPos*> symbolpos;
		std::vector<CSymbolPos*> staticpos;
		SectionType entrysection;
		OffsetType entryoffset;
	};

	ParseResult MAPParse(CMapInfo *result, const char *path);
}
#pragma pack( pop )
