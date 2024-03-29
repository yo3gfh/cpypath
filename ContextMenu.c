
// contextmenu.c

/*--------------------------------------------------------------
    The IShellExtInit interface is incorporated into the 
    IContextMenu interface
--------------------------------------------------------------*/

#include "cpypath.h"
#include <windows.h>
#include <shlobj.h>
#include <strsafe.h>
#include <stdarg.h>
#include <winnetwk.h>
#include <stdio.h>
#include "ContextMenu.h"

#define SEVERITY_SUCCESS    0
// menu ID (copy path to clipboard)
#define IDM_CPTC            0x0001
#define IDB_CPTC            8001
// sizeof TCHAR would do, but let's be generous
#define ALLOC_MULT          (sizeof(TCHAR)+2)            

#pragma warn(disable: 2231 2030 2260)                           

const TCHAR * PathExtractDir ( const TCHAR * src );
BOOL PathsExpandAsNeeded ( TCHAR * dest, size_t cchDest, TCHAR * src );

// Keep a count of DLL references
extern UINT         g_uiRefThisDll;
extern HINSTANCE    g_hInstance;
UINT                g_nFiles;
HBITMAP             g_hMenuBmp;


// The virtual table of functions for IContextMenu interface
IContextMenuVtbl icontextMenuVtbl = {
    CContextMenuExt_QueryInterface,
    CContextMenuExt_AddRef,
    CContextMenuExt_Release,
    CContextMenuExt_QueryContextMenu,
    CContextMenuExt_InvokeCommand,
    CContextMenuExt_GetCommandString
};

// The virtual table of functions for IShellExtInit interface
IShellExtInitVtbl ishellInitExtVtbl = {
    CShellInitExt_QueryInterface,
    CShellInitExt_AddRef,
    CShellInitExt_Release,
    CShellInitExt_Initialize
};

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_Create 
/*--------------------------------------------------------------------------*/
//           Type: IContextMenu * 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: IContextMenu constructor
/*--------------------------------------------------------------------@@-@@-*/
IContextMenu * CContextMenuExt_Create ( void )
/*--------------------------------------------------------------------------*/
{
    // Create the ContextMenuExtStruct that will contain interfaces and vars
    ContextMenuExtStruct * pCM = malloc ( sizeof(ContextMenuExtStruct) );

    if ( !pCM )
        return NULL;

    // Point to the IContextMenu and IShellExtInit Vtbl's
    pCM->cm.lpVtbl = &icontextMenuVtbl;
    pCM->si.lpVtbl = &ishellInitExtVtbl;

    // increment the class reference count
    pCM->m_ulRef = 1;
    pCM->m_pszSource = NULL;

    g_uiRefThisDll++;

    // Return the IContextMenu virtual table
    return &pCM->cm;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_QueryInterface 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this: 
//    Param.    2: REFIID riid        : 
//    Param.    3: LPVOID *ppv        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: IContextMenu interface routines
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_QueryInterface 
    ( IContextMenu * this, REFIID riid, LPVOID *ppv )
/*--------------------------------------------------------------------------*/
{
    // The address of the struct is the same as the address
    // of the IContextMenu Virtual table. 
    ContextMenuExtStruct * pCM = (ContextMenuExtStruct*)this;
    
    if ( IsEqualIID ( riid, &IID_IUnknown ) 
        || IsEqualIID ( riid, &IID_IContextMenu ) )
    {
        *ppv = this;
        pCM->m_ulRef++;
        return S_OK;
    }
    else if ( IsEqualIID ( riid, &IID_IShellExtInit ) )
    {
        // Give the IShellInitExt interface
        *ppv = &pCM->si;
        pCM->m_ulRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_AddRef 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IContextMenu * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: 
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CContextMenuExt_AddRef ( IContextMenu * this )
/*--------------------------------------------------------------------------*/
{
    ContextMenuExtStruct * pCM = (ContextMenuExtStruct*)this;
    return ++pCM->m_ulRef;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_Release 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IContextMenu * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CContextMenuExt_Release ( IContextMenu * this )
/*--------------------------------------------------------------------------*/
{
    ContextMenuExtStruct * pCM = (ContextMenuExtStruct*)this;

    if ( --pCM->m_ulRef == 0 )
    {
        free ( pCM->m_pszSource );
        free ( this );
        DeleteObject ( g_hMenuBmp ); // free menu bitmap
        g_uiRefThisDll--;
        return 0;
    }

    return pCM->m_ulRef;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_GetCommandString 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this: 
//    Param.    2: UINT_PTR idCmd     : 
//    Param.    3: UINT uFlags        : 
//    Param.    4: UINT *pwReserved   : 
//    Param.    5: LPSTR pszName      : 
//    Param.    6: UINT cchMax        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_GetCommandString 
    ( IContextMenu * this, UINT_PTR idCmd, UINT uFlags, 
    UINT *pwReserved, LPSTR pszName, UINT cchMax )
/*--------------------------------------------------------------------------*/
{
    HRESULT hr = S_OK;

    switch ( uFlags )
    {
        case GCS_HELPTEXTA:
        switch ( idCmd )
        {
            case IDM_CPTC:
            StringCchCopyA ( (LPSTR)pszName, cchMax, 
                        (g_nFiles == 1) ? "Copy Path to Clipboard" :
                        "Copy All Paths to Clipboard" );
            hr = S_OK;
            break;
            
            default:
            hr = E_NOTIMPL;
            break;
        }
        break;

        case GCS_HELPTEXTW:
        switch ( idCmd )
        {
            case IDM_CPTC:
            StringCchCopyW ( (LPWSTR)pszName, cchMax,
                        (g_nFiles == 1) ? L"Copy Path to Clipboard" :
                        L"Copy All Paths to Clipboard");
            hr = S_OK;
            break;

            default:
            hr = E_NOTIMPL;
            break;
        }
        break;
    }
    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_QueryContextMenu 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this: 
//    Param.    2: HMENU hMenu        : 
//    Param.    3: UINT uiIndexMenu   : 
//    Param.    4: UINT idCmdFirst    : 
//    Param.    5: UINT idCmdLast     : 
//    Param.    6: UINT uFlags        : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_QueryContextMenu 
    ( IContextMenu * this, HMENU hMenu, UINT uiIndexMenu, 
    UINT idCmdFirst, UINT idCmdLast, UINT uFlags )
/*--------------------------------------------------------------------------*/
{
    HINSTANCE        hinst;
    HBITMAP          hbmp;
    UINT             ix;
    ULONG            sev;
    MENUITEMINFO     mi;

    hinst            = g_hInstance;
    hbmp             = LoadBitmap ( hinst, MAKEINTRESOURCE(IDB_CPTC) );
    g_hMenuBmp       = hbmp;

    ix               = uiIndexMenu;
    sev              = SEVERITY_SUCCESS;

    RtlZeroMemory ( &mi, sizeof (MENUITEMINFO) );
    mi.cbSize        = sizeof (MENUITEMINFO);
    mi.fMask         = MIIM_STRING | MIIM_STATE | MIIM_BITMAP | MIIM_FTYPE | 
                        MIIM_ID | MIIM_CHECKMARKS;
    mi.dwTypeData    = ( g_nFiles == 1 ) ? TEXT("Copy &Path to Clipboard") :
                        TEXT("Copy All &Paths to Clipboard");
    mi.wID           = idCmdFirst + IDM_CPTC;
    mi.fType         = MFT_STRING;
    mi.fState        = MFS_ENABLED;
    mi.hbmpItem      = hbmp;
    mi.hbmpChecked   = hbmp;
    mi.hbmpUnchecked = hbmp;

    __try
    {
        InsertMenu(hMenu, ix++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        InsertMenuItem ( hMenu, ix++, TRUE, &mi );
        InsertMenu(hMenu, ix++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        sev = SEVERITY_ERROR;
    }
    
    return MAKE_HRESULT(sev, FACILITY_NULL, (USHORT)(IDM_CPTC + 1));
}

PROCESS_INFORMATION pi;
STARTUPINFO         si;

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_InvokeCommand 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IContextMenu * this         : 
//    Param.    2: LPCMINVOKECOMMANDINFO lpici : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Executed when our menu is clicked. We just copy to
//                 clipboard the filelist received in * this 
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CContextMenuExt_InvokeCommand 
    ( IContextMenu * this, LPCMINVOKECOMMANDINFO lpici )
/*--------------------------------------------------------------------------*/
{
    // here we dump a bunch of filenames user selected.. 
    // how many he selected? lol
    // pCM->m_pszSource_bsize should tell us

    HGLOBAL         hMem;
    TCHAR           * buf;
    HRESULT         hr = S_OK;
    DWORD           buf_ccb, alloc_size;
    UINT            cf_flags;

    #ifdef UNICODE
        cf_flags    = CF_UNICODETEXT;
    #else
        cf_flags    = CF_TEXT;
    #endif

    ContextMenuExtStruct * pCM = ( ContextMenuExtStruct* )this;

    if ( pCM->m_pszSource_bsize == 0 )
    {
        hr = E_FAIL;
        return hr;
    }

    alloc_size = pCM->m_pszSource_bsize + MAX_PATH;
    buf_ccb = alloc_size / sizeof(TCHAR);

    // alloc in 4k chunks
    hMem = GlobalAlloc ( GHND, (alloc_size+4096+1) & (~4095) );

    if ( hMem == NULL )
    {
        hr = E_FAIL;
        return hr;
    }
    
    buf = (TCHAR *)GlobalLock ( hMem );

    if ( buf == NULL )
    {
        GlobalUnlock ( hMem );
        GlobalFree ( hMem );
        hr = E_FAIL;

        return hr;
    }

    __try
    {
        switch ( LOWORD(lpici->lpVerb) )
        {
            case IDM_CPTC:
                // Check each path from the list and expand
                // to fully qualified UNC path, if necessary 
                PathsExpandAsNeeded ( buf, buf_ccb,
                    pCM->m_pszSource );

                GlobalUnlock ( hMem );
                
                if ( !OpenClipboard ( NULL ) )
                {
                    hr = E_FAIL;
                    GlobalFree ( hMem );
                    return hr;
                }

                EmptyClipboard();

                if ( !SetClipboardData ( cf_flags, hMem ) )
                {
                    hr = E_FAIL;
                    GlobalFree ( hMem );
                    CloseClipboard();
                    return hr;
                }

                CloseClipboard();
                break;

            default:
                hr = E_FAIL;
                break;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = E_FAIL;
    }

    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CContextMenuExt_ErrMessage 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: DWORD dwErrcode : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
void CContextMenuExt_ErrMessage ( DWORD dwErrcode )
/*--------------------------------------------------------------------------*/
{
    LPTSTR pMsgBuf;

    FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErrcode, 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPTSTR)&pMsgBuf, 0, NULL);

    MessageBox ( GetForegroundWindow(), pMsgBuf, 
        TEXT("cpypathSH"), MB_ICONERROR );

    LocalFree(pMsgBuf);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_QueryInterface 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IShellExtInit * this: 
//    Param.    2: REFIID riid         : 
//    Param.    3: LPVOID *ppv         : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: IShellExtInit interface routines
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CShellInitExt_QueryInterface 
    ( IShellExtInit * this, REFIID riid, LPVOID *ppv )
/*--------------------------------------------------------------------------*/
{
    /*-----------------------------------------------------------------
    IContextMenu Vtbl is the same address as ContextMenuExtStruct.
     IShellExtInit is sizeof(IContextMenu) further on.
    -----------------------------------------------------------------*/
    IContextMenu * pIContextMenu = (IContextMenu *)(this-1);
    return pIContextMenu->lpVtbl->QueryInterface ( pIContextMenu, riid, ppv );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_AddRef 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IShellExtInit * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CShellInitExt_AddRef ( IShellExtInit * this )
/*--------------------------------------------------------------------------*/
{
    // Redirect the IShellExtInit's AddRef to the IContextMenu interface
    IContextMenu * pIContextMenu = (IContextMenu *)(this-1);
    return pIContextMenu->lpVtbl->AddRef ( pIContextMenu );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_Release 
/*--------------------------------------------------------------------------*/
//           Type: ULONG STDMETHODCALLTYPE 
//    Param.    1: IShellExtInit * this : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
ULONG STDMETHODCALLTYPE CShellInitExt_Release ( IShellExtInit * this )
/*--------------------------------------------------------------------------*/
{
    // Redirect the IShellExtInit's Release to the IContextMenu interface
    IContextMenu * pIContextMenu = (IContextMenu *)(this-1);
    return pIContextMenu->lpVtbl->Release ( pIContextMenu );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CShellInitExt_Initialize 
/*--------------------------------------------------------------------------*/
//           Type: STDMETHODIMP 
//    Param.    1: IShellExtInit * this    : 
//    Param.    2: LPCITEMIDLIST pidlFolder: 
//    Param.    3: LPDATAOBJECT lpdobj     : 
//    Param.    4: HKEY hKeyProgID         : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.09.2020
//    DESCRIPTION: Get the selected file list from Explorer, transform it into
//                 cr/lf separated file list
/*--------------------------------------------------------------------@@-@@-*/
STDMETHODIMP CShellInitExt_Initialize 
    ( IShellExtInit * this, LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT lpdobj, HKEY hKeyProgID )
/*--------------------------------------------------------------------------*/
{
    FORMATETC      fe;
    STGMEDIUM      stgmed;
    #ifndef UNICODE
    char           * temp;
    ULONG          bSize = 0;
    #endif
    ULONG          iSize = 0;
    ULONG          i, j;

    fe.cfFormat    = CF_HDROP;
    fe.ptd         = NULL;
    fe.dwAspect    = DVASPECT_CONTENT;
    fe.lindex      = -1;
    fe.tymed       = TYMED_HGLOBAL;

    ContextMenuExtStruct * pCM = (ContextMenuExtStruct *)(this-1);
    
    // Get the storage medium from the data object.
    HRESULT hr = lpdobj->lpVtbl->GetData ( lpdobj, &fe, &stgmed );

    if ( SUCCEEDED(hr) )
    {
        if ( stgmed.hGlobal )
        {
            __try
            {
                g_nFiles = DragQueryFile ( stgmed.hGlobal, 0xFFFFFFFF, NULL, 0 );
                
                LPDROPFILES pDropFiles = 
                    (LPDROPFILES)GlobalLock ( stgmed.hGlobal );

                LPSTR pszFiles  = NULL, pszTemp = NULL;
                LPWSTR pswFiles = NULL, pswTemp = NULL;

                if (pDropFiles->fWide)
                {
                    pswFiles    =    (LPWSTR) 
                        ((BYTE*) pDropFiles + pDropFiles->pFiles);
                    pswTemp     =    (LPWSTR) 
                        ((BYTE*) pDropFiles + pDropFiles->pFiles);
                }
                else
                {
                    pszFiles    =    (LPSTR) pDropFiles + pDropFiles->pFiles;
                    pszTemp     =    (LPSTR) pDropFiles + pDropFiles->pFiles;
                }

                while ( pszFiles && *pszFiles || pswFiles && *pswFiles )
                {
                    if ( pDropFiles->fWide )
                    {
                        //Get size of first file/folders path
                        #ifndef UNICODE
                        iSize += WideCharToMultiByte ( CP_ACP, 0, pswFiles, 
                            -1, NULL, 0, NULL, NULL );
                        #else
                        iSize += lstrlenW ( pswFiles ) + 1;
                        #endif
                        pswFiles += (lstrlenW ( pswFiles ) + 1 );
                    }
                    else
                    {
                        //Get size of first file/folders path
                        iSize += lstrlenA ( pszFiles ) + 1;
                        pszFiles += (lstrlenA ( pszFiles ) + 1 );
                    }
                }

                if ( iSize )
                {
                    pCM->m_pszSource = malloc ( iSize<<ALLOC_MULT );
                    // if we fail, at least signal we have crap
                    pCM->m_pszSource_bsize = 0; 

                    if ( !pCM->m_pszSource )
                    {
                        hr = E_OUTOFMEMORY;
                        GlobalUnlock ( stgmed.hGlobal );
                        ReleaseStgMedium ( &stgmed );
                        return hr;
                    }

                    RtlZeroMemory ( pCM->m_pszSource, iSize<<ALLOC_MULT );

                    if ( pswTemp )
                    {
                        #ifndef UNICODE
                        temp = malloc ( iSize<<ALLOC_MULT );

                        if ( !temp )
                        {
                            hr = E_OUTOFMEMORY;
                            GlobalUnlock ( stgmed.hGlobal );
                            ReleaseStgMedium ( &stgmed );
                            return hr;
                        }

                        RtlZeroMemory ( temp, iSize<<ALLOC_MULT );
                        bSize = WideCharToMultiByte ( CP_ACP, 0, pswTemp, 
                            iSize, temp, iSize, NULL, NULL );

                        for ( i = 0, j = 0; i < bSize-1; i++, j++ )
                        {
                            if ( temp[i] == '\0' )
                            {
                                pCM->m_pszSource[j++] = TEXT('\x0d');
                                pCM->m_pszSource[j]   = TEXT('\x0a');
                                continue;
                            }
                            pCM->m_pszSource[j] = (TCHAR)temp[i];
                        }

                        pCM->m_pszSource[j++]  = TEXT('\x0d');
                        pCM->m_pszSource[j++]  = TEXT('\x0a');
                        pCM->m_pszSource[j]    = TEXT('\0');
                        pCM->m_pszSource_bsize = j<<ALLOC_MULT;
                        free ( temp );

                        #else // UNICODE build

                        for ( i = 0, j = 0; i < iSize-1; i++, j++ )
                        {
                            if ( pswTemp[i] == TEXT('\0') )
                            {
                                pCM->m_pszSource[j++] = TEXT('\x0d');
                                pCM->m_pszSource[j]   = TEXT('\x0a');
                                continue;
                            }
                            pCM->m_pszSource[j] = pswTemp[i];
                        }

                        pCM->m_pszSource[j++]  = TEXT('\x0d');
                        pCM->m_pszSource[j++]  = TEXT('\x0a');
                        pCM->m_pszSource[j]    = TEXT('\0');
                        pCM->m_pszSource_bsize = j<<ALLOC_MULT;
                        #endif
                    }
                    else
                    {
                        for ( i = 0, j = 0; i < iSize-1; i++, j++ )
                        {
                            if ( pszTemp[i] == '\0' )
                            {
                                pCM->m_pszSource[j++] = TEXT('\x0d');
                                pCM->m_pszSource[j]   = TEXT('\x0a');
                                continue;
                            }
                            pCM->m_pszSource[j] = pszTemp[i];
                        }

                        pCM->m_pszSource[j++]  = TEXT('\x0d');
                        pCM->m_pszSource[j++]  = TEXT('\x0a');
                        pCM->m_pszSource[j]    = TEXT('\0');
                        pCM->m_pszSource_bsize = j<<ALLOC_MULT;
                    }
                }
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                hr = E_UNEXPECTED;
            }
        }
        else
            hr = E_UNEXPECTED;

        GlobalUnlock ( stgmed.hGlobal );
        ReleaseStgMedium ( &stgmed );
    }
    return hr;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: PathExtractDir 
/*--------------------------------------------------------------------------*/
//           Type: const TCHAR * 
//    Param.    1: const TCHAR * src : some path
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 09.06.2021
//    DESCRIPTION: Extracts the drive letter part from a path
/*--------------------------------------------------------------------@@-@@-*/
const TCHAR * PathExtractDir ( const TCHAR * src )
/*--------------------------------------------------------------------------*/
{
    static  TCHAR buf[MAX_PATH+1];
    UINT    len, idx;

    buf[0] = TEXT('\0');

    if ( src == NULL )
        return buf;

    len = lstrlen ( src );

    if ( len > MAX_PATH )
        return buf;

    idx = 0;

    while (( src[idx] != TEXT(':') ) && ( idx < len ))
        idx++;

    if ( idx == len )
        return buf;

    StringCchCopyN ( buf, ARRAYSIZE(buf), src, idx+1 );

    return buf;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: PathsExpandAsNeeded 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: TCHAR * dest  : where to put expanded paths
//    Param.    2: size_t cchDest: dest size, in chars
//    Param.    3: TCHAR * src   : null terminated src
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 09.06.2021
//    DESCRIPTION: Parses src for cr/lf separated paths and expands
//                 mapped network drives to UNC paths, as needed. Returns
//                 TRUE on success, FALSE on failure.
/*--------------------------------------------------------------------@@-@@-*/
BOOL PathsExpandAsNeeded ( TCHAR * dest, size_t cchDest, TCHAR * src )
/*--------------------------------------------------------------------------*/
{
    TCHAR   orig_path[4096];
    TCHAR   exp_path[4096];
    UINT    dst_idx, src_idx, src_len, pstart, exp_len;

    if ( dest == NULL || src == NULL || cchDest == 0 )
        return FALSE;

    src_len         = lstrlen ( src );
    src_idx         = 0;
    dst_idx         = 0;
    pstart          = 0;
    exp_len         = ARRAYSIZE(exp_path);

    dest[0]         = TEXT('\0');

    if ( src_len > cchDest )
        src_len = cchDest;

    while ( src_idx < src_len )
    {
        if ( (src[src_idx] == TEXT('\n')) ||
                (src[src_idx] == TEXT('\r')) ||
                    (dst_idx >= 4095) )
        {
            StringCchCopyN ( orig_path, ARRAYSIZE(orig_path), 
                src+pstart, dst_idx+1 );

            // do we have a network drive mapped as the drive letter?
            if ( WNetGetConnection ( PathExtractDir ( orig_path ), 
                    exp_path, &exp_len ) == NO_ERROR )
            {
                // yes, replace it with the fully q. UNC path
                StringCchCat ( dest, cchDest, exp_path );
                StringCchCat ( dest, cchDest, orig_path+2 );
            }
            else
            {
                // no, just copy verbatim
                StringCchCat ( dest, cchDest, orig_path );
            }

            StringCchCat ( dest, cchDest, TEXT("\n") );

            dst_idx = 0;

            while ( (src[src_idx] == TEXT('\n')) ||
                (src[src_idx] == TEXT('\r')) )
                    src_idx++;

            pstart = src_idx;
        }
        else
        {
            src_idx++;
            dst_idx++;
        }
    }

    return TRUE;
}
