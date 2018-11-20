#pragma once
#include "stdafx.h"
#include <set>

enum class breakpoint_type
{
	bp_execute,
	bp_mread,
	bp_mwrite,

	__bitset_enum_max
};

/*
* This class acts as a layer between the UI and Emu for breakpoints.
*/
class breakpoint_handler
{

public:
	breakpoint_handler();
	~breakpoint_handler();

	bool IsBreakOnBPM();

	/**
	* Returns true iff breakpoint exists at loc.
	* TODO: Add arg for flags, gameid, and maybe even thread if it should be thread local breakpoint.... breakpoint struct is probably what'll happen
	*/
	bool HasBreakpoint(u32 loc, bs_t<breakpoint_type> type);

	/**
	* Returns true if added successfully. TODO: flags
	*/
	bool AddBreakpoint(u32 loc, bs_t<breakpoint_type> type);


	/**
	* Returns true if removed breakpoint at loc successfully.
	*/
	bool RemoveBreakpoint(u32 loc, bs_t<breakpoint_type> type);

private:
	// TODO : generalize to hold multiple games and handle flags.Probably do : std::map<std::string (gameid), std::set<breakpoint>>.
	// Although, externally, they'll only be accessed by loc (I think) so a map of maps may also do? 
	std::map<u32, bs_t<breakpoint_type>> m_breakpoints_list;
	bool break_on_bpm;
};

extern breakpoint_handler *g_breakpoint_handler;
