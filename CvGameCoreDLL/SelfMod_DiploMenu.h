#pragma once
#ifndef SELF_MOD_DIPLO_MENU_H
#define SELF_MOD_DIPLO_MENU_H

typedef void (*VoidFunc)(); // argument-less stub for all hooks.
struct T_REPLACE_POINTER{
    T_REPLACE_POINTER(VoidFunc target, VoidFunc hook)
        : target(target), hook(hook), trampolin(NULL) {}

    VoidFunc target;
    VoidFunc hook;
    VoidFunc trampolin;
};

struct Position {
    int x,y;
};
struct Size {
    int w,h;
};
struct Rect {
    int x,y,w,h;
};

class DiploHooks {
    public:
        DiploHooks();
        ~DiploHooks();

        T_REPLACE_POINTER Replace_DipLeftTop;
        T_REPLACE_POINTER Replace_DipLeftBottom;
        T_REPLACE_POINTER Replace_DipRightTop;
        T_REPLACE_POINTER Replace_DipRightBottom;
        T_REPLACE_POINTER Replace_DipMidHeadline_MP;
        T_REPLACE_POINTER Replace_DipMidHeadline_SP;
        T_REPLACE_POINTER Replace_DipMidCenter_MP;
        T_REPLACE_POINTER Replace_DipMidCenter_SP;
        T_REPLACE_POINTER Replace_DipLeaderhead_RestoreCenter;
        T_REPLACE_POINTER Replace_DipMidBottom_MP;
        T_REPLACE_POINTER Replace_DipMidBottom_SP;
        T_REPLACE_POINTER Replace_addLeaderheadGFC;

        T_REPLACE_POINTER Replace_setText;
        T_REPLACE_POINTER Replace_setLabel;
};

class DiploWinSizes
{
    public:
        DiploWinSizes(float factorMain = 1.0f, int extraWidthSides = 0):
            factorMain(factorMain),
            factorLeaderhead(1.0f),
            extraWidthSides(extraWidthSides) {
                // update(factorMain, extraWidthSides);
                //     crashs if update() is using gDLL->... calls.
                //     Delay first update before setup of hooking functions.
            }
        void update(float factorMain, int extraWidthSides);

        float factorMain;
        float factorLeaderhead;
        int extraWidthSides;
        Rect leftTop;
        Rect leftBottom;
        Rect rightTop;
        Rect rightBottom;
        Rect midHeadline;
        Rect midBottom;
        Rect midCenter;
        Rect midCenterSP;
        Position midLeaderheadOffset; // offset in midCenter (0x40,5)
        Size midLeaderheadSize;
        Rect leaderhead;

    private:
        void scaleAroundMidpoint(
                const float s, const float t,
                const Rect &midpoint,
                const Rect &source,
                Rect &dest);
};


#endif

#ifndef __ASM_TEMPLATES_H__
#define __ASM_TEMPLATES_H__

/* Templates to modify of floats in releation to esp register.
 * Note: No evaluation, like 'OFFSET+4' in arguments allowed.
 * Note: Do not define locale variables in hooked function unless you also adapt the offset.
 * Note: __asm on each line is required because macro shrinks to single line.
 * Note: Do not add semicolon. Without ; the snippets can be nested.
 * */

#define C(comment) // Well, commenting in macros sucks...

// Strip overhead which disturbes ESP register, etc.
#define FUNC_NAKED __declspec(naked)

#ifndef FUNC_NAKED
// Old approach which handles function overhead manually. (Just as reference)
/* With debug information, each function begins with the push of two
 * variables on the stack:
 * push        ebp  
 * mov         ebp,esp  
 * push        esi
 *
 * We need to respect this offset during the manipulation of stack variables.
 */
#ifdef _DEBUG
#define DEBUG_OFFSET 8
#define DEBUG_UNDO \
  __asm{ \
    __asm pop esi \
    __asm pop ebp \
  }
#else
#ifdef WITH_FRAME_POINTER // /Oy- compiler flag?!
#define DEBUG_OFFSET 4
#define DEBUG_UNDO \
  __asm{ \
    __asm pop ebp \
  }
#else
#define DEBUG_OFFSET 0
#define DEBUG_UNDO 
#endif
#endif
#else
#define DEBUG_OFFSET 0
#define DEBUG_UNDO 
#endif

/* Multiply float by float. */
#define FFMUL(OFFSET, FACTOR) \
    __asm{ \
        __asm 		fld [FACTOR] \
        __asm 		fmul DWORD PTR [esp+(OFFSET+DEBUG_OFFSET)] \
        __asm		fstp DWORD PTR [esp+(OFFSET+DEBUG_OFFSET)] \
    }

/* Multiply int with float scaling factor. */
#define IFMUL(OFFSET, FACTOR) \
    __asm{ \
        __asm 			fld [FACTOR] \
        __asm 		fimul DWORD PTR [esp+(OFFSET+DEBUG_OFFSET)] \
        __asm 			fistp DWORD PTR [esp+(OFFSET+DEBUG_OFFSET)] \
    }

/* Multiplication of int with float and rounding up towards next integer.
 *
 * Assembler x86 rounding control:
   ; ROUNDING CONTROL FIELD
   ;     The rounding control (RC) field of the FPU
   ;     control register (bits 10 and 11)
   ;
   ;     Rounding Mode                  RC Field Setting
   ;
   ;     Round nearest     (even)       00b
   ;     Round down        (toward -)   01b
   ;     Round up          (toward +)   10b
   ;     Round toward zero (Truncate)   11b

   or    ax,0C00h  ;this will set both bits of the RC field to the truncating mode
                   ; => RC=11
   push  eax       ;use the stack to store the modified Control Word in memory \
   fldcw [esp]     ;load the modified Control Word \

   Attention: Backup+Restoring of previous control word (fstcw) is needed!
*/
#define IFMUL_ROUNDUP(OFFSET, FACTOR) \
    __asm push  eax         C("Backup eax and set esp on free slot") \
    __asm fstcw [esp]       C("Backup current control word in [esp]") \
    __asm mov   eax, [esp]  C("Copy control word") \
    __asm or    ah, 0Ch     C("Truncate mode, 11b") \
    __asm xor   ah, 04h     C("Round toward + mode, 10b") \
    __asm push  eax         C("Push exa on stack for fldcw") \
    __asm fldcw [esp]       C("Load new control word") \
    IFMUL(OFFSET+DEBUG_OFFSET+8, FACTOR) C("+8 because stack is grown by 2 words") \
    __asm pop eax           C("Discard changed control word") \
    __asm fldcw [esp]       C("Load previous control word") \
    __asm pop eax           C("Discard previous control word") \


/* Push integer on stack and multiply */
#define IFMUL_PUSH(REGISTER, FACTOR) \
    __asm{ \
        __asm push REGISTER \
        IFMUL(0, FACTOR) \
        __asm pop REGISTER \
    }

/* Add float value. */
#define FFADD(OFFSET, SHIFTVAR) \
    __asm{ \
        __asm 			fld [SHIFTVAR] \
        __asm 			fadd DWORD PTR [esp+(OFFSET+DEBUG_OFFSET)] \
        __asm 			fstp DWORD PTR [esp+(OFFSET+DEBUG_OFFSET)] \
    }

/* Add integer to integer value. */
#define IIADD(OFFSET, REG1, REG2, VALUE) \
    __asm{ \
        __asm 			mov REG1, [esp+(OFFSET+DEBUG_OFFSET)] \
        __asm 			mov REG2, [VALUE] \
        __asm 			add REG1, REG2 \
        __asm 			mov [esp+(OFFSET+DEBUG_OFFSET)], REG1 \
    }

#define IIADD8(OFFSET, VALUE) \
    __asm{ \
        __asm 			push eax \
        __asm 			push ecx \
        __asm 			mov eax, [esp+(OFFSET+DEBUG_OFFSET)] \
        __asm 			mov ecx, [VALUE] \
        __asm 			add eax, ecx \
        __asm 			mov [esp+(OFFSET+DEBUG_OFFSET)], eax \
        __asm 			pop ecx \
        __asm 			pop eax \
    }

/* Perform rescaling of (x,w) tupel a.k.a. reposition of left border.
 *
 * iX, iW, fS given. Evaluate
 * w2 := fS * iW
 * x2 := x + ( iX - w2 )
 *
 * Require two registers.
 */
#define IIF_SCALE_LEFT(OFFSET_X, OFFSET_W, SCALEFACTOR, REG1, REG2) \
    __asm{ \
        __asm		mov REG1, [esp+(OFFSET_W+DEBUG_OFFSET)] \
        __asm		fld [SCALEFACTOR] \
        __asm		fmul DWORD PTR [esp+(OFFSET_W+DEBUG_OFFSET)] \
        __asm		fstp DWORD PTR [esp+(OFFSET_W+DEBUG_OFFSET)] \
        __asm		mov REG2, [esp+(OFFSET_W+DEBUG_OFFSET)] \
        __asm		sub REG1, REG2 \
        __asm		mov REG2, [esp+(OFFSET_X+DEBUG_OFFSET)] \
        __asm		sub REG2, REG1 \
        __asm		mov [esp+(OFFSET_X+DEBUG_OFFSET)], REG2 \
    }

#define COPY_ON_STACK(OFFSET, REG, VALUE) \
    __asm{ \
        __asm mov REG, VALUE \
        __asm mov [esp+(OFFSET+DEBUG_OFFSET)], REG \
    }

#define COPY(TO, REG, FROM) \
    __asm{ \
        __asm mov REG, FROM \
        __asm mov TO, REG \
    }

#endif

// Continue with original function
/* In Debug-Builds, compiler adds following check at the end:
   CvGameCoreDLL.Cv_DipLeftTop+53- 5E                    - pop esi
   CvGameCoreDLL.Cv_DipLeftTop+54- 3B EC                 - cmp ebp,esp
   CvGameCoreDLL.Cv_DipLeftTop+56- E8 11610000           - call CvGameCoreDLL._RTC_CheckEsp
   CvGameCoreDLL.Cv_DipLeftTop+5B- 5D                    - pop ebp
   CvGameCoreDLL.Cv_DipLeftTop+5C- C3                    - ret 

   Note that this codes IS NEVER REACHED reached due 'jmp' in trampolin.
   Thus we had to cleanup manualay before trampolin pointer is used.
   */
#define TRAMPOLINE(TARGET)\
DEBUG_UNDO \
/* Using assembler here to avoid 'call' instruction which would change the ESP register. */ \
__asm{ \
    jmp dword ptr [TARGET] \
}

