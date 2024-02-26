#include "Neo.h"
#include "SHAD.h"
#include "StringUtils.h"

SHAD::SHAD(const string &path, int _tabsize) :  root(0), tabsize(_tabsize)
{
	LoadResource(path,2);
}

SHAD::SHAD(int _tabsize) : tabsize(_tabsize)
{
	root = new SHAD_Node;
}

SHAD::SHAD(const char *pMem, int memsize, int tabsize)
{
	Parse(pMem, memsize, tabsize);
}

SHAD::~SHAD()
{
	if (root)
	{
		root->DeleteChildren();
		delete root;
	}
}

bool SHAD::Write(const string &path, const string &titleComment)
{
	FileManager &fm = FileManager::Instance();
	FileHandle fh;
	if (fm.StreamWriteBegin(fh, path))
	{
		if (!titleComment.empty())
			fm.StreamWrite(fh, STR("#{}\n",titleComment));

		int indent = 0;
		if (root)
		{
			for (int i=0; i<root->GetChildCount(); i++)
			{
				root->GetChild(i)->Write(fh, indent);
			}
		}
		fm.StreamWriteEnd(fh);
		return true;
	}
	return false;
}

void SHAD_Node::Write(FileHandle fh, int indent) const
{
	FileManager &fm = FileManager::Instance();
	for (int i=0; i<indent; i++)
		fm.StreamWrite(fh, "\t");

	if (StringContainsAny(name, ":#,"))
		fm.StreamWrite(fh, STR("\"{}\"", name));
	else
		fm.StreamWrite(fh, name);

	if (values.size() > 0)
	{
		fm.StreamWrite(fh, ": ");
		for (u32 i=0; i<values.size()-1; i++)
		{
			if (StringContainsAny(values[i], ":#,"))
				fm.StreamWrite(fh, STR("\"{}\",", values[i]));
			else
				fm.StreamWrite(fh, STR("{},", values[i]));
		}

		if (StringContainsAny(values[values.size() - 1],":#,"))
			fm.StreamWrite(fh, STR("\"{}\"", values[values.size() - 1]));
		else
			fm.StreamWrite(fh, STR("{}", values[values.size() - 1]));
	}
	fm.StreamWrite(fh, "\r\n");

	for (u32 i=0; i<children.size(); i++)
	{
		children[i]->Write(fh, indent+1);
	}

	if (isHeading)
		fm.StreamWrite(fh, "\r\n");
}

void SHAD::LoadResource(const string &path, int tabsize)
{
	MemBlock filemem;
	if (FileManager::Instance().Read(path, filemem))
	{
		//DMLOG("Parse SHAD File: %s, size: %d",path.CStr(),size);
		Parse((char *)filemem.Mem(), (int)filemem.Size(), tabsize);
	}
}

const char *FastTokenizer::TokenizeString(const char * &pIn, char * &pOut, const char *pOutEnd, const char *pEnd, char endDelimiter)
{
	const char *pString = pOut;
	const char *pLastAdded = pOut;

	bool inQuotes=false;
	while (pIn < pEnd && *pIn != 13 && (inQuotes || (*pIn != endDelimiter && *pIn != '#')))
	{
		if (*pIn == '"')
		{
			inQuotes = !inQuotes;
			pIn++;
		}
		else if (!inQuotes && *pIn == '&')
		{
			if (strncmp(pIn, "&quot;", 6) == 0)
			{
				*pOut++ = '"';
				pIn += 6;
			}
			else if (strncmp(pIn, "&cr;", 4) == 0)
			{
				*pOut++ = '\n';
				pIn += 4;
			}
			else if (strncmp(pIn, "&amp;", 4) == 0)
			{
				*pOut++ = '&';
				pIn += 4;
			}
			else
			{
				*pOut++ = *pIn++;
			}
		}
		else
		{
			*pOut++ = *pIn++;
			if (inQuotes)
				pLastAdded = pOut;
		}
		Assert(pOut<pOutEnd, "SHAD file block size too big!");
	}

	// strip off dead space...
	char *pStringEnd = pOut;
	while (pStringEnd > pLastAdded && (pStringEnd[-1] == ' ' || pStringEnd[-1] == '\t' || pStringEnd[-1] == 13 || pStringEnd[-1] == 10))
		pStringEnd--;
	*pStringEnd++ = 0;
	pOut = pStringEnd;

	// skip delimiter
	if (*pIn == endDelimiter)
		pIn++;

	// skip dead space
	while (pIn<pEnd && (*pIn == ' ' || *pIn == '\t'))
		pIn++;

	return pString;
}

FastTokenizer::~FastTokenizer()
{
	delete pStringData;
	for (unsigned int i = 0; i < lines.size(); i++)
		lines[i].tokens.clear();
}

FastTokenizer::FastTokenizer(const char *pData, u32 size)
{
	pStringData = new char[size];
	char *pOut = pStringData;
	char *pOutEnd = pStringData+size;

	const char *pIn = pData;
	const char *pEnd = pData+size;
	while (pIn<pEnd && *pIn)
	{
		// parse line
		FastTokenizer_LineData ld;
		ld.indent = 0;

		// count the indents
		while (pIn < pEnd && (*pIn == ' ' || *pIn == '\t'))
		{
			ld.indent++;
			pIn++;
		}

		// comment? just skip the line
		if (*pIn == '#')
		{
			while (pIn < pEnd && (*pIn != 13 && *pIn != 10))
				pIn++;
			while (pIn < pEnd && (*pIn == 13 || *pIn == 10))
				pIn++;
			continue;
		}

		// ok, this is the first char of the first token...  ends with # or : or EOL
		ld.AddToken(TokenizeString(pIn, pOut, pOutEnd, pEnd, ':'));

		// are there more?
		while (pIn<pEnd && (*pIn != 13 && *pIn != 10) && *pIn != '#')
			ld.AddToken(TokenizeString(pIn, pOut, pOutEnd, pEnd, ','));

		// skip comment
		if (*pIn == '#')
		{
			while (pIn<pEnd && (*pIn != 13 && *pIn != 10))
				pIn++;
		}

		// skip EOL
		while (pIn<pEnd && (*pIn == 13 || *pIn == 10))
			pIn++;

		// thats it. we got the line and all its stringy goodness
		lines.push_back(ld);
	}
}

void SHAD::CreateNodeFromTokenizer( SHAD_Node &parentNode, FastTokenizer &ft, int &lineIdx)
{
	FastTokenizer_LineData &line = ft.lines[lineIdx];
	int indent = line.indent;

	SHAD_Node *pNode = new SHAD_Node();
	pNode->SetParent(&parentNode);
	pNode->SetIndent(indent);
	pNode->SetName(line.Token(0));
	for (int i=1; i<line.GetCount(); i++)
		pNode->AddString(line.Token(i));
	lineIdx++;

	while (lineIdx < (int)ft.lines.size() && ft.lines[lineIdx].indent > indent)
		CreateNodeFromTokenizer(*pNode, ft, lineIdx);
}

void SHAD::Parse( const char *data, int size, int _tabsize )
{
	tabsize = _tabsize;
	//const char* delimiters = "\n";

	FastTokenizer ft(data,size);

	int lineIdx = 0;
	int indent = -1;

	root = new SHAD_Node();
	root->SetIndent(indent);
	root->SetName("root");

	while (lineIdx < (int)ft.lines.size())
		CreateNodeFromTokenizer(*root, ft, lineIdx);
}


void SHAD_Node::DeleteChildren()
{
	for (unsigned int i=0; i<children.size(); i++) 
	{ 
		children[i]->DeleteChildren(); 
		delete children[i]; 
	}
	children.clear(); 
}



//===========================================================================
//===========================================================================

SHADReader::SHADReader(const string &path)
{
    m_tokenizeBuffer = 0;
    root = 0;
	m_filename=path;
	m_heapMem=0;
	MemBlock filemem;
	if (FileManager::Instance().Read(path, filemem) != 0)
	{
        m_tokenizeBuffer = new char[256 * 1024];
		Parse((char *)filemem.Mem(), (int)filemem.Size());
        delete[] m_tokenizeBuffer;
        m_tokenizeBuffer = 0;
	}
}

SHADReader::SHADReader(const string &name, const char *pMem, int memSize)
{
	m_ownMemory = false;
	m_filename=name;
	m_heapMem=0;
	m_tokenizeBuffer = new char[256 * 1024];
	Parse(pMem, memSize);
}

SHADReader::~SHADReader()
{
	if (m_ownMemory)
		delete[] m_heapMem;
	delete[] m_tokenizeBuffer;
}

u8 *SHADReader::FastAlloc(int size)
{
	// we can't grow because everything is pointing inside our memory... so assert...
	// if it becomes a problem getting the size right, we should us offsets in the nodes instead of pointers...
	Assert(m_heapUsage+size <= m_heapSize, STR("SHAD Reader out of memory: FileMem {}, BufferSize {}  (Nodes {}, Values {})",m_fileSize,m_heapSize,m_nodeCount,m_valueCount));

	u8 *allocedMem = m_heapMem + m_heapUsage;
	m_heapUsage = (m_heapUsage+size+3)&(~3);
	return allocedMem;
}

SHADReader_Node *SHADReader::AllocNode()
{
	SHADReader_Node *node = (SHADReader_Node *)FastAlloc(sizeof(SHADReader_Node));
	node->parent = 0;
	node->name = 0;
	node->childCount = 0;
	node->children = 0;
	node->next = 0;
	node->valueCount = 0;
	node->values = 0;
	m_nodeCount++;
	return node;
}

u8 *SHADReader::DupMem(u8 *source, int size)
{
	u8 *mem = FastAlloc(size);
	memcpy(mem, source, size);
	return mem;
}


void SHADReader::Parse( const char *data, int size )
{
	m_bracketIndent = 0;
	m_currentLine = 1;
	m_errorsLogged = 0;

	if (m_heapMem)
		delete[] m_heapMem;

	// SHAD file shouldn't take more than twice the source file memory plus some overhead for small files...
	m_heapSize = size*6+10000;
	m_heapUsage = 0;
	m_heapMem = new u8[m_heapSize];

	// we don't know the tabsize yet...
	m_tabSize = -1;

	m_nodeCount = 0;
	m_valueCount = 0;

	root = AllocNode();
	root->indent = -1;
	m_fileMem = data;
	m_fileSize = size;
	m_fileParsed = 0;

	SHADReader_Node *firstChild = ReadNode();
	if (firstChild)
		ParseNode(root,firstChild);
}

SHADReader_Node *SHADReader::ReadNode()
{
	while (m_fileParsed < m_fileSize)
	{
		// grab a name
		int indent;
		const char *name;
		if (!ParseName(name, indent))
			continue;

		if (indent != 0 && m_bracketIndent == 0)
		{
			if (m_tabSize == -1)
				m_tabSize = indent;
			if ((indent % m_tabSize) != 0 && m_errorsLogged<10)
			{
				LOG(SHAD, STR("{}({}): has bad indent of {} (should be multiple of first indent: {})!", m_filename, m_currentLine, indent, m_tabSize));
				m_errorsLogged++;
			}
		}

		SHADReader_Node *node = AllocNode();
		node->name = name;
		node->indent = (m_bracketIndent != 0) ? m_bracketIndent : indent;

		vector<const char *> valuelist;

		// get the values - we support up to 1024 values...
		int valueCount = 0;
		const char *currValue;
		while (ParseValue(currValue))
		{
			valuelist.push_back(currValue);
			valueCount++;
		}

		// copy the values in
		if (valueCount > 0)
		{
			node->valueCount = valueCount;
			node->values = (const char **)DupMem((u8*)&valuelist[0], valueCount*sizeof(const char *));
			m_valueCount += valueCount;
		}

		if (m_fileMem[m_fileParsed] == '{')
		{
			node->indent = m_bracketIndent;
			m_bracketIndent++;
			m_fileParsed++;
		}

		//DMLOG("ReadNode: %s(%d), values %d (%s)",node->name,node->indent,node->valueCount,node->valueCount>0?node->values[0]:"");
		return node;
	}
	return 0;
}

SHADReader_Node *SHADReader::ParseNode(SHADReader_Node *parent, SHADReader_Node *firstChild)
{
	//  scLog("ParseNode parent %s(%d),  child %s(%d)",parent->name?parent->name:"root",parent->indent,firstChild->name,firstChild->indent);

	SHADReader_Node *node = firstChild;
	SHADReader_Node *endChild = firstChild;
	while (node && node->indent > parent->indent)
	{
		if (node->indent == firstChild->indent)
		{
			// sibling...
			node->parent = parent;
			parent->childCount++;
			endChild->next = node;
			endChild = node;

			//      scLog("%s(%d) is a sibling. Parent '%s' has %d children",node->name,node->indent,parent->name?parent->name:"root",parent->childCount);

			node = ReadNode();
		}
		else if (node->indent > endChild->indent)
		{
			//scLog("%s(%d) is a child... recursing...",node->name,node->indent);

			// child of the endChild node
			node = ParseNode(endChild, node);
		}
		else
		{
			LOG(SHAD, STR("{}({}) Aborting parse: Unexpected node indent {}", m_filename, m_currentLine, node->indent));
			parent->childCount = 0;
			return 0;
		}
	}

	//  scLog("Finished parsing parent %s(%d), children %d",parent->name?parent->name:"root",parent->indent,parent->childCount);

	// alloc the parent children array pointer...
	if (parent->childCount > 0)
	{
		parent->children = (SHADReader_Node**)FastAlloc(sizeof(SHADReader_Node*)*parent->childCount);
		for (u32 i=0; i<parent->childCount; i++)
		{
			parent->children[i] = firstChild;
			firstChild = firstChild->next;
		}
	}

	return node;
}

bool SHADReader::ParseName(const char * &name, int &indent)
{
	const char *pIn = &m_fileMem[m_fileParsed];
	const char *pEnd = &m_fileMem[m_fileSize];

	// skip previous eol
	while (pIn < pEnd && (*pIn == 13 || *pIn == 10))
	{
		if (*pIn == 13)
			m_currentLine++;

		pIn++;
	}

	// count the indents
	indent = 0;
	while (pIn < pEnd && (*pIn == ' ' || *pIn == '\t'))
	{
		indent++;
		pIn++;
	}

	if (pIn >= pEnd)
	{
		m_fileParsed = (int)(pIn - &m_fileMem[0]);
		return false;
	}
	
	// comment? just skip the line. report that there was no name found...
	if (*pIn == '#' || *pIn == ';')
	{
		while (pIn < pEnd && (*pIn != 13 && *pIn != 10))
			pIn++;
		while (pIn < pEnd && (*pIn == 13 || *pIn == 10))
		{
			if (*pIn == 13)
				m_currentLine++;

			pIn++;
		}

		m_fileParsed = (int)(pIn - &m_fileMem[0]);
		return false;
	}

	// end of bracket section?
	if (*pIn == '}')
	{
		m_bracketIndent--;
		pIn++;
		m_fileParsed = (int)(pIn - &m_fileMem[0]);
		return false;
	}

	int length;
	bool hitColon;
	bool success = TokenizeString(pIn, m_tokenizeBuffer, pEnd, ':', length, hitColon);
	m_fileParsed = (int)(pIn - &m_fileMem[0]);
	if (success)
	{
		name = (const char *)DupMem((u8*)m_tokenizeBuffer, length+1);
		return true;
	}
	return false;
}

bool SHADReader::ParseValue(const char * &value)
{
	const char *pIn = &m_fileMem[m_fileParsed];
	const char *pEnd = &m_fileMem[m_fileSize];

	int length;
	bool hitComma;
	bool success = TokenizeString(pIn, m_tokenizeBuffer, pEnd, ',', length, hitComma);

	// if line ends in ',' we assume next line continues the values...
	// so just skip all white space and EOL now...
	if (hitComma)
	{
		while (pIn < pEnd && (*pIn == 13 || *pIn == 10))
			pIn++;
	}

	m_fileParsed = (int)(pIn - &m_fileMem[0]);

	if (success)
	{
		value = (const char *)DupMem((u8*)m_tokenizeBuffer, length+1);
		return true;
	}
	return false;
}

bool SHADReader::TokenizeString(const char * &pIn, char *pOut, const char *pEnd, char endDelimiter, int &length, bool &hitToken)
{
	bool validString = false;
	const char *pResult = pOut;
	const char *pLastAdded = pOut;
	hitToken = false;

	bool inQuotes=false;
	while (pIn < pEnd && (*pIn != 13 && *pIn != 10) && (inQuotes || (*pIn != endDelimiter && *pIn != '#' && *pIn != '{')))
	{
		if (*pIn == '"')
		{
			inQuotes = !inQuotes;
			pIn++;
			validString = true;
		}
		else if (!inQuotes && *pIn == '&')
		{
			if (strncmp(pIn, "&quot;", 6) == 0)
			{
				*pOut++ = '"';
				pIn += 6;
			}
			else if (strncmp(pIn, "&cr;", 4) == 0)
			{
				*pOut++ = '\n';
				pIn += 4;
			}
			else if (strncmp(pIn, "&amp;", 4) == 0)
			{
				*pOut++ = '&';
				pIn += 4;
			}
			else
			{
				*pOut++ = *pIn++;
			}
		}
		else
		{
			*pOut++ = *pIn++;
			if (inQuotes)
				pLastAdded = pOut;
		}
	}

	// strip off dead space...
	char *pStringEnd = pOut;
	while (pStringEnd > pLastAdded && (pStringEnd[-1] == ' ' || pStringEnd[-1] == '\t' || pStringEnd[-1] == 13 || pStringEnd[-1] == 10))
		pStringEnd--;
	length = (int)(pStringEnd - pResult);
	*pStringEnd++ = 0;

	// skip delimiter
	if (pIn < pEnd && *pIn == endDelimiter)
	{
		validString = true;
		hitToken = true;
		pIn++;
	}

	// skip dead space
	while (pIn<pEnd && (*pIn == ' ' || *pIn == '\t'))
		pIn++;

	return (pResult[0] != 0) || validString;
}

void SHADReader_Node::Dump()
{
	char spaces[256];
	memset(spaces,' ',255);
	string line;
	if (indent > 0)
		line = string(spaces, &spaces[indent * 2]);
	if (name)
		line+=name;
	line+=": ";
	for (u32 i=0; i<valueCount; i++)
	{
		if (*(values[i]) == 0 || strchr(values[i],',') || strchr(values[i],'{') || strchr(values[i],';') || strchr(values[i],'#'))
			line+=STR("\"%s\"",values[i]);
		else
			line+=values[i];
		if (i<valueCount-1)
			line+=", ";
	}
	if (childCount > 0)
	{
		line+=" {";
		LOG(SHAD, line);
		for (u32 i=0; i<childCount; i++)
			children[i]->Dump();
		string closeLine;
		if (indent > 0)
			closeLine = string(spaces, &spaces[indent * 2]);
		closeLine += "}";
		LOG(SHAD, closeLine);
	}
	else
		LOG(SHAD, line);
}
