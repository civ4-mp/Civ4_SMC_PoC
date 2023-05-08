#pragma once
#ifndef SELF_MOD_H
#define SELF_MOD_H

/*	smcpoc: Let's put all runtime patches for Civ4BeyondSword.exe (v3.19)
	in a single place. (But, so far, there's only one.) */

#include "SelfMod_DiploMenu.h"

class Civ4BeyondSwordMods
{
public:
	// (Unused for now)
	bool isPlotIndicatorSizePatched() const { return m_bPlotIndicatorSizePatched; }
	void patchPlotIndicatorSize();
private:
	bool m_bPlotIndicatorSizePatched;

#ifdef SELF_MOD_DIPLO_MENU_H
public:
	bool isDiploWinPatched() const { return m_bDiploWinSizePatched; }
	void patchDiploWin();
	DiploHooks diploHooks; // member variables accessed by hook functions
private:
	void hookDiploWin();
	bool m_bDiploWinSizePatched;
	DiploWinSizes diploWinSizes;
#endif
};


namespace smc
{
	extern Civ4BeyondSwordMods BtS_EXE;
};

#endif
