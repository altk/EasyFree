// algparam.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_IMPORTS

#include "algparam.h"

NAMESPACE_BEGIN(CryptoPP)
	PAssignIntToInteger g_pAssignIntToInteger = nullptr;

	bool CombinedNameValuePairs::GetVoidValue(const char* name, const type_info& valueType, void* pValue) const
	{
		if (strcmp(name, "ValueNames") == 0)
		{
			return m_pairs1.GetVoidValue(name, valueType, pValue) && m_pairs2.GetVoidValue(name, valueType, pValue);
		}
		return m_pairs1.GetVoidValue(name, valueType, pValue) || m_pairs2.GetVoidValue(name, valueType, pValue);
	}

	void AlgorithmParametersBase::operator=(const AlgorithmParametersBase& rhs)
	{
		assert(false);
	}

	bool AlgorithmParametersBase::GetVoidValue(const char* name, const type_info& valueType, void* pValue) const
	{
		if (strcmp(name, "ValueNames") == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(name, typeid(std::string), valueType);
			if (m_next.get()) m_next->GetVoidValue(name, valueType, pValue);
			(*reinterpret_cast<std::string *>(pValue) += m_name) += ";";
			return true;
		}
		if (strcmp(name, m_name) == 0)
		{
			AssignValue(name, valueType, pValue);
			m_used = true;
			return true;
		}
		if (m_next.get())
		{
			return m_next->GetVoidValue(name, valueType, pValue);
		}
		return false;
	}

	AlgorithmParameters::AlgorithmParameters()
		: m_defaultThrowIfNotUsed(true) {}

	AlgorithmParameters::AlgorithmParameters(const AlgorithmParameters& x)
		: m_defaultThrowIfNotUsed(x.m_defaultThrowIfNotUsed)
	{
		m_next.reset(const_cast<AlgorithmParameters &>(x).m_next.release());
	}

	AlgorithmParameters& AlgorithmParameters::operator=(const AlgorithmParameters& x)
	{
		m_next.reset(const_cast<AlgorithmParameters &>(x).m_next.release());
		return *this;
	}

	bool AlgorithmParameters::GetVoidValue(const char* name, const type_info& valueType, void* pValue) const
	{
		if (m_next.get())
		{
			return m_next->GetVoidValue(name, valueType, pValue);
		}
		return false;
	}

	NAMESPACE_END

#endif