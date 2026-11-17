// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CXMLReader.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "daisy/io/CTextReader.h"
#include "CXMLReader.h"
#include "ox/core/CString.h"
#include "ox/core/CStringFunctions.h"
#include "ox/io/IXMLReader.h"
#include <math.h>
#include "daisy/other/fast_atof.h"
#include <string.h>
#include <wchar.h>

namespace ox { namespace io {} }
namespace daisy
{
namespace io
{
using namespace ox::io;


CXMLReader::CXMLReader(CTextReader* reader)
: Reader(reader), P(0), TextBegin(0), TextSize(0), CurrentNodeType(EXN_NONE)
{
	if (Reader)
	{
		Reader->grab();
		P = Reader->getText();
		TextBegin = P;
		TextSize = Reader->getTextSize();
	}
}


//! Destructor
CXMLReader::~CXMLReader()
{
	if (Reader)
		Reader->drop();
}


//! Reads forward to the next xml node.
//! \return Returns false, if there was no further node.
bool CXMLReader::read()
{
	if ((unsigned int)(P - TextBegin) < TextSize - 1 && *P != 0)
	{
		parseCurrentNode();
		//++P;
		return true;
	}

	return false;
}


void CXMLReader::parseCurrentNode()
{
    wchar_t* start = P;

	while(*P != L'<' && *P)
		++P;

	if (!*P)
		return;

	if (P - start > 2)
	{
		// we found some text
		setText(start, P);
		--P;
		return;
	}

	++P;

	switch(*P)
	{
	case L'/':
		parseClosingXMLElement(); break;
	case L'?':
		ignoreDefinition();	break;
	case L'!':
		parseComment();	break;
	default:
		parseOpeningXMLElement();
		break;
	}
}


//! sets the state that text was found
void CXMLReader::setText(wchar_t* start, wchar_t* end)
{
    ox::core::CString<wchar_t> s(start, (int)(end - start));
	NodeName = replaceSpecialCharacters(s);
	CurrentNodeType = EXN_TEXT;
}



//! ignores an xml definition like <?xml something />
void CXMLReader::ignoreDefinition()
{
	CurrentNodeType = EXN_UNKNOWN;

	while(*P != L'>')
		++P;

	++P;
}


//! parses a comment
void CXMLReader::parseComment()
{
	CurrentNodeType = EXN_COMMENT;
	P += 1;

	wchar_t *pCommentBegin = P;

	int count = 1;

	while(count)
	{
		if (*P == L'>')
			--count;
		else
		if (*P == L'<')
			++count;

		++P;
	}

	P -= 3;
	NodeName = ox::core::CString<wchar_t>(pCommentBegin+2, (int)(P - pCommentBegin-2));
	P += 3;
}


//! parses an opening xml element and reads attributes
void CXMLReader::parseOpeningXMLElement()
{
	CurrentNodeType = EXN_ELEMENT;
	IsEmptyElement = false;
	Attributes.clear();

	// find name
	const wchar_t* startName = P;

	while(*P != L'>' && !isWhiteSpace(*P))
		++P;

	const wchar_t* endName = P;

	// find Attributes
	while(*P != L'>')
	{
		if (isWhiteSpace(*P))
			++P;
		else
		{
			if (*P != L'/')
			{
				// we've got an attribute

				// read the attribute names
				const wchar_t* attributeNameBegin = P;

				while(!isWhiteSpace(*P) && *P != L'=')
					++P;

				const wchar_t* attributeNameEnd = P;
				++P;

				// read the attribute value
				while(*P != L'\"')
					++P;

				++P;
				const wchar_t* attributeValueBegin = P;

				while(*P != L'\"')
					++P;

				const wchar_t* attributeValueEnd = P;
				++P;

				SAttribute attr;
				attr.Name = ox::core::CString<wchar_t>(attributeNameBegin, attributeNameEnd - attributeNameBegin);
				ox::core::CString<wchar_t> s(attributeValueBegin, attributeValueEnd - attributeValueBegin);
				attr.Value = replaceSpecialCharacters(s);
				Attributes.push_back(attr);
			}
			else
			{
				// tag is closed directly
				++P;
				IsEmptyElement = true;
				break;
			}
		}
	}

	NodeName = ox::core::CString<wchar_t>(startName, (int)(endName - startName));

	++P;
}


//! parses an closing xml tag
void CXMLReader::parseClosingXMLElement()
{
	CurrentNodeType = EXN_ELEMENT_END;
	IsEmptyElement = false;
	Attributes.clear();

	++P;
	const wchar_t* pBeginClose = P;

	while(*P != L'>')
		++P;

	NodeName = ox::core::CString<wchar_t>(pBeginClose, (int)(P - pBeginClose));
	++P;
}



//! Returns the type of the current XML node.
EXML_NODE CXMLReader::getNodeType()
{
	return CurrentNodeType;
}


//! Returns attribute count of the current XML node.
int CXMLReader::getAttributeCount()
{
	return Attributes.size();
}


//! Returns name of an attribute.
const wchar_t* CXMLReader::getAttributeName(int idx)
{
	if (idx < 0 || idx >= (int)Attributes.size())
		return 0;

	return Attributes[idx].Name.c_str();
}


//! Returns the value of an attribute.
const wchar_t* CXMLReader::getAttributeValue(int idx)
{
	if (idx < 0 || idx >= (int)Attributes.size())
		return 0;

	return Attributes[idx].Value.c_str();
}


//! Returns the value of an attribute.
const wchar_t* CXMLReader::getAttributeValue(const wchar_t* name)
{
	SAttribute* attr = getAttributeByName(name);
	if (!attr)
		return 0;

	return attr->Value.c_str();
}


//! Returns the value of an attribute
const wchar_t* CXMLReader::getAttributeValueSafe(const wchar_t* name)
{
	SAttribute* attr = getAttributeByName(name);
	if (!attr)
		return L"";

	return attr->Value.c_str();
}



//! Returns the value of an attribute as integer.
int CXMLReader::getAttributeValueAsInt(const wchar_t* name)
{
	return (int)getAttributeValueAsFloat(name);
}


//! Returns the value of an attribute as float.
float CXMLReader::getAttributeValueAsFloat(const wchar_t* name)
{
	SAttribute* attr = getAttributeByName(name);
	if (!attr)
		return 0;

	ox::core::CString<char> c = ox::core::CStringFunctions::wideToAnsi(attr->Value.c_str());
	return core::fast_atof(c.c_str());
}


//! Returns the name of the current node.
const wchar_t* CXMLReader::getNodeName()
{
	return NodeName.c_str();
}


//! Returns data of the current node.
const wchar_t* CXMLReader::getNodeData()
{
	return NodeName.c_str();
}


//! Returns if an element is an empty element, like <foo />
bool CXMLReader::isEmptyElement()
{
	return IsEmptyElement;
}


CXMLReader::SAttribute* CXMLReader::getAttributeByName(const wchar_t* name)
{
	if (!name)
		return 0;

	ox::core::CString<wchar_t> n = name;

	for (int i=0; i<(int)Attributes.size(); ++i)
		if (Attributes[i].Name == n)
			return &Attributes[i];

	return 0;
}

const CXMLReader::XMLSpecialCharacters XMLRSChar[] =
{
	// all start with &
	{ L'&', L"amp;" },
	{ L'<', L"lt;" },
	{ L'>', L"gt;" },
	{ L'"', L"quot;" },
	{ L' ', 0}
};

ox::core::CString<wchar_t> CXMLReader::replaceSpecialCharacters(ox::core::CString<wchar_t>& origstr)
{
	int pos = origstr.findFirst(L'&');
	int oldPos = 0;

	if (pos == -1)
		return origstr;

	ox::core::CString<wchar_t> newstr;

	while(pos != -1 && pos < origstr.size()-2)
	{
		// check if it is one of the special characters

		int specialChar = -1;
		for (int i=0; XMLRSChar[i].Symbol != 0; ++i)
		{
			const wchar_t* p = &origstr.c_str()[pos]+1;
			if (!wcsncmp(p, XMLRSChar[i].Symbol, wcslen(XMLRSChar[i].Symbol)))
			{
				specialChar = i;
				break;
			}
		}

		if (specialChar != -1)
		{
			newstr.append(origstr.subString(oldPos, pos - oldPos));
			newstr.append(XMLRSChar[specialChar].Character);
			pos += wcslen(XMLRSChar[specialChar].Symbol) + 1;
		}
		else
		{
			newstr.append(origstr.subString(oldPos, pos - oldPos + 1));
			pos += 1;
		}

		// find next &
		oldPos = pos;
		pos = origstr.findNext(L'&', pos);
	}

	if (oldPos < origstr.size()-1)
		newstr.append(origstr.subString(oldPos, origstr.size()-oldPos));

	return newstr;
}

} // end namespace daisy
} // end namespace io
