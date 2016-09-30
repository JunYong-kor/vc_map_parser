#include "MAPParser.h"
// #include <windows.h>
// #include <imagehlp.h>
// 
// #pragma comment (lib, "DbgHelp.lib")

#define RESERVESIZE 65536
#define NAMEBUFSIZE 100000

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#pragma warning(disable:4996)

static inline char *nextpos(char *pos)
{
	//find whitespace 
	while(*pos == ' ' || *pos == '\r' || *pos == '\n'
		/*|| *pos == '\t' || *pos == '\b' || *pos == '\0' || *pos == 1*/)
	{
		++pos;
	}
	return pos;
}
static inline char *find_crlf(char *str)
{
	for(; *str; ++str)
	{
		if(*str == '\r') //|| *str == '\n')
		{
			return str;
		}
	}
	return nullptr;
}

template<typename T>
static inline char *hextodeclow(char *str, T *result)
{
	*result = 0;
	for(unsigned char ch = 0;;++str)
	{
		if('a' <= *str && *str <= 'f')
		{
			ch = *str - 'W'; //'W' is 'a' - 10
		}
		else if('0' <= *str && *str <= '9')
		{
			ch = *str - '0';
		}
		else
		{
			break;
		}
		*result = ch + (*result << 4);
	}
	return str;
}

MAPParser::CMapInfo::CMapInfo()
	: title(0)
	, timestampstr(0)
	, timestamp(0)
	, preferredaddress(0)
	, entrysection(0)
	, entryoffset(0)
{
	mempool = new CMemoryPool();
	staticpos.reserve(RESERVESIZE);
}

MAPParser::CMapInfo::~CMapInfo()
{
	delete mempool;
}

MAPParser::ParseResult MAPParser::MAPParse(CMapInfo *result, const char *path
	, unsigned flag)
{
	MAPParser::ParseResult returnvalue = MAPParser::Parse_Success;
	if(result == nullptr || path == nullptr)
	{
		return Invalid_Parameter;
	}

	HANDLE hFile = CreateFileA(path, FILE_GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return FileLoadFail;
	}

	{
		HANDLE hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			returnvalue = FileMappingFail;
			goto EndMapping;
		}

		{
			LPVOID mapViewOfFile = 0;
			mapViewOfFile = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
			if(mapViewOfFile == nullptr)
			{
				returnvalue = MapViewingFail;
				goto EndViewing;
			}

			{
				char *readpos = (char*)mapViewOfFile;
				char namebuf[NAMEBUFSIZE];
				char unnamebuf[NAMEBUFSIZE];

				//Read ModuleName
				readpos = nextpos(readpos);
				char *strend = find_crlf(readpos);
				size_t bufsize = strend - readpos;
				result->title = result->mempool->alloc(bufsize + 1);
				memcpy(result->title, readpos, bufsize);
				result->title[bufsize] = 0;

				//Read TimeStamp
				readpos = strstr(strend, "is");
				readpos += 2;
				readpos = nextpos(readpos);
				readpos = hextodeclow(readpos, &result->timestamp);
				readpos = strchr(readpos, '(') + 1;
				strend = strchr(readpos, ')');
				bufsize = strend - readpos;
				result->timestampstr = result->mempool->alloc(bufsize + 1);
				memcpy(result->timestampstr, readpos, bufsize);
				result->timestampstr[bufsize] = 0;

				//Read Preferred load address
				readpos = strstr(strend, "is");
				readpos += 2;
				readpos = nextpos(readpos);
				readpos = hextodeclow(readpos, &result->preferredaddress);

				//Read base position
				readpos = find_crlf(readpos);
				readpos = nextpos(readpos);

				if(readpos[0] == 'S' && readpos[3] == 'r') // Start. not Static
				{
					CBasePos *data = nullptr;

					do
					{
						readpos = find_crlf(readpos);
						readpos = nextpos(readpos);

						if(*readpos == 'A' || readpos[4] == 't' || readpos[1] == 'n')
						{
							break;
						}

						data = (CBasePos*)result->mempool->alloc(sizeof(CBasePos));
						readpos = hextodeclow(readpos, &data->section);
						readpos = hextodeclow(readpos + 1, &data->offset);
						readpos = nextpos(readpos);
						readpos = hextodeclow(readpos, &data->length);
						readpos += 2;
						strend = strchr(readpos, ' ');
						bufsize = strend - readpos;
						data->name = result->mempool->alloc(bufsize + 1);
						memcpy(data->name, readpos, bufsize);
						data->name[bufsize] = 0;
						readpos = nextpos(strend);
						data->classtype = (*readpos == 'D' ? DATA : CODE);
						result->basepos.push_back(data);
					} while(true);
				}

				//Read function, varable pos
				//Read static pos
				while(*readpos == 'A' || *readpos == 'S')
				{
					std::vector<CSymbolPos*>& supply
						= (*readpos == 'S' ? result->staticpos : result->symbolpos);
					CSymbolPos *data = nullptr;

					do
					{
						readpos = find_crlf(readpos);
						readpos = nextpos(readpos);
						if(*readpos == 'S' || readpos[1] == 'n')
						{
							break;
						}

						data = (CSymbolPos*)result->mempool->alloc(sizeof(CSymbolPos));

						readpos = hextodeclow(readpos, &data->section);
						readpos = hextodeclow(readpos + 1, &data->offset);
						readpos = nextpos(readpos);
						strend = strchr(readpos, ' ');
						bufsize = strend - readpos;

						if(flag)
						{
							memcpy(namebuf, readpos, bufsize);
							namebuf[bufsize] = 0;

							UnDecorateSymbolName(namebuf, unnamebuf, NAMEBUFSIZE, flag);
							bufsize = strlen(unnamebuf);
							data->name = result->mempool->alloc(bufsize + 1);
							memcpy(data->name, unnamebuf, bufsize);
						}
						else
						{
							data->name = result->mempool->alloc(bufsize + 1);
							memcpy(data->name, readpos, bufsize);
						}
						data->name[bufsize] = 0;
						readpos = nextpos(strend);
						readpos = hextodeclow(readpos, &data->rvabase);

						if(readpos[1] == 'f')
						{
							if(readpos[3] == 'i')
							{
								data->flag = INLINE_FUNC;
							}
							else
							{
								data->flag = FUNCTION;
							}
						}
						else
						{
							data->flag = SYMBOLFLAG_NONE;
						}

						readpos += 5;
						strend = find_crlf(readpos);
						bufsize = strend - readpos;
						data->object = result->mempool->alloc(bufsize + 1);
						memcpy(data->object, readpos, bufsize);
						data->object[bufsize] = 0;
						supply.push_back(data);
					} while(true);
				}

				//Read entry point
				if(*readpos == 'e')
				{
					readpos = strstr(readpos, "at");
					readpos += 2;
					readpos = nextpos(readpos);
					readpos = hextodeclow(readpos, &result->entrysection);
					readpos = hextodeclow(readpos + 1, &result->entryoffset);
				}
			}
			UnmapViewOfFile(mapViewOfFile);
			EndViewing:;
		}
		CloseHandle(hFileMap);
		EndMapping:;
	}
	CloseHandle(hFile);
	return returnvalue;
}
