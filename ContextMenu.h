
// contextmenu.h

#ifndef _CONTEXTMENU_H_
#define _CONTEXTMENU_H_

// IContextMenu methods
STDMETHODIMP CContextMenuExt_QueryInterface ( IContextMenu * this,
    REFIID riid, LPVOID *ppvOut );

ULONG STDMETHODCALLTYPE CContextMenuExt_AddRef ( IContextMenu * this );

ULONG STDMETHODCALLTYPE CContextMenuExt_Release ( IContextMenu * this );
STDMETHODIMP CContextMenuExt_QueryContextMenu ( IContextMenu * this, HMENU, 
    UINT, UINT, UINT, UINT );
STDMETHODIMP CContextMenuExt_InvokeCommand ( IContextMenu * this, 
    LPCMINVOKECOMMANDINFO );
STDMETHODIMP CContextMenuExt_GetCommandString ( IContextMenu * this, 
    UINT_PTR, UINT, UINT *, LPSTR, UINT );

// IContextMenu constructor
IContextMenu        * CContextMenuExt_Create                    ( void );

// This struct acts somewhat like a pseudo class in that you have
// variables accociated with an instance of this interface.
typedef struct _ContextMenuExtStruct
{
    // Two interfaces
    IContextMenu            cm;
    IShellExtInit           si;

    // second part of the struct for the variables
    LPTSTR                  m_pszSource;
    // let's put this so CContextMenuExt_InvokeCommand
    // will know how many bytes to alloc for the cmdline
    // buffer
    size_t                  m_pszSource_bsize;
    ULONG                   m_ulRef;
} ContextMenuExtStruct;

// IShellExtInit methods
STDMETHODIMP CShellInitExt_QueryInterface ( IShellExtInit * this, 
    REFIID riid, LPVOID* ppvObject );

ULONG STDMETHODCALLTYPE CShellInitExt_AddRef ( IShellExtInit * this );

ULONG STDMETHODCALLTYPE CShellInitExt_Release ( IShellExtInit * this );
STDMETHODIMP CShellInitExt_Initialize ( IShellExtInit * this, 
    LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hKeyProgID );

#endif // _CONTEXTMENU_H_


