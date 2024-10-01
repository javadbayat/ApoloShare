#define _CRT_SECURE_NO_WARNINGS
#include <ally.h>
#include <fineftp\server.h>

#pragma comment(lib, "fineftp-server.lib")

#define FTP_SERVER_PORT 8007

// Ally Commands
#define AC_INIT_SERVICE 1
#define AC_START_SERVICE 2
#define AC_GET_QR_CODE 3

HRESULT CALLBACK AllyCore( VARIANTARG *pVarArgs, UINT cArgs, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, LPVOID lpExtraData ) {
	HRESULT hr = S_OK;
	VARIANT vTemp;
	fineftp::FtpServer** ppMyServer = (fineftp::FtpServer**) lpExtraData;

	VariantInit(&vTemp);

	switch ( pVarArgs[cArgs - 1].lVal ) {
	case AC_INIT_SERVICE :
		{
			LPSAFEARRAY &refIPAddress = pVarArgs[0].parray;
			VARIANT *pBytesOfIP = NULL;
			hr = SafeArrayAccessData( refIPAddress, (void **) &pBytesOfIP );
			if (SUCCEEDED(hr)) {
				std::string strIPAddress = "";
				for (int i = 0; i < 4; i++) {
					hr = VariantChangeType( &vTemp, &pBytesOfIP[i], 0, VT_INT);
					if (FAILED(hr))
						break;

					if (i)
						strIPAddress += ".";

					strIPAddress += std::to_string( vTemp.intVal );
				}

				if (SUCCEEDED(hr)) {
					*ppMyServer = new fineftp::FtpServer( strIPAddress, FTP_SERVER_PORT );
					if (*ppMyServer == NULL)
						hr = E_OUTOFMEMORY;
				}

				SafeArrayUnaccessData( refIPAddress );
			}

			break;
		}
	case AC_START_SERVICE :
		{
			BSTR &bsLocalRootPath = pVarArgs[0].bstrVal;
			size_t sThreadCount;

			size_t len = wcstombs( NULL, bsLocalRootPath, 0 );
			PSTR pszBuffer = new CHAR[len];
			if (pszBuffer) {
				wcstombs( pszBuffer, bsLocalRootPath, len );
				std::string strLocalRootPath( pszBuffer );
				delete[] pszBuffer;

				if ((*ppMyServer)->addUserAnonymous( strLocalRootPath, fineftp::Permission::All )) {
					hr = VariantChangeType( &vTemp, &pVarArgs[1], 0, VT_INT );
					if (SUCCEEDED(hr)) {
						sThreadCount = (size_t)(pVarArgs[1].intVal);
						if ((*ppMyServer)->start( sThreadCount ) == false)
							hr = E_FAIL;
					}
				}
				else
					hr = E_FAIL;
			}
			else
				hr = E_OUTOFMEMORY;

			break;
		}
	case AC_GET_QR_CODE :
		{
			// Shoot, I really don't know how to implement this!
			break;
		}
	}

	VariantClear(&vTemp);

	return hr;
}

int main( int argc, char *argv[] ) {
	HRESULT hr = S_OK;
	PWSTR pwszErrorMsg = NULL;
	fineftp::FtpServer* pMyServer = NULL;
	HANDLE pHtaProcess = NULL;
	std::array<HANDLE, 1> arrProcessHandle;

	hr = CoInitialize( NULL );
	if (SUCCEEDED(hr)) {
		hr = InstantAllyStartup( &AllyCore, (LPVOID) &pMyServer );
		if (SUCCEEDED(hr)) {
			hr = GetHtaProcess( &pHtaProcess );
			if (SUCCEEDED(hr)) {
				arrProcessHandle[0] = pHtaProcess;
				hr = WaitTransparently( arrProcessHandle, INFINITE );
				if (FAILED(hr))
					pwszErrorMsg = L"Failed to wait for the HTA to exit.";

				CloseHandle( pHtaProcess );
			}
			else
				pwszErrorMsg = L"Failed to find the MSHTA process.";

			if (SUCCEEDED(hr)) {
				pMyServer->stop();
				delete pMyServer;
			}
		}
		else
			pwszErrorMsg = L"The Ally failed to start up.";

		CoUninitialize();
	}
	else
		pwszErrorMsg = L"Failed to initialize COM.";

	if (FAILED(hr))
		AllyErrorExit( hr, pwszErrorMsg, L"Teleport" );

	return 0;
}