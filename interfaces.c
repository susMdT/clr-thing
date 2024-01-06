#include "interfaces.h"
#include "clr.h"
#include <Unknwn.h>

// MyHostMalloc (IHostMalloc) Function Implementations
HRESULT Alloc( MyHostMalloc* This, IN SIZE_T cbSize, IN EMemoryCriticalLevel eCriticalLevel, OUT void** ppMem ) {
    *ppMem = HeapAlloc( This->hHeap, HEAP_NO_SERIALIZE | HEAP_GENERATE_EXCEPTIONS, cbSize );
    if ( !*ppMem ) {
        return E_OUTOFMEMORY;
    }
    if ( Lock ) {
        printf( "HeapAlloc\n" );
    }
    return S_OK;
}

HRESULT DebugAlloc( MyHostMalloc* This, IN SIZE_T cbSize, IN EMemoryCriticalLevel eCriticalLevel, IN char* pszFileName, IN int iLineNo, _Outptr_result_maybenull_  void** ppMem) {
    return This->lpVtbl->Alloc( This, cbSize, eCriticalLevel, ppMem );
}

HRESULT Free( MyHostMalloc* This, IN void* pMem ) {
    HeapFree( This->hHeap, NULL, pMem );
    if ( Lock ) {
        printf( "HeapFree\n" );
    }
    return S_OK;
}

MyHostMalloc* NewMyHostMalloc() {

    MyHostMalloc* m            = malloc( sizeof(MyHostMalloc) );
    ZeroMemory( m, sizeof( MyHostMalloc ) );

    MyHostControlVtbl* m_vtbl  = malloc( sizeof( MyHostMallocVtbl ) );
    ZeroMemory( m_vtbl, sizeof( MyHostMallocVtbl ) );

    m->lpVtbl               = m_vtbl;
    m->lpVtbl->Alloc        = &Alloc;
    m->lpVtbl->DebugAlloc   = &DebugAlloc;
    m->lpVtbl->Free         = &Free;
    return m;
}

// MyHostMemoryManager (IHostMemoryManager) Function Implementations
HRESULT CreateMalloc( MyHostMemoryManager* This, IN DWORD dwMallocType, OUT MyHostMalloc** ppMalloc ) {

    MyHostMalloc* m = NewMyHostMalloc();
    m->hHeap        = HeapCreate( HEAP_NO_SERIALIZE, 0, 0 );
    *ppMalloc       = m;
    if ( Lock ) {
        printf( "HeapCreate\n" );
    }
    return S_OK;
}

HRESULT M_VirtualAlloc( MyHostMemoryManager* This, IN void* pAddress, IN SIZE_T dwSize, IN DWORD flAllocationType, IN DWORD flProtect, IN EMemoryCriticalLevel eCriticalLevel, OUT void** ppMem ) {
    *ppMem = VirtualAlloc( pAddress, dwSize, flAllocationType, flProtect );
    if ( Lock ) {
        printf( "VirtualAlloc: 0x%llx at 0x%llx, perm 0x%llx\n", dwSize, pAddress, flProtect );
    }
    return S_OK;
}

HRESULT M_VirtualFree( MyHostMemoryManager* This, IN LPVOID lpAddress, IN SIZE_T dwSize, IN DWORD dwFreeType ) {
    VirtualFree( lpAddress, dwSize, dwFreeType );
    if ( Lock ) {
        printf( "VirtualFree\n" );
    }
    return S_OK;
}

HRESULT M_VirtualQuery( MyHostMemoryManager* This, IN void* lpAddress, OUT void* lpBuffer, IN SIZE_T dwLength, OUT SIZE_T* pResult ) {
    *pResult = VirtualQuery( lpAddress, lpBuffer, dwLength );
    if ( Lock ) {
        printf( "VirtualQuery\n" );
    }
    return S_OK;
}

HRESULT M_VirtualProtect( MyHostMemoryManager* This, IN void* lpAddress, IN SIZE_T dwSize, IN DWORD flNewProtect, OUT DWORD* pflOldProtect ) {
    VirtualProtect( lpAddress, dwSize, flNewProtect, pflOldProtect );
    if ( Lock ) {
        printf( "VirtualProtect\n" );
    }
    return S_OK;
}

HRESULT GetMemoryLoad( MyHostMemoryManager* This, OUT DWORD* pMemoryLoad, OUT SIZE_T* pAvailableBytes ) {
    // copied, need to actually obtain these values somehow
    *pMemoryLoad     = 30;
    *pAvailableBytes = 100 * 1024 * 1024;
    return S_OK;
}rust clr heap encryption (https://github.com/lap1nou/CLR_Heap_

HRESULT RegisterMemoryNotificationCallback( MyHostMemoryManager* This, IN ICLRMemoryNotificationCallback* pCallback ) {
    // TODO
    return S_OK;
}

HRESULT NeedsVirtualAddressSpace( MyHostMemoryManager* This, IN LPVOID startAddress, IN SIZE_T size ) {
    // TODO
    return S_OK;
}

HRESULT AcquiredVirtualAddressSpace( MyHostMemoryManager* This, IN LPVOID startAddress, IN SIZE_T size ) {
    // TODO
    return S_OK;
}

HRESULT ReleasedVirtualAddressSpace( MyHostMemoryManager* This, IN LPVOID startAddress ) {
    return S_OK;
}

MyHostMemoryManager* NewMyHostMemoryManager() {

    MyHostMemoryManager* m                          = malloc( sizeof( MyHostMemoryManager ) );
    ZeroMemory( m, sizeof( MyHostMemoryManager ) );

    MyHostControlVtbl* m_vtbl                       = malloc( sizeof( MyHostMemoryManagerVtbl ) );
    ZeroMemory( m_vtbl, sizeof( MyHostMemoryManagerVtbl ) );

    m->lpVtbl                                       = m_vtbl;
    m->lpVtbl->AcquiredVirtualAddressSpace          = &AcquiredVirtualAddressSpace;
    m->lpVtbl->CreateMalloc                         = &CreateMalloc;
    m->lpVtbl->GetMemoryLoad                        = &GetMemoryLoad;
    m->lpVtbl->NeedsVirtualAddressSpace             = &NeedsVirtualAddressSpace;
    m->lpVtbl->RegisterMemoryNotificationCallback   = &RegisterMemoryNotificationCallback;
    m->lpVtbl->ReleasedVirtualAddressSpace          = &ReleasedVirtualAddressSpace;
    m->lpVtbl->VirtualAlloc                         = &M_VirtualAlloc;
    m->lpVtbl->VirtualFree                          = &M_VirtualFree;
    m->lpVtbl->VirtualProtect                       = &M_VirtualProtect;
    m->lpVtbl->VirtualQuery                         = &M_VirtualQuery;

    return m;
}

// MyHostControl (IHostControl) Function Implementations

// IDK WHAT THESE 3 DO BUT THEY ARE COOKING ! ! !
HRESULT QueryInterface( MyHostControl* This, IN REFIID riid, _COM_Outptr_  void** ppvObject ) {
    return S_OK;
}

ULONG AddRef( MyHostControl* This ) {
    return 1;
}

ULONG Release( MyHostControl* This ) {
    return 0;
}

HRESULT GetHostManager( MyHostControl* This, IN REFIID riid, OUT void** ppObject ) {
    MyHostMemoryManager* m = NULL;

    if ( !memcmp( riid, &IID_IHostMemoryManager, sizeof(IID) ) ) {
         m          = NewMyHostMemoryManager();
        *ppObject   = m;
        return S_OK;
    }
    else {
        *ppObject = NULL;
        return E_NOINTERFACE;
    }
}
HRESULT SetAppDomainManager( MyHostControl* This, IN DWORD dwAppDomainID, IN IUnknown* pUnkAppDomainManager ) {
    return S_OK;
}

MyHostControl* NewMyHostControl() {
    MyHostControl*     m           = malloc( sizeof( MyHostControl ) );
    ZeroMemory( m, sizeof( MyHostControl ) );

    MyHostControlVtbl* m_vtbl      = malloc( sizeof( MyHostControlVtbl ) );
    ZeroMemory( m_vtbl, sizeof(MyHostControlVtbl) );

    m->lpVtbl                      = m_vtbl;
    m->lpVtbl->AddRef              = &AddRef;
    m->lpVtbl->Release             = &Release;
    m->lpVtbl->QueryInterface      = &QueryInterface;
    m->lpVtbl->GetHostManager      = &GetHostManager;
    m->lpVtbl->SetAppDomainManager = &SetAppDomainManager;
    return m;
}
