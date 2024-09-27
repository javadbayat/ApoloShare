#define HTAALLY_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#include "ally.h"
#include <TlHelp32.h>
#include <stdio.h>
#include <stdlib.h>

#define ACK_TIMEOUT_INTERVAL 10000

#define DISPID_METHOD_ACK                    (1)
#define DISPID_PROP_ProcessID                (2)
#define DISPID_PROP_FullName                (3)
#define DISPID_PROP_SessionID                (4)

class CInstantAlly : public IDispatch
{
private:
    ULONG m_cRef;
    PFNALLYPROC m_pfnAllyController;
    PVOID m_pvExtraData;
    HANDLE m_hEndOfHandshake;
    OLECHAR *m_pszSessionID;
public:
    typedef std::array<HANDLE, 1>::iterator PEVENT;

    CInstantAlly(PFNALLYPROC pfnAllyController, LPVOID lpExtraData)
    {
        m_cRef = 1;
        m_pfnAllyController = pfnAllyController;
        m_pvExtraData = lpExtraData;
        m_hEndOfHandshake = NULL;
        m_pszSessionID = NULL;
    }

    ~CInstantAlly(void)
    {
        if (m_hEndOfHandshake)
            CloseHandle(m_hEndOfHandshake);
        
        if (m_pszSessionID)
            CoTaskMemFree(m_pszSessionID);
    }

    //IUnknown Members
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv)
    {
        HRESULT hr = S_OK;
        if (ppv)
        {
            *ppv = NULL;

            if (IsEqualIID(riid, IID_IDispatch) || IsEqualIID(riid, IID_IUnknown))
                *ppv = (IDispatch *) (this);
            else
                hr = E_NOINTERFACE;

            if (*ppv)
                AddRef();
        }
        else
            hr = E_INVALIDARG;

        return hr;
    }

    ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return((ULONG) (InterlockedIncrement((long *) &m_cRef)));
    }

    ULONG STDMETHODCALLTYPE Release(void)
    {
        ULONG cRef = (ULONG) (InterlockedDecrement((long *) &m_cRef));
        if (!cRef)
            delete this;

        return cRef;
    }

    //IDispatch Members
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT FAR* pctInfo)
    {
        if (!pctInfo)
            return E_INVALIDARG;

        *pctInfo = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo FAR* FAR* ppTInfo)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgDispId)
    {
        HRESULT hr = S_OK;
        UINT i;

        for (i = 0;i < cNames;i++)
        {
            if (!_wcsicmp(rgszNames[i], L"ACK"))
                rgDispId[i] = DISPID_METHOD_ACK;
            else if (!_wcsicmp(rgszNames[i], L"ProcessID"))
                rgDispId[i] = DISPID_PROP_ProcessID;
            else if (!_wcsicmp(rgszNames[i], L"FullName"))
                rgDispId[i] = DISPID_PROP_FullName;
            else if (!_wcsicmp(rgszNames[i], L"SessionID"))
                rgDispId[i] = DISPID_PROP_SessionID;
            else
            {
                rgDispId[i] = DISPID_UNKNOWN;
                hr = DISP_E_UNKNOWNNAME;
            }
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pDispParams, VARIANT FAR* pVarResult, EXCEPINFO FAR* pExcepInfo, UINT FAR* puArgErr)
    {
        HRESULT hr = S_OK;
        VARIANT *pVarResult2;
        VARIANT vTemp;

        if (!pDispParams)
            hr = E_INVALIDARG;

        if (!IsEqualIID(riid, IID_NULL))
            hr = DISP_E_UNKNOWNINTERFACE;

        if (pVarResult)
            pVarResult2 = pVarResult;
        else
            VariantInit(pVarResult2 = &vTemp);

        if (SUCCEEDED(hr))
        {
            switch (dispIdMember)
            {
            case DISPID_METHOD_ACK :
                {
                    if (wFlags & DISPATCH_METHOD)
                    {
                        if (pDispParams->cArgs == 0)
                            hr = StartSession();
                        else
                            hr = DISP_E_BADPARAMCOUNT;
                    }
                    else
                        hr = DISP_E_MEMBERNOTFOUND;
                    break;
                }
            case DISPID_PROP_ProcessID :
                {
                    if (wFlags & DISPATCH_PROPERTYGET)
                    {
                        if (pDispParams->cArgs == 0)
                        {
                            pVarResult2->vt = VT_UI4;
                            hr = get_ProcessID(&(pVarResult2->ulVal));
                        }
                        else
                            hr = DISP_E_BADPARAMCOUNT;
                    }
                    else
                        hr = DISP_E_MEMBERNOTFOUND;
                    break;
                }
            case DISPID_PROP_FullName :
                {
                    if (wFlags & DISPATCH_PROPERTYGET)
                    {
                        if (pDispParams->cArgs == 0)
                        {
                            pVarResult2->vt = VT_BSTR;
                            hr = get_FullName(&(pVarResult2->bstrVal));
                        }
                        else
                            hr = DISP_E_BADPARAMCOUNT;
                    }
                    else
                        hr = DISP_E_MEMBERNOTFOUND;
                    break;
                }
            case DISPID_PROP_SessionID :
                {
                    if (wFlags & DISPATCH_PROPERTYGET)
                    {
                        if (pDispParams->cArgs == 0)
                        {
                            pVarResult2->vt = VT_BSTR;
                            hr = get_SessionID(&(pVarResult2->bstrVal));
                        }
                        else
                            hr = DISP_E_BADPARAMCOUNT;
                    }
                    else
                        hr = DISP_E_MEMBERNOTFOUND;
                    break;
                }
            case DISPID_VALUE :
                {
                    if (wFlags & DISPATCH_METHOD)
                    {
                        if (m_pszSessionID)
                            hr = m_pfnAllyController(pDispParams->rgvarg, pDispParams->cArgs, pVarResult2, pExcepInfo, m_pvExtraData);
                        else
                            hr = E_FAIL;
                    }
                    else
                        hr = DISP_E_MEMBERNOTFOUND;
                    break;
                }
            default :
                {
                    hr = DISP_E_MEMBERNOTFOUND;
                    break;
                }
            }
        }

        if (!pVarResult)
            VariantClear(pVarResult2);

        return hr;
    }

    //Ally Members
    HRESULT STDMETHODCALLTYPE get_ProcessID(ULONG *pulProcID)
    {
        *pulProcID = GetCurrentProcessId();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE get_FullName(BSTR *pbsFullName)
    {
        HRESULT hr = S_OK;
        PWSTR pwszFullName = new WCHAR[600];

        if (pwszFullName)
        {
            if (GetModuleFileName(NULL, pwszFullName, 600))
                *pbsFullName = SysAllocString(pwszFullName);
            else
                hr = GetLastErrorAsHR;

            delete[] pwszFullName;
        }
        else
            hr = E_OUTOFMEMORY;

        return hr;
    }

    HRESULT STDMETHODCALLTYPE get_SessionID(BSTR *pbsSessionID)
    {
        *pbsSessionID = SysAllocString(m_pszSessionID);
        return (*pbsSessionID) ? S_OK : E_OUTOFMEMORY;
    }

    HRESULT STDMETHODCALLTYPE Realize(LPSTR lpszOBJREF, PEVENT &pEndOfHandshake)
    {
        HRESULT hr = S_OK;
        IBindCtx *pBindContext = NULL;
        IMoniker *pObjRefMon = NULL;
        OLECHAR *strObjRef = NULL;

        m_hEndOfHandshake = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_hEndOfHandshake)
        {
            hr = CreateBindCtx(0, &pBindContext);
            if (SUCCEEDED(hr))
            {
                hr = CreateObjrefMoniker((IUnknown *) this, &pObjRefMon);
                if (SUCCEEDED(hr))
                {
                    hr = pObjRefMon->GetDisplayName(pBindContext, NULL, &strObjRef);
                    if (SUCCEEDED(hr))
                    {
                        wcstombs(lpszOBJREF, strObjRef, MAX_PATH);
                        CoTaskMemFree(strObjRef);
                    }

                    pObjRefMon->Release();
                }
                
                pBindContext->Release();
            }

            if (SUCCEEDED(hr))
                *pEndOfHandshake = m_hEndOfHandshake;
            else
                CloseHandle(m_hEndOfHandshake);
        }
        else
            hr = GetLastErrorAsHR;

        return hr;
    }

    HRESULT STDMETHODCALLTYPE StartSession(void)
    {
        HRESULT hr = S_OK;
        GUID newGUID;

        if (m_pszSessionID)
            return E_FAIL;

        if (SetEvent(m_hEndOfHandshake))
        {
            hr = CoCreateGuid(&newGUID);
            if (SUCCEEDED(hr))
                hr = StringFromIID(newGUID, &m_pszSessionID);
        }
        else
            hr = GetLastErrorAsHR;

        return hr;
    }
};

typedef CInstantAlly *PInstantAlly;

template <std::size_t N> HRESULT WaitTransparently(const std::array<HANDLE, N>& arrHandles, DWORD dwMilliseconds, BOOL fWaitAll) {
    register DWORD res = MsgWaitForMultipleObjects((DWORD)(arrHandles.size()), arrHandles.data(), fWaitAll, dwMilliseconds, 0x000001FF);
    if (res == WAIT_FAILED)
        return GetLastErrorAsHR;
    else if (res == WAIT_TIMEOUT)
        return S_FALSE;
    else
        return S_OK;
}

HRESULT STDAPICALLTYPE InstantAllyStartup( PFNALLYPROC pfnAllyController, LPVOID lpExtraData, DWORD dwFlags ) {
    HRESULT hr = S_OK;
    PInstantAlly pAllyService;
    CHAR szObjRef[MAX_PATH];
    CHAR szHandshakeBuffer[MAX_PATH];
    std::array<HANDLE, 1> hAckReceived;
    PSTR result;

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    pAllyService = new CInstantAlly(pfnAllyController, lpExtraData);
    if (pAllyService)
    {
        hr = pAllyService->Realize(szObjRef, hAckReceived.begin());
        if (SUCCEEDED(hr))
        {
            // Begin the handshake with the HTA
            result = fgets(szHandshakeBuffer, sizeof(szHandshakeBuffer), stdin);
            if (result && (!strcmp(result, "SYN\n")))
            {
                strcat(szObjRef, "\r\n");
                if (fputs(szObjRef, stdout) >= 0)
                {
                    result = fgets(szHandshakeBuffer, sizeof(szHandshakeBuffer), stdin);
                    if (result && (!strcmp(result, "SYN/ACK\n")))
                    {
                        // Wait until the final ACK is received
                        hr = WaitTransparently(hAckReceived, ACK_TIMEOUT_INTERVAL);
                        if (hr == S_FALSE)
                            hr = ALLY_E_ACK_TIMED_OUT;
                    }
                    else
                        hr = ALLY_E_HANDSHAKE_FAILED;
                }
                else
                    hr = ALLY_E_HANDSHAKE_FAILED;
            }
            else
                hr = ALLY_E_HANDSHAKE_FAILED;

            SecureZeroMemory(szObjRef, MAX_PATH);
        }

        if (FAILED(hr))
            delete pAllyService;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT STDAPICALLTYPE GetHtaProcess(PHANDLE phProcess, DWORD dwDesiredAccess)
{
    HRESULT hr = S_OK;
    DWORD currentID = GetCurrentProcessId();
    PROCESSENTRY32 pe;
    DWORD &parentID = pe.th32ParentProcessID;
    HANDLE hSnapshot;
    DWORD dwLastError;

    *phProcess = NULL;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        ZeroMemory(&pe, sizeof(PROCESSENTRY32));
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe))
        {
            while (TRUE)
            {
                if (pe.th32ProcessID == currentID)
                    break;

                if (!Process32Next(hSnapshot, &pe))
                {
                    dwLastError = GetLastError();
                    if (dwLastError == ERROR_NO_MORE_FILES)
                        hr = __HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                    else
                        hr = __HRESULT_FROM_WIN32(dwLastError);

                    break;
                }
            }

            if (SUCCEEDED(hr))
            {
                *phProcess = OpenProcess(dwDesiredAccess, FALSE, parentID);
                if (*phProcess == NULL)
                    hr = GetLastErrorAsHR;
            }
        }
        else
            hr = GetLastErrorAsHR;
    }
    else
        hr = GetLastErrorAsHR;

    return hr;
}

void STDAPICALLTYPE AllyErrorExitW(HRESULT hrErrorCode, LPCWSTR lpErrorMessage, LPCWSTR lpErrorCaption)
{
    size_t stLen = wcslen(lpErrorMessage);
    PWSTR pMsg = new WCHAR[stLen + 30];
    wcscpy(pMsg, lpErrorMessage);
    wcscat(pMsg, L"\r\nError code: 0x");
    _ltow(hrErrorCode, pMsg + stLen + 16, 16);
    MessageBoxW(NULL, pMsg, lpErrorCaption, MB_OK | MB_ICONSTOP);
    delete[] pMsg;
    ExitProcess(1);
}

void STDAPICALLTYPE AllyErrorExitA(HRESULT hrErrorCode, LPCSTR lpErrorMessage, LPCSTR lpErrorCaption)
{
    size_t stLen = strlen(lpErrorMessage);
    PSTR pMsg = new CHAR[stLen + 30];
    strcpy(pMsg, lpErrorMessage);
    strcat(pMsg, "\r\nError code: 0x");
    _ltoa(hrErrorCode, pMsg + stLen + 16, 16);
    MessageBoxA(NULL, pMsg, lpErrorCaption, MB_OK | MB_ICONSTOP);
    delete[] pMsg;
    ExitProcess(1);
}

void STDAPICALLTYPE AllyErrorExitW(DWORD dwWin32ErrorCode, LPCWSTR lpErrorMessage, LPCWSTR lpErrorCaption)
{
    AllyErrorExitW(__HRESULT_FROM_WIN32(dwWin32ErrorCode), lpErrorMessage, lpErrorCaption);
}

void STDAPICALLTYPE AllyErrorExitA(DWORD dwWin32ErrorCode, LPCSTR lpErrorMessage, LPCSTR lpErrorCaption)
{
    AllyErrorExitA(__HRESULT_FROM_WIN32(dwWin32ErrorCode), lpErrorMessage, lpErrorCaption);
}