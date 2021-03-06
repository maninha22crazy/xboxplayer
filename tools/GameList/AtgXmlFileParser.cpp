//-----------------------------------------------------------------------------
// AtgXmlFileParser.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//
// This XML parser is somewhat unique, in that it bundles up the results from each SAX
// callback and distributes a smaller set of callbacks to the actual loader methods
// implemented in ProcessMeshData, ProcessFrameData, etc.
//
// For example, consider this sample XML document:
//
// <A>foobar</A>
//
// A straight SAX callback class would receive 3 callbacks:
// ElementBegin( "A" )
// ElementContent( "foobar" )
// ElementEnd( "A" )
//
// The problem is that the content is disassociated from the tag that it belongs to,
// especially since the ATG SAX parser may deliver multiple ElementContent() callbacks
// if the element content is particularly large.
//
// The ElementBegin(), ElementContent(), and ElementEnd() handlers implemented in
// XmlFileParser cache and accumulate the results into m_CurrentElementDesc, and then
// only call two handlers after that - HandleElementData() and HandleElementEnd().
// The data callback includes the name of the begin tag as well as any content between
// the begin and end tags.
//
// So, the same XML document is presented to the loader code like this:
// HandleElementData() strElementName = "A" strElementBody = "foobar" bEndElement = FALSE
// HandleElementEnd() strElementName = "A" strElementBody = "" bEndElement = TRUE
//
// Not only does this reduce complexity, it fits the XATG file format better because
// most of the data is in a rough key-value format, like <Size>3.0</Size>.
//
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgXmlFileParser.h"
#include "Externs.h"


UINT			m_nType;			// 节点类型
ArcadeInfo*		pArcadeInfo;		
bool localFind = true;

// 多语言支持
LPCWSTR LocaleLanguage2[11] =
{
	L"es-es",	// the default locale
	L"es-es",	// English
	L"ja-jp",	// Japanese
	L"de-de",	// German
	L"fr-fr",	// French
	L"es-es",	// Spanish
	L"it-it",	// Italian
	L"ko-kr",	// Korean
	L"zh-tw",	// Traditional Chinese
	L"pt-br",	// Portuguese
	L"zh-tw"	// Simplified Chinese
};


namespace ATG
{

#define MATCH_ELEMENT_NAME(x) (_wcsicmp(m_CurrentElementDesc.strElementName,x)==0)
#define MIN_SAFE_VERSION        1.6f

	const BOOL g_bDebugXMLParser = FALSE;


	Scene*              g_pCurrentScene = NULL;
	Frame*              g_pRootFrame = NULL;
	DWORD               g_dwLoaderFlags = 0;
	CHAR                g_strMediaRootPath[MAX_PATH];
	DWORD*              g_pLoadProgress = NULL;
	BYTE*               g_pBinaryBlobData = NULL;

	CRITICAL_SECTION*   g_pD3DCriticalSection = NULL;

	CHAR                g_strParseError[256];



	HRESULT XmlFileParser::LoadXMLFile( const CHAR* strFilename,void* pNode,UINT nType)
	{
		XMLParser parser;
		XmlFileParser XATGParser;

		g_strParseError[0] = '\0';
		
		localFind = true;
		m_nType = nType;
		if(m_nType == 1)
		{
			pArcadeInfo = (ArcadeInfo*)pNode;
		}

		parser.RegisterSAXCallbackInterface( &XATGParser );


		HRESULT hr = parser.ParseXMLFile( strFilename );

		if( SUCCEEDED( hr ) )
		{

		}
		return hr;
	}

	CHAR* XmlFileParser::GetParseErrorMessage()
	{
		return g_strParseError;
	}

	VOID XmlFileParser::SetParseProgress( DWORD dwProgress )
	{
		if( g_pLoadProgress != NULL )
			*g_pLoadProgress = dwProgress;
	}

	VOID XmlFileParser::Error( HRESULT hError, const CHAR* strMessage )
	{
		//OutputDebugString( strMessage );
		//OutputDebugString( "\n" );
		strcpy_s( g_strParseError, strMessage );
	}

	HRESULT XmlFileParser::EndDocument()
	{
		if( strlen( g_strParseError ) > 0 )
			return E_FAIL;
		return S_OK;
	}


	inline BOOL ErrorHasOccurred()
	{
		return ( g_strParseError[0] != '\0' );
	}


	HRESULT XmlFileParser::ElementBegin( const WCHAR* strName, UINT NameLen, const XMLAttribute* pAttributes,UINT NumAttributes )
	{
		// Check if an error has been encountered in scene parsing.
		if( ErrorHasOccurred() )
			return E_FAIL;

		// Distribute an accumulated begin+content package if one exists.
		HandleElementData();

		// Start a new begin+content package.
		// Copy the begin tag name to the current element desc.
		wcsncpy_s( m_CurrentElementDesc.strElementName, strName, NameLen );

		if(m_nType == 1)
		{
			CopyAttributesArc( pAttributes, NumAttributes );
		}
		else
		{
			CopyAttributesAppConfig( pAttributes, NumAttributes );
		}


		// Clear out the accumulated element body.
		m_CurrentElementDesc.strElementBody[0] = L'\0';
		// Copy all attributes from the begin tag into the current element desc.



		return S_OK;
	}

	HRESULT XmlFileParser::ElementContent( const WCHAR* strData, UINT DataLen, BOOL More )
	{
		// Accumulate this element content into the current desc body content.
		wcsncat_s( m_CurrentElementDesc.strElementBody, strData, DataLen );


		if(m_nType == 1 && localFind && _wcsicmp(m_CurrentElementDesc.strElementName,L"Description") == 0)
		{
			memset(pArcadeInfo->strDescription,0,1024); 
			
			wcsncpy_s( pArcadeInfo->strDescription,m_CurrentElementDesc.strElementBody,wcslen(m_CurrentElementDesc.strElementBody) );
			localFind = false;
		}
		return S_OK;
	}

	HRESULT XmlFileParser::ElementEnd( const WCHAR* strName, UINT NameLen )
	{
		// Check if an error has been encountered in scene parsing.
		if( ErrorHasOccurred() )
			return E_FAIL;

		// Distribute an accumulated begin+content package if one exists.
		HandleElementData();

		// Copy the end tag name into the current element desc.
		wcsncpy_s( m_CurrentElementDesc.strElementName, strName, NameLen );

		// Clear out the element body.
		m_CurrentElementDesc.strElementBody[0] = L'\0';
		// Distribute the end tag.
		HandleElementEnd();
		// Clear out the element name.
		m_CurrentElementDesc.strElementName[0] = L'\0';

		return S_OK;
	}

	VOID XmlFileParser::CopyAttributesArc( const XMLAttribute* pAttributes, UINT uAttributeCount )
	{
		m_CurrentElementDesc.Attributes.clear();

		
		if(_wcsicmp(m_CurrentElementDesc.strElementName,L"TitleInfo") == 0)
		{
			for(UINT i = 0; i < uAttributeCount; i++ )
			{
				WCHAR strNameT[MAX_PATH];
				memset(strNameT,0,MAX_PATH); 
				wcsncpy_s( strNameT,pAttributes[i].strName,pAttributes[i].NameLen);

				if(_wcsicmp(strNameT,L"ImagePath") == 0)
				{
					memset(strNameT,0,MAX_PATH); 
					wcsncpy_s( strNameT,pAttributes[i].strValue,pAttributes[i].ValueLen);
					swprintf( pArcadeInfo->strImagePath, L"file://dice:/%s", strNameT);
				}
				else if(_wcsicmp(strNameT,L"locale") == 0)
				{
					memset(strNameT,0,MAX_PATH); 
					wcsncpy_s( strNameT,pAttributes[i].strValue,pAttributes[i].ValueLen);
					if(_wcsicmp(strNameT,LocaleLanguage2[m_ConfigNode.nLanguage]) == 0)
					{
						localFind = true;
					}
				}
				else if(localFind && _wcsicmp(strNameT,L"Name") == 0)
				{
					memset(pArcadeInfo->strName,0,MAX_PATH); 
					wcsncpy_s( pArcadeInfo->strName,pAttributes[i].strValue,pAttributes[i].ValueLen);
				}
			}
		}
	}

	VOID XmlFileParser::CopyAttributesAppConfig( const XMLAttribute* pAttributes, UINT uAttributeCount )
	{
		m_CurrentElementDesc.Attributes.clear();

		WCHAR strName[MAX_PATH];
		WCHAR strValue[MAX_PATH];
		memset(strName,0,MAX_PATH); 
		memset(strValue,0,MAX_PATH);
		for(UINT i = 0; i < uAttributeCount; i++ )
		{
			WCHAR strNameT[MAX_PATH];
			memset(strNameT,0,MAX_PATH); 

			wcsncpy_s( strNameT,pAttributes[i].strName,pAttributes[i].NameLen);

			if(_wcsicmp(strNameT,L"name") == 0)
			{
				wcsncpy_s( strName,pAttributes[i].strValue,pAttributes[i].ValueLen);
			}
			else if(_wcsicmp(strNameT,L"value") == 0)
			{
				wcsncpy_s( strValue,pAttributes[i].strValue,pAttributes[i].ValueLen);
			}
		}

		if(_wcsicmp(strName,L"language") == 0)
		{
			m_ConfigNode.nLanguage = _wtoi(strValue);
		}
		else if(_wcsicmp(strName,L"device") == 0)
		{
			wcsncpy_s( m_ConfigNode.strDevice,strValue,wcslen(strValue));
		}
		else if(_wcsicmp(strName,L"oemcode") == 0)
		{
			m_ConfigNode.nOemCode = _wtoi(strValue);
		}
		else if(_wcsicmp(strName,L"showwall") == 0)
		{
			m_ConfigNode.nShowWall = _wtoi(strValue);
		}
		else if(_wcsicmp(strName,L"shownewwall") == 0)
		{
			m_ConfigNode.nShowNewWall = _wtoi(strValue);
		}
		else if(_wcsicmp(strName,L"wallPath") == 0)
		{
			wcsncpy_s( m_ConfigNode.strWallPath,strValue,wcslen(strValue));

		}
		else if(_wcsicmp(strName,L"gametype") == 0)
		{
			m_ConfigNode.nGameType = _wtoi(strValue);
		}
	}



	BOOL XmlFileParser::FindAttribute( const WCHAR* strName, WCHAR* strDest, UINT uDestLength )
	{
		const WCHAR* strValue = FindAttribute( strName );
		if( strValue != NULL )
		{
			wcscpy_s( strDest, uDestLength, strValue );
			return TRUE;
		}
		strDest[0] = L'\0';
		return FALSE;
	}

	const WCHAR* XmlFileParser::FindAttribute( const WCHAR* strName )
	{
		for( UINT i = 0; i < m_CurrentElementDesc.Attributes.size(); i++ )
		{
			const XMLElementAttribute& Attribute = m_CurrentElementDesc.Attributes[i];
			if( _wcsicmp( Attribute.strName, strName ) == 0 )
			{
				return Attribute.strValue;
			}
		}
		return NULL;
	}


	VOID ScrubFloatString( WCHAR* strFloatString )
	{
		WCHAR* pChar = strFloatString;
		while( *pChar != L'\0' )
		{
			if( *pChar == L'{' || *pChar == L'}' || *pChar == L',' || *pChar == L'\t' )
				*pChar = L' ';
			pChar++;
		}
	}



	//--------------------------------------------------------------------------------------
	// Name: HandleElementData()
	// Desc: This method gets "first crack" at a begin tag + content combo.
	//       It identifies certain high-level tags and sets the loader state appropriately.
	//--------------------------------------------------------------------------------------
	VOID XmlFileParser::HandleElementData()
	{
		// If the tag name is blank, return.
		if( wcslen( m_CurrentElementDesc.strElementName ) == 0 )
			return;

		// We are processing a begin tag.
		m_CurrentElementDesc.bEndElement = FALSE;
	}


	//--------------------------------------------------------------------------------------
	// Name: HandleElementEnd()
	// Desc: This method gets "first crack" at an end tag.
	//       It labels the tag state as "end tag" and passes the tag onto the loader
	//       methods.
	//--------------------------------------------------------------------------------------
	VOID XmlFileParser::HandleElementEnd()
	{
		// We are processing an end tag.
		m_CurrentElementDesc.bEndElement = TRUE;

		// Distribute the end tag to the appropriate loader function.
		//DistributeElementToLoaders();
	}


	WCHAR* AdvanceToken( WCHAR* pCurrentToken )
	{
		pCurrentToken = wcschr( pCurrentToken, L' ' );
		if( pCurrentToken == NULL )
			return NULL;
		while( *pCurrentToken == L' ' )
			pCurrentToken++;
		if( *pCurrentToken == '\0' )
			return NULL;
		return pCurrentToken;
	}


} // namespace ATG
