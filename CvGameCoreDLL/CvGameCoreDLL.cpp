#include "CvGameCoreDLL.h"

#include "CvGameCoreDLLUndefNew.h"

char *get_dll_folder();

// Logging
#include <cstdlib>
#include <string.h>
#include <fstream>
bool logactive(false);
bool invert_LoadLibraryChanges(false);
std::ofstream logfile;
void logMsg(char* format, ... )
{
  if (logactive == false) {
    if (invert_LoadLibraryChanges){
      logfile.open("DLL.log", std::ofstream::out | std::ofstream::app); // Append on file
    }else{ 
      logfile.open("DLL.log"); // New file
    }
    if (logfile.is_open()) {
      logactive = true;
    }
  }
  if (logactive == false) {
    return;
  }

	static const int __logMsg_kBufSize = 2048;
	static char __logMsg_buf[__logMsg_kBufSize];
	_vsnprintf( __logMsg_buf, __logMsg_kBufSize-4, format, (char*)(&format+1) );

  logfile << __logMsg_buf << std::endl;
  logfile.flush(); 
}

/*struct DLLMetadata {
  uint original_pointer;
  int num_loading;
  const char *mod_dlls;
} Hm, needs to be placed alligned to mod_dlls string. It is easier to use two different DLL versions.*/

// For Wine/Non Visual Studio environment: Re-route Logging.
#undef OutputDebugString
#define OutputDebugString(x) logMsg("%s", x)
// Logging END

#include <new>

#include "CvGlobals.h"
#include "FProfiler.h"
#include "CvDLLInterfaceIFaceBase.h"

//
// operator global new and delete override for gamecore DLL 
//
void *__cdecl operator new(size_t size)
{
	if (gDLL)
	{
		return gDLL->newMem(size, __FILE__, __LINE__);
	}
	return malloc(size);
}

void __cdecl operator delete (void *p)
{
	if (gDLL)
	{
		gDLL->delMem(p, __FILE__, __LINE__);
	}
	else
	{
		free(p);
	}
}

void* operator new[](size_t size)
{
	if (gDLL)
		return gDLL->newMemArray(size, __FILE__, __LINE__);
	return malloc(size);
}

void operator delete[](void* pvMem)
{
	if (gDLL)
	{
		gDLL->delMemArray(pvMem, __FILE__, __LINE__);
	}
	else
	{
		free(pvMem);
	}
}

void *__cdecl operator new(size_t size, char* pcFile, int iLine)
{
	return gDLL->newMem(size, pcFile, iLine);
}

void *__cdecl operator new[](size_t size, char* pcFile, int iLine)
{
	return gDLL->newMem(size, pcFile, iLine);
}

void __cdecl operator delete(void* pvMem, char* pcFile, int iLine)
{
	gDLL->delMem(pvMem, pcFile, iLine);
}

void __cdecl operator delete[](void* pvMem, char* pcFile, int iLine)
{
	gDLL->delMem(pvMem, pcFile, iLine);
}


void* reallocMem(void* a, unsigned int uiBytes, const char* pcFile, int iLine)
{
	return gDLL->reallocMem(a, uiBytes, pcFile, iLine);
}

unsigned int memSize(void* a)
{
	return gDLL->memSize(a);
}

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved)
{
  switch( ul_reason_for_call ) {
    case DLL_PROCESS_ATTACH:
      {

        // set timer precision
        MMRESULT iTimeSet = timeBeginPeriod(1);		// set timeGetTime and sleep resolution to 1 ms, otherwise it's 10-16ms
        FAssertMsg(iTimeSet==TIMERR_NOERROR, "failed setting timer resolution to 1 ms");

        char *mod_path = get_dll_folder();
        if( mod_path == NULL ) return false;

        const char *orig_prefix = "Assets\\%s";
        const char *changed_prefix = mod_path; // Just a prefix because "\%s" is not included.  
                                               // But the string is long enough to distict it from orig_prefix/random bytes.
        /* Every binary (Normal Civ4:BTS exe, Pitboss exe, Steam Civ4:BTS exe) containing a push instruction
         * with a pointer to the *orig_prefix content.
         * We comparing the pointer positions with orig_prefix to detect the binary variant. This address
         * needs to be stored to restore it during the second DLL loading.
         */
        const uint uiPotentialCodeAddresses[] = {
          /* uiCodeAddress_Normal  */ 0x004D6446,
          /* uiCodeAddress_Pitboss */ 0x004906E6,
          /* uiCodeAddress_Steam   */ 0x0 // TODO
        };
        uint uiCodeAddress = 0x0;

        for(int i=0; i<ARRAYSIZE(uiPotentialCodeAddresses); ++i){
          const uint pushAdr = uiPotentialCodeAddresses[i];
          if (pushAdr && *((char *)pushAdr) == 0x68 &&
              strncmp(orig_prefix, *((const char **)(pushAdr+1)), strlen(orig_prefix)) == 0){
            uiCodeAddress = pushAdr;
            break;
          }
          if (pushAdr && *((char *)pushAdr) == 0x68 &&
              strncmp(changed_prefix, *((const char **)(pushAdr+1)), strlen(changed_prefix)) == 0){

            invert_LoadLibraryChanges = true;
            uiCodeAddress = pushAdr;
            break;
          }
        }

        // The DLL is being loaded into the virtual address space of the current process as a result of the process starting up 
        // This line is not on the beginning of the case because Logging behaviour may depend on 'invert_LoadLibraryChanges' value.
        OutputDebugString("DLL_PROCESS_ATTACH\n");

        if (uiCodeAddress == 0x0){
          logMsg("Can not detect type of binary: CodeAddress undefined.");
          free(mod_path);
          return false;
        }

        if (invert_LoadLibraryChanges) {
          free(mod_path);
          // Remove protection
          DWORD uiOldProtect, uiDummy;
          size_t uiSize = 5;

          if (0 == VirtualProtect(reinterpret_cast<LPVOID>(uiCodeAddress),
                /*	Will affect an entire page, not just the few bytes we need. Through
                    SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo); sysInfo.dwPageSize;
                    I see a page size of 4 KB. */
                uiSize, PAGE_EXECUTE_READWRITE, &uiOldProtect))
          {
            logMsg("Failed to unprotect memory for runtime patch");
            break;
          }

          // Overwrite pointer in push instruction in Civ4:BTS exe / Pitboss exe (TODO: Steam binary)
          const char *original_pointer = *((const char **)(uiCodeAddress+1))-4;
          *((const char **)(uiCodeAddress+1)) = original_pointer;

          // Clear memory allocated during first run of dll loading
          free((void *)original_pointer);

          // Restore protection
          if (0 == VirtualProtect(reinterpret_cast<LPVOID>(uiCodeAddress),
                uiSize, uiOldProtect, &uiDummy))
          {
            logMsg("Failed to restore memory protection");
            break;
          }

          logMsg("Second stage finished");
          return true;
        }else{

          // Get path variables
          const char *env_path = getenv("PATH");
          logMsg("Path: %s", env_path);
          logMsg("Assets folder of mod: %s", mod_path);

          // Create new string containing '...\Mods\[MOD]\DLLs' path
          std::string mod_dlls(mod_path);
          size_t f = mod_dlls.rfind("\\Assets");
          if (f == std::string::npos) {
            break;
          }
          mod_dlls.erase(f);
          mod_dlls.append("\\DLLs");

          // Update path environment variable
          std::string path("PATH=");
          path.append(env_path);
          path.append(";");
          path.append(mod_dlls);
          path.append(";");
          putenv(path.c_str()); // C4996
          free(mod_path);
          logMsg("New path: %s", getenv("PATH"));


          /* Save mod_path char array in new array, but also prepend id with pointer to original string
           * p='[original_pointer][new_string]'
           */
          mod_dlls.append("\\%s");
          size_t mod_dlls_len = strlen(mod_dlls.c_str());
          void *p = calloc(4 + mod_dlls_len + 1, sizeof(char));
          *((uint *)p) = uiCodeAddress;
          memcpy((char *)p+4, mod_dlls.c_str(), mod_dlls_len + 1);
          const char *p_mod_path = (const char *)p+4;

          // Remove protection
          DWORD uiOldProtect, uiDummy;
          size_t uiSize = 5;

          if (0 == VirtualProtect(reinterpret_cast<LPVOID>(uiCodeAddress),
                /*	Will affect an entire page, not just the few bytes we need. Through
                    SYSTEM_INFO sysInfo; GetSystemInfo(&sysInfo); sysInfo.dwPageSize;
                    I see a page size of 4 KB. */
                uiSize, PAGE_EXECUTE_READWRITE, &uiOldProtect))
          {
            logMsg("Failed to unprotect memory for runtime patch");
            break;
          }

          // Overwrite pointer in push instruction in Civ4:BTS exe / Pitboss exe (TODO: Steam binary)
          *((const char **)(uiCodeAddress+1)) = p_mod_path;

          // Restore protection
          if (0 == VirtualProtect(reinterpret_cast<LPVOID>(uiCodeAddress),
                uiSize, uiOldProtect, &uiDummy))
          {
            logMsg("Failed to restore memory protection");
            break;
          }
        }

        logMsg("First stage finished");
        // Failing LoadLibrary to provoke loading of [Mod]\DLLs\CvGameCoreDLL.dll
        return false;
      }
		break;
	case DLL_THREAD_ATTACH:
		// OutputDebugString("DLL_THREAD_ATTACH\n");
		break;
	case DLL_THREAD_DETACH:
		// OutputDebugString("DLL_THREAD_DETACH\n");
		break;
	case DLL_PROCESS_DETACH:
		OutputDebugString("DLL_PROCESS_DETACH\n");
		timeEndPeriod(1);
		GC.setDLLIFace(NULL);
		break;
	}
	
	return true;	// success
}

//
// enable dll profiler if necessary, clear history
//
void startProfilingDLL()
{
	if (GC.isDLLProfilerEnabled())
	{
		gDLL->ProfilerBegin();
	}
}

//
// dump profile stats on-screen
//
void stopProfilingDLL()
{
	if (GC.isDLLProfilerEnabled())
	{
		gDLL->ProfilerEnd();
	}
}

/* Return folder of this DLL/EXE.
 *
 * Free returned char after usage! */
char *get_dll_folder(){

#define MAX_PARAM 1000
	char *path = (char *)calloc( (MAX_PARAM + 1), sizeof(char));
	HMODULE hm = NULL;

	if (!GetModuleHandleExA( /*0x04 | 0x02*/ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				(LPCSTR) &get_dll_folder,
				&hm))
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle returned %d\n", ret);
	}
	GetModuleFileNameA(hm, path, MAX_PARAM);

	// path variable should now contain the full filepath to localFunc
	// Strip dll filename.
	char *last_slash = strrchr(path, '\\');
	*last_slash = '\0';
	//fprintf(stdout, "%s\n", path);

	return path;
}
