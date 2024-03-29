
// classfactory.c

#include "cpypath.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include "contextMenu.h"
#include "classfactory.h"


// declared in cpypath.c
extern UINT             g_uiRefThisDll; // Reference count for this DLL
extern HINSTANCE        g_hInstance;    

// The virtual table for the ClassFactory
IClassFactoryVtbl iclassFactoryVtbl = {
    CClassFactory_QueryInterface,
    CClassFactory_AddRef,
    CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer
};

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CClassFactory_Create 
/*--------------------------------------------------------------------------*/
//           Type: IClassFactory * 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: ClassFactoryEx constructor
/*--------------------------------------------------------------------@@-@@-*/
IClassFactory * CClassFactory_Create ( void )
/*--------------------------------------------------------------------------*/
{
     // Create the ClassFactoryStruct that will contain interfaces and vars
    ClassFactoryStruct * pCF = malloc ( sizeof(*pCF) );

    if(!pCF)
        return NULL;

    pCF->fc.lpVtbl = &iclassFactoryVtbl;

    // init the vars
    pCF->m_hInstance  = g_hInstance;    // Instance handle for this DLL
    pCF->m_ulRef = 1;                    // increment the reference

    ++g_uiRefThisDll;

    // Return the IClassFactory virtual table
    return &pCF->fc;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CClassFactory_QueryInterface 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IClassFactory *this: 
//    Param.    2: REFIID riid        : 
//    Param.    3: LPVOID *ppv        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CClassFactory_QueryInterface 
    ( IClassFactory *this, REFIID riid, LPVOID *ppv )
/*--------------------------------------------------------------------------*/
{
    // The address of the struct is the same as the address
    // of the IClassFactory Virtual table. 
    ClassFactoryStruct * pCF = (ClassFactoryStruct*)this;

    if ( IsEqualIID ( riid, &IID_IUnknown ) || 
        IsEqualIID ( riid, &IID_IClassFactory ))
    {
        *ppv = this;
        ++pCF->m_ulRef;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CClassFactory_AddRef 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IClassFactory *this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CClassFactory_AddRef ( IClassFactory *this )
/*--------------------------------------------------------------------------*/
{
    ClassFactoryStruct * pCF = (ClassFactoryStruct*)this;
    return ++pCF->m_ulRef;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CClassFactory_Release 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IClassFactory *this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CClassFactory_Release ( IClassFactory *this )
/*--------------------------------------------------------------------------*/
{
    ClassFactoryStruct * pCF = (ClassFactoryStruct*)this;
    if ( --pCF->m_ulRef == 0 )
    {
        free(this);
        --g_uiRefThisDll;
        return 0;
    }
    return pCF->m_ulRef;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CClassFactory_CreateInstance 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IClassFactory *this: 
//    Param.    2: LPUNKNOWN pUnkOuter: 
//    Param.    3: REFIID riid        : 
//    Param.    4: LPVOID *ppv        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CClassFactory_CreateInstance 
    ( IClassFactory *this, LPUNKNOWN pUnkOuter, REFIID riid,  LPVOID *ppv )
/*--------------------------------------------------------------------------*/
{
    *ppv = NULL;
    
    if ( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    HRESULT hr = S_OK;
    // Creates the IContextMenu incorperating IShellExtInit interfaces
    IContextMenu * pIContextMenu = CContextMenuExt_Create();
    if (NULL == pIContextMenu)
    {
        return E_OUTOFMEMORY;
    }

    // This puts the IContextMenu interface into 'ppv'
    hr = pIContextMenu->lpVtbl->QueryInterface ( pIContextMenu, riid, ppv );
    pIContextMenu->lpVtbl->Release ( pIContextMenu );
    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CClassFactory_LockServer 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IClassFactory *this: 
//    Param.    2: BOOL fLock         : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CClassFactory_LockServer ( IClassFactory *this, BOOL fLock )
/*--------------------------------------------------------------------------*/
{
    return E_NOTIMPL;
}
