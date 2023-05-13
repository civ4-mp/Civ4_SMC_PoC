
#define _MBCS
#undef _UNICODE
#include <cstdlib> // needed for update_path_env()
#include <stdlib.h> // needed for update_path_env()

#include "CvGameCoreDLL.h"
#include "SelfMod.h"

#ifdef WITHOUT_EXTERNAL_DLLS
#define	MH_DisableHook(_)
#define MH_Uninitialize()
#define	MH_Initialize()
#define	MH_CreateHook(A,B,C)
#define	MH_EnableHook(_) 
#else
#include "minhook/MinHook.h"
#pragma comment(lib, "minhook/MinHook.x86.lib")
#endif

/* ATTENTION: Manipulating stuff by assembly code and
 * then returning the original function by trampolin just
 * works for 'release' build. Otherwise (debug) the overhead
 * messing up the stack/registers.
 */

/* ================= HOOKS ================== */

/* DiploMenu: Left top element
 * Free register: eax
 *
 * Data: x,y,w,h from esp+0x38 to esp+0x44
 * Default values: see stack trace
 * Stack trace:
 0012FBC0 					esp
 0012FBF8 dd       1C7h ; Ã
 0012FBFC dd       0AEh ; «
 0012FC00 dd       102h
 0012FC04 dd       1F9h
 *
 *
 * Exe: 0x0057F4E0
 */
void FUNC_NAKED Cv_DipLeftTop(){
	// Use pre-evaluated positions on stack in assembler code...
	__asm{
		push ecx
		IIADD(0x38 + 4, eax, ecx, smc::BtS_EXE.diploWinSizes.leftTop.x)
		IIADD(0x3c + 4, eax, ecx, smc::BtS_EXE.diploWinSizes.leftTop.y)
		pop ecx
	}
	COPY_ON_STACK(0x40, eax, smc::BtS_EXE.diploWinSizes.leftTop.w);
	COPY_ON_STACK(0x44, eax, smc::BtS_EXE.diploWinSizes.leftTop.h);

	// Prepare leaderhead scaling in open diplomenu
#if 0
	__asm{
		fld  DWORD PTR smc::BtS_EXE.diploWinSizes.factorMain
		fstp DWORD PTR smc::BtS_EXE.diploWinSizes.factorLeaderhead
	}
#else
  __asm{
    mov eax, [smc::BtS_EXE.diploWinSizes.factorMain]
    mov [smc::BtS_EXE.diploWinSizes.factorLeaderhead], eax
  }
#endif

	// Continue with original function
	//smc::BtS_EXE.diploHooks.Replace_DipLeftTop.trampolin(); // Problematic in DEBUG-Mode.
  // Undo ESP-shift by debug-variables on stack, first force use of jmp, but not 'call'.
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipLeftTop.trampolin);
}

/* As reference here are the assembler lines of Cv_DipRightTop for different compiling options.
 * It shows the different prelude code which influcences the ESP register value.
 * Note that the lines after 'jmp' will not be reached!
 *
 * Usage of FUNC_NAKED prevents this branching and everthing is like case (3).
 *
 * 1) Debugbuild
   CvGameCoreDLL.Cv_DipLeftTop - 55                      - push ebp
   CvGameCoreDLL.Cv_DipLeftTop+1- 8B EC                  - mov ebp,esp
   CvGameCoreDLL.Cv_DipLeftTop+3- 56                     - push esi
   CvGameCoreDLL.Cv_DipLeftTop+4- 51                     - push ecx
   CvGameCoreDLL.Cv_DipLeftTop+5- 8B 44 24 44            - mov eax,[esp+44]
   CvGameCoreDLL.Cv_DipLeftTop+9- 8B 0D EC266F04         - mov ecx,[CvGameCoreDLL.smc::BtS_EXE+BC]
   CvGameCoreDLL.Cv_DipLeftTop+F- 03 C1                  - add eax,ecx
   CvGameCoreDLL.Cv_DipLeftTop+11- 89 44 24 44           - mov [esp+44],eax
   […]
   CvGameCoreDLL.Cv_DipLeftTop+42- 5E                    - pop esi
   CvGameCoreDLL.Cv_DipLeftTop+43- 5D                    - pop ebp
   CvGameCoreDLL.Cv_DipLeftTop+44- FF 25 3C266F04        - jmp dword ptr [CvGameCoreDLL.smc::BtS_EXE+C]
   CvGameCoreDLL.Cv_DipLeftTop+4A- 5E                    - pop esi
   CvGameCoreDLL.Cv_DipLeftTop+4B- 3B EC                 - cmp ebp,esp
   CvGameCoreDLL.Cv_DipLeftTop+4D- E8 4A600000           - call CvGameCoreDLL._RTC_CheckEsp
   CvGameCoreDLL.Cv_DipLeftTop+52- 5D                    - pop ebp
   CvGameCoreDLL.Cv_DipLeftTop+53- C3                    - ret 
 * 
 * 2) Releasebuild with /Oy- Option => push/pop of EBP (?? NO, this is also true with /Oy... ?!)
   CvGameCoreDLL.CyUnit::CyUnit+109F0 - 55                    - push ebp
   CvGameCoreDLL.CyUnit::CyUnit+109F1 - 8B EC                 - mov ebp,esp
   CvGameCoreDLL.CyUnit::CyUnit+109F3 - 51                    - push ecx
   CvGameCoreDLL.CyUnit::CyUnit+109F4 - 8B 44 24 3C           - mov eax,[esp+40]
   CvGameCoreDLL.CyUnit::CyUnit+109F8 - 8B 0D A4D3F403        - mov ecx,[CvGameCoreDLL.dll+45D3A4]
   CvGameCoreDLL.CyUnit::CyUnit+109FE - 03 C1                 - add eax,ecx
   CvGameCoreDLL.CyUnit::CyUnit+10A00 - 89 44 24 3C           - mov [esp+40],eax
   […]
   CvGameCoreDLL.CyUnit::CyUnit+10A31 - FF 25 F4D2F403        - jmp dword ptr [CvGameCoreDLL.dll+45D2F4]
   CvGameCoreDLL.CyUnit::CyUnit+10A37 - 5D                    - pop ebp
   CvGameCoreDLL.CyUnit::CyUnit+10A38 - C3                    - ret 
 *
 * 3) Release build with Civ4 standard options in Makefile
   CvGameCoreDLL.CyPlot::CyPlot+107F0 - 51                    - push ecx
   CvGameCoreDLL.CyPlot::CyPlot+107F1 - 8B 44 24 3C           - mov eax,[esp+3C]
   CvGameCoreDLL.CyPlot::CyPlot+107F5 - 8B 0D A4D3E403        - mov ecx,[CvGameCoreDLL.dll+45D3A4]
   CvGameCoreDLL.CyPlot::CyPlot+107FB - 03 C1                 - add eax,ecx
   CvGameCoreDLL.CyPlot::CyPlot+107FD - 89 44 24 3C           - mov [esp+3C],eax
   […]
   CvGameCoreDLL.CyPlot::CyPlot+10830 - FF 25 F4D2E403        - jmp dword ptr [CvGameCoreDLL.dll+45D2F4]
 */


/* DiploMenu: Left bottom element
 *
 * Note: Similar to above.
 * Stack trace:
 0012FBC0 					esp
 0012FBF8 dd       1C7h ; Ã
 0012FBFC dd       2A7h ; º
 0012FC00 dd       102h
 0012FC04 dd       0E8h ; Þ
 *
 */
void FUNC_NAKED Cv_DipLeftBottom(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x3c, eax, ecx, smc::BtS_EXE.diploWinSizes.leftBottom.x)
			IIADD(0x40, eax, ecx, smc::BtS_EXE.diploWinSizes.leftBottom.y)
			pop ecx
	}
	COPY_ON_STACK(0x40, eax, smc::BtS_EXE.diploWinSizes.leftBottom.w);
	COPY_ON_STACK(0x44, eax, smc::BtS_EXE.diploWinSizes.leftBottom.h);

	//smc::BtS_EXE.diploHooks.Replace_DipLeftBottom.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipLeftBottom.trampolin);
}

/* DiploMenu: Right top element
 *
 * Note: Similar to above.
 * Stack trace:
 0012FBC0 					esp
 0012FBF8 dd       4B6h ; Â
 0012FBFC dd       0AEh ; «
 0012FC00 dd       102h
 0012FC04 dd       1F9h ; ¨
 */
void FUNC_NAKED Cv_DipRightTop(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x3c, eax, ecx, smc::BtS_EXE.diploWinSizes.rightTop.x)
			IIADD(0x40, eax, ecx, smc::BtS_EXE.diploWinSizes.rightTop.y)
			pop ecx
	}
	COPY_ON_STACK(0x40, eax, smc::BtS_EXE.diploWinSizes.rightTop.w);
	COPY_ON_STACK(0x44, eax, smc::BtS_EXE.diploWinSizes.rightTop.h);

	//smc::BtS_EXE.diploHooks.Replace_DipRightTop.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipRightTop.trampolin);
}

/* DiploMenu: Right bottom element
 *
 * Note: Similar to above.
 * Stack trace:
 0012FBC0 					esp
 0012FBF8 dd       4B6h ; Â
 0012FBFC dd       2A7h ; º
 0012FC00 dd       102h
 0012FC04 dd       0E8h ; Þ
 */
void FUNC_NAKED Cv_DipRightBottom(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x3c, eax, ecx, smc::BtS_EXE.diploWinSizes.rightBottom.x)
			IIADD(0x40, eax, ecx, smc::BtS_EXE.diploWinSizes.rightBottom.y)
			pop ecx
	}
	COPY_ON_STACK(0x40, eax, smc::BtS_EXE.diploWinSizes.rightBottom.w);
	COPY_ON_STACK(0x44, eax, smc::BtS_EXE.diploWinSizes.rightBottom.h);

	//smc::BtS_EXE.diploHooks.Replace_DipRightBottom.trampolin();
  TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipRightBottom.trampolin);
}

/* DiploMenu: Headline in MP branch
 * Free register: eax
 *
 * Data: x,y,w,h from esp+0x30 to esp+0x3c
 * Default values: 0x2CF, 0x0AE, 0x1E1, 0x2B
 */
void FUNC_NAKED Cv_DipMidHeadline_MP(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x34, eax, ecx, smc::BtS_EXE.diploWinSizes.midHeadline.x)
			IIADD(0x38, eax, ecx, smc::BtS_EXE.diploWinSizes.midHeadline.y)
			pop ecx
	}
	COPY_ON_STACK(0x38, eax, smc::BtS_EXE.diploWinSizes.midHeadline.w);
	COPY_ON_STACK(0x3c, eax, smc::BtS_EXE.diploWinSizes.midHeadline.h);

	//smc::BtS_EXE.diploHooks.Replace_DipMidHeadline_MP.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipMidHeadline_MP.trampolin);
}

void FUNC_NAKED Cv_DipMidHeadline_SP(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x34, eax, ecx, smc::BtS_EXE.diploWinSizes.midHeadline.x)
			IIADD(0x38, eax, ecx, smc::BtS_EXE.diploWinSizes.midHeadline.y)
			pop ecx
	}
	COPY_ON_STACK(0x38, eax, smc::BtS_EXE.diploWinSizes.midHeadline.w);
	COPY_ON_STACK(0x3c, eax, smc::BtS_EXE.diploWinSizes.midHeadline.h);

	//smc::BtS_EXE.diploHooks.Replace_DipMidHeadline_SP.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipMidHeadline_SP.trampolin);
}

/* DiploMenu: Center in MP branch
 * Free register: eax
 *
 * Data: x,y,w,h from esp+0x30 to esp+0x3c
 * Default values: 0x2CF, 0x0D9, 0x1E1, 0x1CE
 */
void FUNC_NAKED Cv_DipMidCenter_MP(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x34, eax, ecx, smc::BtS_EXE.diploWinSizes.midCenter.x)
			IIADD(0x38, eax, ecx, smc::BtS_EXE.diploWinSizes.midCenter.y)
			pop ecx
	}
	COPY_ON_STACK(0x38, eax, smc::BtS_EXE.diploWinSizes.midCenter.w);
	COPY_ON_STACK(0x3c, eax, smc::BtS_EXE.diploWinSizes.midCenter.h);

	//smc::BtS_EXE.diploHooks.Replace_DipMidCenter_MP.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipMidCenter_MP.trampolin);
}

/* DiploMenu: Leaderhead a.k.a. center in SP branch
 * Free register: eax, edx
 *
 * Data: x,y, decoration_x, decoration_y from esp+0x34 to esp+0x40
 * Default values: see stack trace
 *
 * Note: The rescaling will be done by an other hook, see Cv_addLeaderheadGFC.
 * Note: Finally, it proves as same like Cv_DipMidCenter_MP
 * Stack trace:
 0012FC5C esp
 0012FC90 dd       2CFh ; ¤
 0012FC94 dd       0D9h ; +
 0012FC98 dd       1E1h ; ß
 0012FC9C dd       1CEh ; +
 *
 */
void FUNC_NAKED Cv_DipMidCenter_SP(){
	__asm{
			IIADD(0x34, edx, eax, smc::BtS_EXE.diploWinSizes.midCenterSP.x)
			IIADD(0x38, edx, eax, smc::BtS_EXE.diploWinSizes.midCenterSP.y)
	}
	// Decoration handle
#if 0
	__asm{
		IFMUL(0x3C, smc::BtS_EXE.diploWinSizes.factorMain)
		IFMUL(0x40, smc::BtS_EXE.diploWinSizes.factorMain)
	}
#else
	COPY_ON_STACK(0x3c, eax, smc::BtS_EXE.diploWinSizes.midCenterSP.w);
	COPY_ON_STACK(0x40, eax, smc::BtS_EXE.diploWinSizes.midCenterSP.h);
#endif

	//smc::BtS_EXE.diploHooks.Replace_DipMidCenter_SP.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipMidCenter_SP.trampolin);
}

/*
 * The above hook scales the container of the leaderhead. This
 *	disturb the center position of the animation, which will be fixed
 *	by this offset change a few instructions later.
 *
 *	Lattest instrutions was:
.text:0055220B                 push    6
.text:0055220D                 push    0
.text:0055220F                 push    0
.text:00552211                 push    5
.text:00552213                 push    40h
 * We change the 0x40.
 */
void FUNC_NAKED Cv_DipLeaderhead_RestoreCenter(){
#if 0
	//(0x40,5) *= scale
	IFMUL(0x00, smc::BtS_EXE.diploWinSizes.factorLeaderhead)
	IFMUL(0x04, smc::BtS_EXE.diploWinSizes.factorLeaderhead)
#else
	COPY_ON_STACK(0x00, eax, smc::BtS_EXE.diploWinSizes.midLeaderheadOffset.x);
	COPY_ON_STACK(0x04, eax, smc::BtS_EXE.diploWinSizes.midLeaderheadOffset.y);
#endif

	//smc::BtS_EXE.diploHooks.Replace_DipLeaderhead_RestoreCenter.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipLeaderhead_RestoreCenter.trampolin);
}


/* DiploMenu: Bottom in MP variant
 * Free register: eax
 *
 * Data: x,y,w,h from esp+0x30 to esp+0x3c
 * Default values: 0x2CF, 0x2A7, 0x1E1, 0xE8
 */
void FUNC_NAKED Cv_DipMidBottom_MP(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x34, eax, ecx, smc::BtS_EXE.diploWinSizes.midBottom.x)
			IIADD(0x38, eax, ecx, smc::BtS_EXE.diploWinSizes.midBottom.y)
			pop ecx
	}
	COPY_ON_STACK(0x38, eax, smc::BtS_EXE.diploWinSizes.midBottom.w);
	COPY_ON_STACK(0x3c, eax, smc::BtS_EXE.diploWinSizes.midBottom.h);

	//smc::BtS_EXE.diploHooks.Replace_DipMidBottom_MP.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipMidBottom_MP.trampolin);
}

/* DiploMenu: Bottom in SP variant
 * Free register: eax, edi
 *
 * Data: x,y,w,h from esp+0x48 to esp+0x50
 * Default values: 0x2CF, 0x2A7, 0x1E1, 0xE8
 *
 * Note: Different offset from esp as mp variant!
 */
void FUNC_NAKED Cv_DipMidBottom_SP(){
	// Use pre-evaluated values...
	__asm{
		push ecx
			IIADD(0x48, eax, ecx, smc::BtS_EXE.diploWinSizes.midBottom.x)
			IIADD(0x4c, eax, ecx, smc::BtS_EXE.diploWinSizes.midBottom.y)
			pop ecx
	}
	COPY_ON_STACK(0x4c, eax, smc::BtS_EXE.diploWinSizes.midBottom.w);
	COPY_ON_STACK(0x50, eax, smc::BtS_EXE.diploWinSizes.midBottom.h);

	//smc::BtS_EXE.diploHooks.Replace_DipMidBottom_SP.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_DipMidBottom_SP.trampolin);
}

/* PythonApi:
 * VOID addLeaderheadGFC (
 * STRING szName, INT eWho, INT eInitAttitude, INT iX,
 * INT iY, INT iWidth, INT iHeight, WidgetType eWidget,
 * INT iData1, INT iData2)
 *
 * Call point: 0x0054F180
 * Critical offsets: 0x08 (height), ecx (width)
 *
 * Note: The changed positition is far behind the Cy-Wrapper because sometimes
 * the changed position will be called with values which are pushed directly
 * on the stack/or in register, i.e. '00553EBA mov     ecx, 161h'
 *
 * Todo: Control position (iX, iY)
 */
void FUNC_NAKED Cv_addLeaderheadGFC(){
#if 0
	// Use pre-evaluated values...
	COPY_ON_STACK(0x08, eax, smc::BtS_EXE.diploWinSizes.midLeaderheadSize.h);
	__asm{
		mov ecx, smc::BtS_EXE.diploWinSizes.midLeaderheadSize.w
	}
#else
	IFMUL_ROUNDUP(0x08, smc::BtS_EXE.diploWinSizes.factorLeaderhead);	// height
	IFMUL_PUSH(ecx, smc::BtS_EXE.diploWinSizes.factorLeaderhead); //width
#endif

	// Reset scaling factor to 1.0f (used in Pedia, e.g.)
	__asm{
		fld1
		fstp DWORD PTR smc::BtS_EXE.diploWinSizes.factorLeaderhead
	}

	//smc::BtS_EXE.diploHooks.Replace_addLeaderheadGFC.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_addLeaderheadGFC.trampolin);
}

// More general hooks (Called from PythonApi) */

/* PythonApi:
 * VOID setText (
 * STRING szName, STRING szAtttachTo, STRING szText, INT uiFlags,
 * FLOAT fX, FLOAT fY, FLOAT fZ, FontType eFont,
 * WidgetType eType, INT iData1, INT iData2)

Ida, CyGInterfaceScreen-Function with above header ( |XREF|=1 ):

.text:004E7030 ; ¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦ S U B R O U T I N E ¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦
.text:004E7030
.text:004E7030
.text:004E7030 sub_4E7030      proc near               ; DATA XREF: sub_718D50+316o
.text:004E7030
.text:004E7030 var_C           = dword ptr -0Ch
.text:004E7030 var_4           = dword ptr -4
.text:004E7030 arg_0           = dword ptr  4
.text:004E7030 arg_4           = dword ptr  8
.text:004E7030 arg_8           = dword ptr  0Ch
.text:004E7030 arg_24          = dword ptr  28h
.text:004E7030 arg_28          = dword ptr  2Ch
.text:004E7030 arg_2C          = dword ptr  30h
.text:004E7030 arg_30          = dword ptr  34h
.text:004E7030 arg_34          = dword ptr  38h
.text:004E7030 arg_38          = dword ptr  3Ch
.text:004E7030 arg_3C          = dword ptr  40h
.text:004E7030 arg_40          = dword ptr  44h

This routine push 11 arguments to the stack and then
calls this one. Is this CvGInterfaceScreen::setText(?) ?!

.text:00570E70 ; ¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦ S U B R O U T I N E ¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦
.text:00570E70
.text:00570E70
.text:00570E70 sub_570E70      proc near               ; CODE XREF: sub_41C120+389p
.text:00570E70                                         ; sub_41C120+428p ...
.text:00570E70
.text:00570E70 var_C           = dword ptr -0Ch
.text:00570E70 var_4           = dword ptr -4
.text:00570E70 arg_0           = dword ptr  4
.text:00570E70 arg_4           = dword ptr  8
.text:00570E70 arg_C           = dword ptr  10h
.text:00570E70 arg_28          = dword ptr  2Ch
.text:00570E70 arg_2C          = dword ptr  30h
.text:00570E70 arg_30          = dword ptr  34h
.text:00570E70 arg_3C          = dword ptr  40h
.text:00570E70 arg_40          = dword ptr  44h

 * Call point: 0x00570E70
 * Critical offsets: 30, 34
*/
void FUNC_NAKED Cv_setText(){
	FFMUL(0x30, smc::BtS_EXE.diploWinSizes.factorMain); // x
	FFMUL(0x34, smc::BtS_EXE.diploWinSizes.factorMain); // y

	//smc::BtS_EXE.diploHooks.Replace_setText.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_setText.trampolin);
}

/* PythonApi:
 * VOID setLabel (
 * STRING szName, STRING szAttachTo, STRING szText, INT uiFlags,
 * FLOAT fX, FLOAT fY, FLOAT fZ, FontType eFont,
 * WidgetType eType, INT iData1, INT iData2)
 *
 *
 * Call point: 0x00570F90 (Cv)
 * Critical offsets: 0x30, 34
 *
 * Note: Alternative for Cy_setLabel were
 * Call point: 0x004E7170
 * Critical offsets: 2C, 30
 */
void FUNC_NAKED Cv_setLabel(){
	FFMUL(0x30, smc::BtS_EXE.diploWinSizes.factorMain); // x
	FFMUL(0x34, smc::BtS_EXE.diploWinSizes.factorMain); // y

	//smc::BtS_EXE.diploHooks.Replace_setLabel.trampolin();
	TRAMPOLINE(smc::BtS_EXE.diploHooks.Replace_setLabel.trampolin);
}
/* ================= END HOOKS ================== */

//
// for logging
//
/*static const int kBufSize = 2048;
void logMsg(char* format, ... )
{
	static char buf[kBufSize];
	_vsnprintf( buf, kBufSize-4, format, (char*)(&format+1) );
	gDLL->logMsg("hooking.log", buf);
}*/

DiploHooks::DiploHooks():
	Replace_DipLeftTop((VoidFunc)0x0057F4E0, &Cv_DipLeftTop),
	Replace_DipLeftBottom((VoidFunc)0x0057F59B, &Cv_DipLeftBottom),
	Replace_DipRightTop((VoidFunc)0x0057F656, &Cv_DipRightTop),
	Replace_DipRightBottom((VoidFunc)0x0057F713, &Cv_DipRightBottom),
	Replace_DipMidHeadline_MP((VoidFunc)0x0058CE53, &Cv_DipMidHeadline_MP),
	Replace_DipMidHeadline_SP((VoidFunc)0x0055496C, &Cv_DipMidHeadline_SP),
	Replace_DipMidCenter_MP((VoidFunc)0x0058D71F, &Cv_DipMidCenter_MP),
	Replace_DipMidCenter_SP((VoidFunc)0x005521F2, &Cv_DipMidCenter_SP),
	Replace_DipLeaderhead_RestoreCenter((VoidFunc)0x00552215, &Cv_DipLeaderhead_RestoreCenter),
	Replace_DipMidBottom_MP((VoidFunc)0x0058D966, &Cv_DipMidBottom_MP),
	Replace_DipMidBottom_SP((VoidFunc)0x005541FC, &Cv_DipMidBottom_SP),
	Replace_addLeaderheadGFC((VoidFunc)0x0054F180, &Cv_addLeaderheadGFC),

	Replace_setText((VoidFunc)0x00570E70, &Cv_setText),
	Replace_setLabel((VoidFunc)0x00570F90, &Cv_setLabel) {
	};


DiploHooks::~DiploHooks() {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}


void DiploWinSizes::update(float factorMain, int extraWidthSides){
	FAssertMsg(factorMain >= 1.0f, "Scaling menu down not supported.");
	FAssertMsg(extraWidthSides >= 0, "Shrinking menu down not supported.");

	this->factorMain = factorMain;
	this->extraWidthSides = extraWidthSides;

	logMsg("update(%f, %d) called\n", this->factorMain, this->extraWidthSides);

	/* Note: The following (x,y) coordinates depinging on resolution
	 * during the debugging time and not the runtime.
	 * The given default values where evaluated for 1920x1080.
	 * => x,y will be used as offsets to get
	 *    resolution independend changes.
	 * */
	const Rect midpoint = {1920/2, 1080/2, 0, 0};
	const Rect default_leftTop = {0x1C7, 0x0AE, 0x102, 0x1F9};
	const Rect default_leftBottom = {0x1C7, 0x2A7, 0x102, 0x0E8};
	const Rect default_rightTop = {0x4B6, 0x0AE, 0x102, 0x1F9};
	const Rect default_rightBottom = {0x4B6, 0x2A7, 0x102, 0x0E8};

	const Rect default_midHeadline = {0x2CF, 0x0AE, 0x1E1, 0x2B};

	/*[    default midCenter-Rect     ]
	 * <----------- 0x1E1 ------------>
	 * [space] | [Leaderhead] | [space]
	 * <---64-->              <---64-->
	 */
	const Rect default_midCenterMP = {0x2CF, 0x0D9, 0x1E1, 0x1CE};
	const Position default_midLeaderheadOffset = {0x40, 5}; // Offset to midCenterMP.[xy]

	const Rect default_midBottom = {0x2CF, 0x2A7, 0x1E1, 0xE8};

	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_leftTop, leftTop);
	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_leftBottom, leftBottom);
	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_rightTop, rightTop);
	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_rightBottom, rightBottom);
	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_midHeadline, midHeadline);
	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_midBottom, midBottom);

	scaleAroundMidpoint(factorMain, factorMain, midpoint,
			default_midCenterMP, midCenter);

	midCenterSP.w = (int)(factorMain*(default_midCenterMP.w - 2*default_midLeaderheadOffset.x)) + 2*default_midLeaderheadOffset.x;
	midCenterSP.h = midCenter.h;
	midCenterSP.x = midCenter.x + (midCenter.w - midCenterSP.w)/2;
	midCenterSP.y =  midCenter.y;


	//Apply extra width change of left and right slide
	leftTop.x -= extraWidthSides;
	leftTop.w += extraWidthSides;
	leftBottom.x -= extraWidthSides;
	leftBottom.w += extraWidthSides;
	rightTop.w += extraWidthSides;
	rightBottom.w += extraWidthSides;
}

/* Map rect (x,y,w,h) from base resolution to
 *  scaled rect, but respect midpoint != origin.
 *
 */
void DiploWinSizes::scaleAroundMidpoint(
		const float s, const float t,
		const Rect &midpoint,
		const Rect &source,
		Rect &dest){
	dest.x = int(s * (source.x - midpoint.x) - ( source.x - midpoint.x));
	dest.y = int(t * (source.y - midpoint.y) - ( source.y - midpoint.y));
	dest.w = int(s * source.w);
	dest.h = int(t * source.h);
}


// PoC Allow loading of DLLs from Mod-Folder (Security hole)
bool update_path_env() {
#if 0
  // Get absolute path of subfolder
  DWORD len = GetCurrentDirectory(0, NULL); // Here, len does include final \0
  LPTSTR _directory = (LPTSTR) malloc(len * sizeof(TCHAR));
  len = GetCurrentDirectory(len, _directory); // Here, len does not include final \0
  if (len == 0 ){
    std::cout << "Can't fetch current directory" << std::endl;
    free(_directory);
    return false;
  }
  std::string directory(_directory);
  directory.append("\\BTS_Wrapper_Libs");
  free(_directory);

  // Get current value of PATH variable
  const std::size_t ENV_BUF_SIZE = 1024; // Enough for your PATH?
  char buf[ENV_BUF_SIZE];
  std::size_t bufsize = ENV_BUF_SIZE;
  int e = getenv_s(&bufsize,buf,bufsize,"PATH");  
  if (e) {
    std::cout << "`getenv_s` failed, returned " << e << '\n';
    return false;
  }

  // Update PATH variable
  std::string env_path = buf;
  //std::cout << "In main process, `PATH`=" << env_path << std::endl;
  env_path += ";";
  env_path += directory;
  e = _putenv_s("PATH",env_path.c_str());
  if (e) {
    std::cout << "`_putenv_s` failed, returned " << e << std::endl;
    return false;
  }
#else
  // Get current value of PATH variable
  const std::size_t ENV_BUF_SIZE = 1024; // Enough for your PATH?
  //char buf2[ENV_BUF_SIZE];
  std::size_t bufsize = ENV_BUF_SIZE;
	int e=0;
  //e = getenv_s(&bufsize,buf2,bufsize,"PATH");  
	const char *libvar = getenv( "LIB" );

  if (e) {
    //std::cout << "`getenv_s` failed, returned " << e << '\n';
    return false;
  }
#endif
  return true;
}


/*extern "C" BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      {
      }
      break;
    case DLL_PROCESS_DETACH:
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
  }
  return true;

}
*/
