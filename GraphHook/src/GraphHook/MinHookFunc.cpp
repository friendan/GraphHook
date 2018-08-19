#include "GraphHook/Common.h"
#include "GraphHook/MinHookFunc.h"

#include <MinHook.h>

namespace gh
{
	MinHookFunc::MinHookFunc() :
		m_target(nullptr)
	{
	}

	MinHookFunc::MinHookFunc(LPVOID target, LPVOID detour, LPVOID* original) :
		m_target(target)
	{
		MH_STATUS status;

		status = MH_CreateHook(target, detour, original);
		if (status != MH_OK)
			THROW_CPP_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));

		status = MH_EnableHook(target);
		if (status != MH_OK)
		{
			MH_RemoveHook(target);
			THROW_CPP_EXCEPTION(Exception() << err_str(MH_StatusToString(status)));
		}
	}

	MinHookFunc::MinHookFunc(MinHookFunc&& other) :
		m_target(other.m_target)
	{
		other.m_target = nullptr;
	}

	MinHookFunc::~MinHookFunc()
	{
		if (m_target != nullptr)
		{
			MH_DisableHook(m_target);
			MH_RemoveHook(m_target);
		}
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
}
