#include "breakpoint_handler.h"

extern void ppu_breakpoint(u32 loc, bool isAdding);

breakpoint_handler::breakpoint_handler() :m_breakpoints_list()
{
	break_on_bpm = false;
}

breakpoint_handler::~breakpoint_handler()
{

}

bool breakpoint_handler::IsBreakOnBPM()
{
	return break_on_bpm;
}

bool breakpoint_handler::HasBreakpoint(u32 loc, bs_t<breakpoint_type> type)
{
	return (m_breakpoints_list.find(loc) != m_breakpoints_list.end()) && ((m_breakpoints_list[loc] & type) == type);
}

bool breakpoint_handler::AddBreakpoint(u32 loc, bs_t<breakpoint_type> type)
{
	m_breakpoints_list[loc] = type;

	if (type & breakpoint_type::bp_execute)
	{
		ppu_breakpoint(loc, true);
	}
	return true;
}

bool breakpoint_handler::RemoveBreakpoint(u32 loc, bs_t<breakpoint_type> type)
{
	if ( (m_breakpoints_list.find(loc) == m_breakpoints_list.end()) )// || ((m_breakpoints_list[loc] & type) != type))
	{
		return false;
	}

	m_breakpoints_list.erase(loc);

	if (type & breakpoint_type::bp_execute)
	{
		ppu_breakpoint(loc, false);
	}

	return true;
}
