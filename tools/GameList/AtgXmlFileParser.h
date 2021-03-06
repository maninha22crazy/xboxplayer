//-------------------------------------------------------------------------------------
//  AtgXmlFileParser.h
//
//  A SAX-based parser to read the ATG scene file format.
//
//  Xbox Advanced Technology Group
//  Copyright (C) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#ifndef ATG_XmlFileParser_H
#define ATG_XmlFileParser_H

#include <vector>
#include "AtgXmlParser.h"
#include "Externs.h"

struct XMLElementAttribute
{
    WCHAR   strName[MAX_PATH];
    WCHAR   strValue[MAX_PATH];
    UINT    NameLen;
    UINT    ValueLen;
};
typedef std::vector <XMLElementAttribute> XMLElementAttributeList;



namespace ATG
{

enum XATGObjectLoaderType
{
    XATG_NONE = 0,
    XATG_MESH,
    XATG_SKINNEDMESHINFLUENCES,
    XATG_VERTEXBUFFER,
    XATG_INDEXBUFFER,
    XATG_VERTEXDECLELEMENT,
    XATG_INDEXBUFFERSUBSET,
    XATG_FRAME,
    XATG_MODEL,
    XATG_MATERIAL,
    XATG_AMBIENTLIGHT,
    XATG_DIRECTIONALLIGHT,
    XATG_POINTLIGHT,
    XATG_SPOTLIGHT,
    XATG_CAMERA,
    XATG_ANIMATION,
};

class XMLElementDesc
{
public:
            XMLElementDesc()
            {
                strElementName[0] = L'\0';
                strElementBody[0] = L'\0';
                bEndElement = FALSE;
            }
    WCHAR   strElementName[128];
    WCHAR   strElementBody[1024];
    XMLElementAttributeList Attributes;
    BOOL bEndElement;
};

class Frame;
class ParameterDesc;

class XATGParserContext
{
public:
    XATGParserContext() : CurrentObjectType( XATG_NONE ),
                          pCurrentObject( NULL ),
                          pUserData( NULL ),
                          pCurrentParentFrame( NULL ),
                          pCurrentParentObject( NULL )
    {
    }
    XATGObjectLoaderType CurrentObjectType;
    VOID* pCurrentObject;
    VOID* pUserData;
    DWORD dwUserDataIndex;
    Frame* pCurrentParentFrame;
    VOID* pCurrentParentObject;
    DWORD dwCurrentParameterIndex;
};

class Scene;


enum XATGLoaderFlags
{
    XATGLOADER_DONOTINITIALIZEMATERIALS = 1,
    XATGLOADER_EFFECTSELECTORPARAMETERS = 2,
    XATGLOADER_DONOTBINDTEXTURES        = 4,
};

class XmlFileParser : public ISAXCallback
{
public:
    HRESULT  PrepareForThreadedLoad( CRITICAL_SECTION* pCriticalSection );
    HRESULT  LoadXMLFile( const CHAR* strFileName,void* pNode,UINT	nType = 0);
    CHAR* GetParseErrorMessage();

    virtual HRESULT StartDocument()
    {
        return S_OK;
    }
    virtual HRESULT EndDocument();

    virtual HRESULT ElementBegin( const WCHAR* strName, UINT NameLen,
                                  const XMLAttribute* pAttributes, UINT NumAttributes );
    virtual HRESULT ElementContent( const WCHAR* strData, UINT DataLen, BOOL More );
    virtual HRESULT ElementEnd( const WCHAR* strName, UINT NameLen );

    virtual HRESULT CDATABegin()
    {
        return S_OK;
    }
    virtual HRESULT CDATAData( const WCHAR* strCDATA, UINT CDATALen, BOOL bMore )
    {
        return S_OK;
    }
    virtual HRESULT CDATAEnd()
    {
        return S_OK;
    }
    virtual VOID    SetParseProgress( DWORD dwProgress );

    virtual VOID    Error( HRESULT hError, const CHAR* strMessage );

protected:
    XATGParserContext m_Context;
    XMLElementDesc m_CurrentElementDesc;

    VOID            CopyAttributesArc( const XMLAttribute* pAttributes, UINT uAttributeCount );				// ========= 解析arc的配置信息
	VOID            CopyAttributesAppConfig( const XMLAttribute* pAttributes, UINT uAttributeCount );		// ========= 解析本程序的配置信息
    VOID            HandleElementData();
    VOID            HandleElementEnd();

    BOOL            FindAttribute( const WCHAR* strName, WCHAR* strDest, UINT uDestLength );
    const WCHAR* FindAttribute( const WCHAR* strName );

    VOID            ProcessRootData();
    VOID            ProcessMeshData();
    VOID            ProcessFrameData();
    VOID            ProcessModelData();
    VOID            ProcessMaterialData();
    VOID            ProcessLightData();
    VOID            ProcessCameraData();
    VOID            ProcessAnimationData();
};

} // namespace ATG

#endif
