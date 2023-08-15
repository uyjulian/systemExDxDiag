#include "ncbind/ncbind.hpp"

#define INITGUID
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objidl.h>
#include <stdio.h>
#include <initguid.h>
#include <dxdiag.h>

static void DxDiagContainerToTJSStructure(iTJSDispatch2* dictionary, IDxDiagContainer* pDxDiagContainer)
{
	DWORD dwPropCount = 0;
	if (SUCCEEDED(pDxDiagContainer->GetNumberOfProps(&dwPropCount)))
	{
		for (DWORD i = 0; i < dwPropCount; i += 1)
		{
			WCHAR wszPropName[256];
			if (SUCCEEDED(pDxDiagContainer->EnumPropNames(i, wszPropName, sizeof(wszPropName) / sizeof(wszPropName[0]))))
			{
				VARIANT var;
				VariantInit( &var );
				if (SUCCEEDED(pDxDiagContainer->GetProp(wszPropName, &var)))
				{
					tTJSVariant value;
					switch( var.vt )
					{
						case VT_UI4:
							value = (tTVInteger)(var.ulVal);
							break;
						case VT_I4:
							value = (tTVInteger)(var.lVal);
							break;
						case VT_BOOL:
							value = var.boolVal ? TJS_W("true") : TJS_W("false");
							break;
						case VT_BSTR:
							value = var.bstrVal;
							break;
					}
					dictionary->PropSet(TJS_MEMBERENSURE | TJS_IGNOREPROP, wszPropName, NULL, &value, dictionary);
					VariantClear(&var);
				}
			}
		}
	}

	DWORD dwChildCount = 0;
	if (SUCCEEDED(pDxDiagContainer->GetNumberOfChildContainers(&dwChildCount)))
	{
		for (DWORD i = 0; i < dwChildCount; i += 1)
		{
			WCHAR wszChildName[256];
			if (SUCCEEDED(pDxDiagContainer->EnumChildContainerNames(i, wszChildName, sizeof(wszChildName) / sizeof(wszChildName[0]))))
			{
				ttstr child_name = wszChildName;
				if (child_name == TJS_W("DxDiag_DirectPlay"))
				{
					// Avoid an optional component installation prompt.
					continue;
				}
				IDxDiagContainer* pChildContainer = NULL;
				if (SUCCEEDED(pDxDiagContainer->GetChildContainer(wszChildName, &pChildContainer)))
				{
					iTJSDispatch2 * sub_dictionary_dispatch = TJSCreateDictionaryObject();
					tTJSVariant sub_dictionary(sub_dictionary_dispatch, sub_dictionary_dispatch);
					sub_dictionary_dispatch->Release();
					sub_dictionary_dispatch = sub_dictionary.AsObjectNoAddRef();
					dictionary->PropSet(TJS_MEMBERENSURE | TJS_IGNOREPROP, child_name.c_str(), child_name.GetHint(), &sub_dictionary, dictionary);
					DxDiagContainerToTJSStructure(sub_dictionary_dispatch, pChildContainer);
				}
				if (pChildContainer)
				{
					pChildContainer->Release();
					pChildContainer = NULL;
				}
			}
		}
	}
}

struct System
{
	static tTJSVariant getDxDiag()
	{
		iTJSDispatch2 * dictionary_dispatch = TJSCreateDictionaryObject();
		tTJSVariant dictionary(dictionary_dispatch, dictionary_dispatch);
		dictionary_dispatch->Release();
		dictionary_dispatch = dictionary.AsObjectNoAddRef();

		CoInitialize(NULL);

		IDxDiagProvider* pDxDiagProvider = NULL;
		IDxDiagContainer* pDxDiagRoot = NULL;

		if (SUCCEEDED(CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER, IID_IDxDiagProvider, (LPVOID*)&pDxDiagProvider)))
		{
			DXDIAG_INIT_PARAMS dxDiagInitParam;
			memset(&dxDiagInitParam, 0, sizeof(DXDIAG_INIT_PARAMS));

			dxDiagInitParam.dwSize = sizeof(DXDIAG_INIT_PARAMS);
			dxDiagInitParam.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
			dxDiagInitParam.bAllowWHQLChecks = FALSE;
			dxDiagInitParam.pReserved = NULL;

			if (SUCCEEDED(pDxDiagProvider->Initialize(&dxDiagInitParam)))
			{
				if (SUCCEEDED(pDxDiagProvider->GetRootContainer(&pDxDiagRoot)))
				{
					DxDiagContainerToTJSStructure(dictionary_dispatch, pDxDiagRoot);
				}
			}
		}

		if (pDxDiagRoot)
		{
			pDxDiagRoot->Release();
			pDxDiagRoot = NULL;
		}
		if (pDxDiagProvider)
		{
			pDxDiagProvider->Release();
			pDxDiagProvider = NULL;
		}

		CoUninitialize();

		return dictionary;
	}
};

NCB_ATTACH_FUNCTION(getDxDiag, System, System::getDxDiag);
