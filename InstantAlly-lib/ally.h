#ifndef HTAALLYHDR_ALLY
#define HTAALLYHDR_ALLY

#include <Windows.h>
#include <array>

#ifndef HTAALLY_IMPLEMENTATION
	#ifdef ARCH_X64
		#ifdef _DEBUG
			#pragma comment(lib, "InstantAllyDbg-x64.lib")
		#else
			#pragma comment(lib, "InstantAlly-x64.lib")
		#endif
	#else
		#ifdef _DEBUG
			#pragma comment(lib, "InstantAllyDbg-x86.lib")
		#else
			#pragma comment(lib, "InstantAlly-x86.lib")
		#endif
	#endif
#endif

typedef HRESULT (CALLBACK* PFNALLYPROC)(VARIANTARG *pVarArgs, UINT cArgs, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, LPVOID lpExtraData);

HRESULT STDAPICALLTYPE InstantAllyStartup( PFNALLYPROC pfnAllyController, LPVOID lpExtraData, DWORD dwFlags = 0 );

#define ALLY_E_HANDSHAKE_FAILED            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1)
#define ALLY_E_ACK_TIMED_OUT            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 2)

#define GetLastErrorAsHR HRESULT_FROM_WIN32(GetLastError())

HRESULT STDAPICALLTYPE GetHtaProcess(PHANDLE phProcess, DWORD dwDesiredAccess = SYNCHRONIZE);
template <std::size_t N> HRESULT WaitTransparently(const std::array<HANDLE, N>& arrHandles, DWORD dwMilliseconds, BOOL fWaitAll = FALSE);

void STDAPICALLTYPE AllyErrorExitW(HRESULT hrErrorCode, LPCWSTR lpErrorMessage, LPCWSTR lpErrorCaption = L"Instant Ally Error");
void STDAPICALLTYPE AllyErrorExitW(DWORD dwWin32ErrorCode, LPCWSTR lpErrorMessage, LPCWSTR lpErrorCaption = L"Instant Ally Error");
void STDAPICALLTYPE AllyErrorExitA(HRESULT hrErrorCode, LPCSTR lpErrorMessage, LPCSTR lpErrorCaption = "Instant Ally Error");
void STDAPICALLTYPE AllyErrorExitA(DWORD dwWin32ErrorCode, LPCSTR lpErrorMessage, LPCSTR lpErrorCaption = "Instant Ally Error");

#ifdef UNICODE
#define AllyErrorExit AllyErrorExitW
#else
#define AllyErrorExit AllyErrorExitA
#endif

#endif