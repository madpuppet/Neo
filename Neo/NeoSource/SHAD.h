#pragma once

#include "Neo.h"
#include "FileSystem.h"
#include "StringUtils.h"
#include "MathUtils.h"

class SHAD_Node
{
public:
	SHAD_Node() : parent(0), indent(0), isHeading(false) {}
	void DeleteChildren();

	int GetIndent() const { return indent; }
	void SetIndent(int _indent) { indent = _indent; }

	const string &GetName() const { return name; }
	void SetName(const string &_name) { name = _name; }
	bool IsName(const string &val) const { return name == val; }

	const SHAD_Node *GetParent() const { return parent; }
	SHAD_Node *GetParent() { return parent; }
	void SetParent(SHAD_Node *_parent) { _parent->AddChild(this); }

	const SHAD_Node *GetChild(const string &_name) const
	{
		for (u32 i=0; i<children.size(); i++)
			if (children[i]->name == _name)
				return children[i];
		return 0;
	}
	SHAD_Node *GetChild(const string &_name)
	{
		for (u32 i=0; i<children.size(); i++)
			if (children[i]->name == _name)
				return children[i];
		return 0;
	}

	SHAD_Node * AddChild(SHAD_Node *pChild)
	{
		pChild->parent = this;
		children.push_back(pChild);
		return pChild;
	}
	SHAD_Node * AddChild(const string &_name)
	{
		SHAD_Node *pChild = new SHAD_Node;
		pChild->name = _name;
		pChild->parent = this;
		children.push_back(pChild);
		return pChild;
	}

	bool HasValue(const string &a_value) const
	{
		for (u32 i=0; i<values.size(); i++)
			if (values[i] == a_value)
				return true;
		return false;
	}

	// some get functions that return a specific child, or a default value if the child does not exist
	string GetChildString(const string &_name, const string &def = string()) const { const SHAD_Node *c = GetChild(_name); return c ? c->GetString() : def; }
	float GetChildFloat(const string &_name, float def = 0.0f) const { const SHAD_Node *c = GetChild(_name); return c ? c->GetFloat() : def; }
	int GetChildInt(const string &_name, int def = 0) const { const SHAD_Node *c = GetChild(_name); return c ? c->GetInt() : def; }
	bool GetChildBool(const string &_name, bool def = false) const { const SHAD_Node *c = GetChild(_name); return c ? c->GetBool() : def; }
	vec2 GetChildVector2(const string &_name, const vec2 &def = vec2(0,0)) const { const SHAD_Node *c = GetChild(_name); return c ? c->GetVector2() : def; }
	vec3 GetChildVector3(const string &_name, const vec3 &def = vec3(0, 0, 0)) const { const SHAD_Node *c = GetChild(_name); return c ? c->GetVector3() : def; }

	SHAD_Node * AddString(const string &val) { values.push_back(val); return this; }
	SHAD_Node * AddFloat(float val)  { values.push_back(STR("%f",val)); return this;  }
	SHAD_Node * AddInt(int val) { values.push_back(STR("%d",val)); return this;}
	SHAD_Node * AddHex64(u64 val) { values.push_back(STR("%llx", val)); return this; }
	SHAD_Node * AddVector2i(const ivec2 &val) { AddInt(val.x); AddInt(val.y); return this; }
	SHAD_Node * AddVector3i(const ivec3 &val) { AddInt(val.x); AddInt(val.y); AddInt(val.z); return this; }
	SHAD_Node * AddBool(bool val) { values.push_back(val?"true":"false"); return this;}
	SHAD_Node * AddVector2(const vec2 &val) { AddFloat(val.x); AddFloat(val.y); return this;}
	SHAD_Node * AddVector3(const vec3 &val) { AddFloat(val.x); AddFloat(val.y); AddFloat(val.z);return this; }
	SHAD_Node * AddVector4(const vec4 &val) { AddFloat(val.x); AddFloat(val.y); AddFloat(val.z); AddFloat(val.w); return this; }
	SHAD_Node * AddColor(const color &val) { AddFloat(val.r); AddFloat(val.g); AddFloat(val.b); AddFloat(val.a);return this; }
	SHAD_Node * AddFloats(const vector<float> &val)  { for (u32 i=0; i<val.size(); i++) values.push_back(STR("%f",val[i]));  return this;}
	SHAD_Node * AddStrings(const vector<string> &val)  { for (u32 i=0; i<val.size(); i++) values.push_back(val[i]); return this; }

	int GetValueCount() const { return (int)values.size(); }
	string GetString(u32 index = 0) const
	{
		return values.size() > index ? values[index] : string();
	}
	bool GetBool(int index = 0) const { return StringToBool(GetString(index)); }
	float GetFloat(int index = 0) const { return StringToF32(GetString(index)); }
	int GetInt(int index = 0) const { return StringToI32(GetString(index)); }
	vec2 GetVector2(int index = 0) const { return vec2(StringToF32(GetString(index)), StringToF32(GetString(index+1))); }
	vec3 GetVector3(int index = 0) const { return vec3(StringToF32(GetString(index)), StringToF32(GetString(index+1)), StringToF32(GetString(index+2))); }
	color GetColor(int index = 0) const { return color(StringToF32(GetString(index)), StringToF32(GetString(index+1)), StringToF32(GetString(index+2)), StringToF32(GetString(index+3))); }
	color GetColor256(int index = 0) const { return color(StringToF32(GetString(index))/255.0f, StringToF32(GetString(index+1)) /255.0f, StringToF32(GetString(index+2)) /255.0f, StringToF32(GetString(index+3)) /255.0f); }

	int GetChildCount() const { return (int)children.size(); }
	const SHAD_Node *GetChild(u32 index) const { return (index < children.size()) ? children[index] : 0; }
	SHAD_Node *GetChild(u32 index) { return (index < children.size()) ? children[index] : 0; }

	void Write(FileHandle fh, int indent) const;

	SHAD_Node *SetAsHeading() { isHeading = true; return this; }

protected:
	vector<SHAD_Node*> children;
	vector<string> values;
	SHAD_Node *parent;
	string name;
	int indent;
	bool isHeading;
};


struct FastTokenizer_LineData
{
	int indent = 0;

	// NOTE: was getting 20+ second load times when connected to the debugger when using STL vector for the tokens,  so implementing my own version
	void AddToken(const char *pToken) 
	{
		tokens.push_back(pToken);
	}
	const char *Token(int index) { return tokens[index]; }
	int GetCount() { return (int)tokens.size(); }

protected:
	vector<const char*> tokens;
	friend struct FastTokenizer;
};

struct FastTokenizer
{
	char *pStringData;
	vector<FastTokenizer_LineData> lines;

	FastTokenizer(const char *pData, u32 size);
	~FastTokenizer();
	const char *TokenizeString(const char * &pIn, char * &pOut, const char *pOutEnd, const char *pEnd, char endDelimiter);
};

class SHAD
{
public:
	SHAD(int tabsize=2);
	SHAD(const string &path, int tabsize);
	SHAD(const char *pMem, int memSize, int tabsize);
	~SHAD();

	SHAD_Node *root;
	int tabsize;

	bool Write(const string &path, const string &titleComment);

protected:
	void LoadResource(const string &path, int _tabsize);
	void Parse( const char *data, int size, int _tabsize );
	void CreateNodeFromTokenizer( SHAD_Node &parentNode, FastTokenizer &ft, int &lineIdx);
};

//====================================================================================================

class SHADReader_NodeChildren 
{
public:
	SHADReader_NodeChildren(class SHADReader_Node** nodes, int size) : m_nodes(nodes), m_size(size) {}

	int size() { return m_size; }
	SHADReader_Node* operator[](int index) { Assert(index >= 0 && index < m_size, "Bad Index!"); return m_nodes[index]; }

	// Iterator class for ranged for-loop
	class Iterator {
	public:
		Iterator(SHADReader_Node** ptr) : current(ptr) {}

		SHADReader_Node* operator*() const {
			return *current;
		}

		Iterator& operator++() {
			++current;
			return *this;
		}

		bool operator!=(const Iterator& other) const {
			return current != other.current;
		}

	private:
		SHADReader_Node** current;
	};

	Iterator begin() const {
		return Iterator(m_nodes);
	}

	Iterator end() const {
		return Iterator(m_nodes + m_size);
	}

private:
	SHADReader_Node** m_nodes;
	int m_size;
};


class SHADReader_Node
{
protected:
	SHADReader_Node *parent;
	SHADReader_Node *next;      // initially build as a linked list, then pack into the children array for indexed lookup...

	const char *name;
	const char **values;
	u32 valueCount;
	SHADReader_Node **children;
	u32 childCount;
	friend class SHADReader;
	int indent;

public:
	const char *Name() const { return name; }
	string GetName() const { return string(name); }
	string GetPath() const { return (parent != 0) ? parent->GetPath() + "/" + name : name; }

	bool IsName(const string &val) const { return val == name; }
	const SHADReader_Node *GetParent() const { return parent; }
	SHADReader_Node *GetParent() { return parent; }

	const SHADReader_Node *GetChild(const string &_name) const
	{
		for (u32 i=0; i<childCount; i++)
			if (_name == children[i]->name)
				return children[i];
		return 0;
	}

	bool HasValue(const string &a_value) const
	{
		for (u32 i=0; i<valueCount; i++)
			if (a_value == values[i])
				return true;
		return false;
	}

	// some get functions that return a specific child, or a default value if the child does not exist
	string GetChildString(const string &_name, const string &def = string()) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetString() : def; }
	float GetChildFloat(const string &_name, float def = 0.0f) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetF32() : def; }
	int GetChildInt(const string &_name, int def = 0) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetI32() : def; }
	int GetChildHex(const string &_name, int def = 0) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetHex() : def; }
	bool GetChildBool(const string &_name, bool def = false) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetBool() : def; }
	vec2 GetChildVector2(const string &_name, const vec2 &def = vec2(0,0)) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetVector2() : def; }
	vec3 GetChildVector3(const string &_name, const vec3 &def = vec3(0, 0, 0)) const { const SHADReader_Node *c = GetChild(_name); return c ? c->GetVector3() : def; }

	int GetValueCount() const { return valueCount; }
	string GetString(u32 index = 0) const
	{
		return index < valueCount ? string(values[index]) : string();
	}
	const char *Value(u32 index=0) const
	{
		return index < valueCount ? values[index] : "";
	}
	u64 GetUID64(const string baseName, u32 index=0) const
	{
		string str = GetString(index);
		if (str[0] == '@')
			return StringHash64(baseName) + StringHash64(str);
		else if (str[0] == '%')
			return GenerateRandomU64();
		else
			return GetHex64(index);
	}

	bool GetBool(int index = 0) const { return StringToBool(GetString(index)); }
	f32 GetF32(int index = 0) const { return StringToF32(Value(index)); }
	i32 GetI32(int index = 0) const { return StringToI32(Value(index)); }
	u64 GetHex64(int index = 0) const { return StringToHex64(Value(index)); }
	int GetHex(int index = 0) const { return StringToHex(Value(index)); }
	vec2 GetVector2(int index = 0) const { return vec2(GetF32(index), GetF32(index+1)); }
	vec3 GetVector3(int index = 0) const { return vec3(GetF32(index), GetF32(index+1), GetF32(index+2)); }
	vec4 GetVector4(int index = 0) const { return vec4(GetF32(index), GetF32(index+1), GetF32(index+2), GetF32(index+3)); }
	vec3 GetDegVector3(int index = 0) const { return vec3(DegToRad(GetF32(index)), DegToRad(GetF32(index+1)), DegToRad(GetF32(index+2))); }
	ivec2 GetVector2i(int index = 0) const { return ivec2(GetI32(index), GetI32(index+1)); }
	ivec3 GetVector3i(int index = 0) const { return ivec3(GetI32(index), GetI32(index + 1), GetI32(index + 2)); }
	ivec4 GetVector4i(int index = 0) const { return ivec4(GetI32(index), GetI32(index + 1), GetI32(index + 2), GetI32(index + 3)); }
	color GetColour(int index = 0) const { return color(GetF32(index), GetF32(index+1), GetF32(index+2), GetF32(index+3)); }
	color GetColour256(int index = 0) const { return color(GetF32(index)/255.0f, GetF32(index+1)/255.0f, GetF32(index+2)/255.0f, GetF32(index+3)/255.0f); }
	int GetEnum(const stringlist fields, int index = 0) { return StringFindInList(GetString(index), fields); }

	SHADReader_NodeChildren GetChildren() { return SHADReader_NodeChildren(children, childCount); }
	int GetChildCount() const { return childCount; }
	const SHADReader_Node *GetChild(u32 index) const { return (index < childCount) ? children[index] : 0; }
	SHADReader_Node *GetChild(u32 index) { return (index < childCount) ? children[index] : 0; }

	void Dump();
};

class SHADReader
{
public:
	SHADReader(const string &path);
	SHADReader(const string &name, const char *pMem, int memSize);
	~SHADReader();

	SHADReader_Node *root;
	u8 *FastAlloc(int size);

protected:
	void Parse( const char *data, int size);

	string m_filename;
	const char *m_fileMem;
	int m_fileSize;
	int m_fileParsed;

	u8 *m_heapMem;
	int m_heapSize;
	int m_heapUsage;
	int m_tabSize;

	SHADReader_Node *AllocNode();
	u8 *DupMem(u8 *source, int size);

	// parse a node and its children
	// returns the next sibling, or NULL at end of file
	SHADReader_Node *ParseNode(SHADReader_Node *parent, SHADReader_Node *firstChild);

	SHADReader_Node *ReadNode();
	bool ParseName(const char * &name, int &indent);
	bool ParseValue(const char * &value);
	bool TokenizeString(const char * &pIn, char *pOut, const char *pEnd, char endDelimiter, int &length, bool &hitToken);

	int m_nodeCount;
	int m_valueCount;
	int m_errorsLogged;
	int m_currentLine;
	int m_bracketIndent;

    // temporary tokenize buffer
    char* m_tokenizeBuffer;
};

