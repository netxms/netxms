/* $Id$ */

//#include <windows.h>
#include <Python.h>
#include <nms_common.h>
#include <nms_agent.h>

#ifdef _WIN32
#define SKELETON_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define SKELETON_EXPORTABLE
#endif

/*
 * static vars
 *
 */
static TCHAR m_szModule[128];
static int m_nRetType;
static PyThreadState *m_gtState = NULL;


/*
 * Handlers
 *
 */
static LONG H_Param(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	ret_uint(pValue, 0x01000000);
	return SYSINFO_RC_SUCCESS;
}

/*
 * cleanup handler
 *
 */
static void UnloadHandler(void)
{
	printf("onUnload\n");
	if (m_gtState != NULL)
	{
		printf("onUnload-DO\n");
		PyEval_AcquireThread(m_gtState);
		m_gtState = NULL;
		Py_Finalize();
	}
}

static NETXMS_SUBAGENT_PARAM m_param[] = 
{
	"", H_Param, NULL, 0, "Custom Python-scripted handler"
};

//
// Subagent information
//
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("nxPython"),
	_T("0.0.1"),
	NULL, // init
	UnloadHandler, // cleanup
	NULL, // command
	0, m_param,
	0, NULL,
	0, NULL
};

//
// Entry point for NetXMS agent
//

#ifdef _NETWARE
extern "C" BOOL SKELETON_EXPORTABLE NxSubAgentInit_SKELETON(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
#else
extern "C" BOOL SKELETON_EXPORTABLE NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
#endif
{
	BOOL bRet = FALSE;
	TCHAR szCommand[128];

	NX_CFG_TEMPLATE cfgTemplate[] =
	{
		{ "Command", CT_STRING, 0, 0, sizeof(szCommand), 0, szCommand },
		{ "Module", CT_STRING, 0, 0, sizeof(m_szModule), 0, m_szModule },
		{ "ReturnType", CT_LONG, 0, 0, 0, 0, &m_nRetType },
	};

	szCommand[0] = 0;

	// load config
	if (NxLoadConfig(pszConfigFile, _T("nxPython"), cfgTemplate, FALSE)
			== NXCFG_ERR_OK && szCommand[0] != 0)
	{
		Py_Initialize();
		m_gtState = PyEval_SaveThread();
		if (m_gtState != NULL)
		{
			bRet = TRUE;

			m_info.dwNumParameters = 1;
			nx_strncpy(m_param[0].szName, szCommand, sizeof(m_param[0].szName));
			m_param[0].iDataType = m_nRetType;

			*ppInfo = &m_info;
		}
	}
	else
	{
		fprintf(stderr, "Can't load config\n");
	}
	return bRet;
}


//
// DLL entry point
//
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}

#endif


//
// NetWare library entry point
//
#ifdef _NETWARE

int _init(void)
{
	return 0;
}

int _fini(void)
{
	return 0;
}

#endif
