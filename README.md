# vc_map_parser

test 107669 line read time 0.113 sec


How to use?

	CMapInfo info;
  MAPParse(&info, filepath)

example
----------------------------
#include "MAPParser.h"
using namespace MAPParser;

int main()
{
	CMapInfo info;
	if(MAPParse(&info, "E:/sourcefolder/trunk/application.map") == Parse_Success) // Parse_Success is 0
  {
    //read ok
    return 1;
  }
  //fail
  return 0;
}
----------------------------

CMapInfo structure

  class CMapInfo
	{
	public:
		CMapInfo();
		~CMapInfo();
		CMemoryPool *mempool;
		char *title;                        //first line string
		char *timestampstr;                 //timestamp string exam "Wed Sep 28 13:06:00 2016"
		TimeStampType timestamp;            //timestamp value.  TimeStampType is unsigned int
		OffsetType preferredaddress;        //OffsetType is unsigned int
		std::vector<CBasePos*> basepos;
		std::vector<CSymbolPos*> symbolpos;
		std::vector<CSymbolPos*> staticpos;
		SectionType entrysection;           //SectionType is unsigned int
		OffsetType entryoffset;
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


