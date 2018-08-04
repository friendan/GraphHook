#include "TH10Hook/Common.h"
#include "TH10Hook/MinHookFunc.h"

#include <MinHook.h>

namespace th
{
	MinHookFunc::MinHookFunc() :
		m_target(nullptr)
	{
	}

	MinHookFunc::MinHookFunc(LPVOID target, LPVOID detour, LPVOID* original) :
		m_target(target)
	{
		MH_STATUS status = MH_CreateHook(target, detour, original);
		if (status != MH_OK)
			BOOST_THROW_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
	}

	MinHookFunc::MinHookFunc(MinHookFunc&& other) :
		m_target(other.m_target)
	{
		other.m_target = nullptr;
	}

	MinHookFunc::~MinHookFunc()
	{
		if (m_target != nullptr)
			MH_RemoveHook(m_target);
	}

	MinHookFunc& MinHookFunc::operator =(MinHookFunc&& other)
	{
		MinHookFunc(std::move(other)).swap(*this);
		return *this;
	}

	void MinHookFunc::swap(MinHookFunc& other)
	{
		std::swap(m_target, other.m_target);
	}

	void MinHookFunc::hook()
	{
		if (m_target == nullptr)
			BOOST_THROW_EXCEPTION(Exception() << err_str("m_target == nullptr"));

		MH_STATUS status = MH_EnableHook(m_target);
		if (status != MH_OK)
			BOOST_THROW_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
	}

	void MinHookFunc::unhook()
	{
		if (m_target == nullptr)
			BOOST_THROW_EXCEPTION(Exception() << err_str("m_target == nullptr"));

		MH_STATUS status = MH_DisableHook(m_target);
		if (status != MH_OK)
			BOOST_THROW_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));

	}
}
