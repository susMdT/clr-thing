#pragma once
#include <windows.h>
#include <metahost.h>
#include <stdio.h>

typedef interface MyHostMemoryManager MyHostMemoryManager;

typedef struct MyHostMemoryManagerVtbl
{
    BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)(
            MyHostMemoryManager* This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */
            _COM_Outptr_  void** ppvObject);

    ULONG(STDMETHODCALLTYPE* AddRef)(
        MyHostMemoryManager* This);

    ULONG(STDMETHODCALLTYPE* Release)(
        MyHostMemoryManager* This);

    HRESULT(STDMETHODCALLTYPE* CreateMalloc)(
        MyHostMemoryManager* This,
        /* [in] */ DWORD dwMallocType,
        /* [out] */ IHostMalloc** ppMalloc);

    HRESULT(STDMETHODCALLTYPE* VirtualAlloc)(
        MyHostMemoryManager* This,
        /* [in] */ void* pAddress,
        /* [in] */ SIZE_T dwSize,
        /* [in] */ DWORD flAllocationType,
        /* [in] */ DWORD flProtect,
        /* [in] */ EMemoryCriticalLevel eCriticalLevel,
        /* [out] */ void** ppMem);

    HRESULT(STDMETHODCALLTYPE* VirtualFree)(
        MyHostMemoryManager* This,
        /* [in] */ LPVOID lpAddress,
        /* [in] */ SIZE_T dwSize,
        /* [in] */ DWORD dwFreeType);

    HRESULT(STDMETHODCALLTYPE* VirtualQuery)(
        MyHostMemoryManager* This,
        /* [in] */ void* lpAddress,
        /* [out] */ void* lpBuffer,
        /* [in] */ SIZE_T dwLength,
        /* [out] */ SIZE_T* pResult);

    HRESULT(STDMETHODCALLTYPE* VirtualProtect)(
        MyHostMemoryManager* This,
        /* [in] */ void* lpAddress,
        /* [in] */ SIZE_T dwSize,
        /* [in] */ DWORD flNewProtect,
        /* [out] */ DWORD* pflOldProtect);

    HRESULT(STDMETHODCALLTYPE* GetMemoryLoad)(
        MyHostMemoryManager* This,
        /* [out] */ DWORD* pMemoryLoad,
        /* [out] */ SIZE_T* pAvailableBytes);

    HRESULT(STDMETHODCALLTYPE* RegisterMemoryNotificationCallback)(
        MyHostMemoryManager* This,
        /* [in] */ ICLRMemoryNotificationCallback* pCallback);

    HRESULT(STDMETHODCALLTYPE* NeedsVirtualAddressSpace)(
        MyHostMemoryManager* This,
        /* [in] */ LPVOID startAddress,
        /* [in] */ SIZE_T size);

    HRESULT(STDMETHODCALLTYPE* AcquiredVirtualAddressSpace)(
        MyHostMemoryManager* This,
        /* [in] */ LPVOID startAddress,
        /* [in] */ SIZE_T size);

    HRESULT(STDMETHODCALLTYPE* ReleasedVirtualAddressSpace)(
        MyHostMemoryManager* This,
        /* [in] */ LPVOID startAddress);

    END_INTERFACE
} MyHostMemoryManagerVtbl;

interface MyHostMemoryManager
{
    CONST_VTBL struct MyHostMemoryManagerVtbl* lpVtbl;
};

typedef interface MyHostControl MyHostControl;

typedef struct MyHostControlVtbl
{
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE* QueryInterface)(
        MyHostControl* This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */
            _COM_Outptr_  void** ppvObject);

    ULONG(STDMETHODCALLTYPE* AddRef)(
        MyHostControl* This);

    ULONG(STDMETHODCALLTYPE* Release)(
        MyHostControl* This);

    HRESULT(STDMETHODCALLTYPE* GetHostManager)(
        MyHostControl* This,
            /* [in] */ REFIID riid,
            /* [out] */ void** ppObject);

    HRESULT(STDMETHODCALLTYPE* SetAppDomainManager)(
        MyHostControl* This,
        /* [in] */ DWORD dwAppDomainID,
        /* [in] */ IUnknown* pUnkAppDomainManager);

    END_INTERFACE
} MyHostControlVtbl;

interface MyHostControl
{
    CONST_VTBL struct MyHostControlVtbl* lpVtbl;
};

typedef interface MyHostMalloc MyHostMalloc;

typedef struct MyHostMallocVtbl
{
    BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)(
            MyHostMalloc* This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */
            _COM_Outptr_  void** ppvObject);

    ULONG(STDMETHODCALLTYPE* AddRef)(
        MyHostMalloc* This);

    ULONG(STDMETHODCALLTYPE* Release)(
        MyHostMalloc* This);

    HRESULT(STDMETHODCALLTYPE* Alloc)(
        MyHostMalloc* This,
        /* [in] */ SIZE_T cbSize,
        /* [in] */ EMemoryCriticalLevel eCriticalLevel,
        /* [out] */ void** ppMem);

    HRESULT(STDMETHODCALLTYPE* DebugAlloc)(
        MyHostMalloc* This,
        /* [in] */ SIZE_T cbSize,
        /* [in] */ EMemoryCriticalLevel eCriticalLevel,
        /* [annotation][in] */
        _In_   char* pszFileName,
        /* [in] */ int iLineNo,
        /* [annotation][out] */
        _Outptr_result_maybenull_  void** ppMem);

    HRESULT(STDMETHODCALLTYPE* Free)(
        MyHostMalloc* This,
        /* [in] */ void* pMem);

    END_INTERFACE
} MyHostMallocVtbl;

interface MyHostMalloc
{
    CONST_VTBL struct MyHostMallocVtbl* lpVtbl;
    HANDLE hHeap;
};

MyHostMalloc*        NewMyHostMalloc();
MyHostMemoryManager* NewMyHostMemoryManager();
MyHostControl*       NewMyHostControl();