// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once

#include <sstream>
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <string>
#include "TypeDef.h"
#include "Util.h"

class FactorCollection;

class Factor
{
	friend std::ostream& operator<<(std::ostream&, const Factor&);

	// only these classes are allowed to instantiate this class
	friend class FactorCollection;

protected:
	Language		m_language;
	FactorType	m_factorType;
	const std::string	*m_ptrString;
	LmId 				m_lmId;

	Factor(Language language, FactorType factorType, const std::string *factorString, LmId lmId);
	
	inline void SetLmId(LmId lmId)
	{
		m_lmId = lmId;
	}

public:
	inline Language GetLanguage() const
	{
		return m_language;
	}
	inline FactorType GetFactorType() const
	{
		return m_factorType;
	}
	inline const std::string &GetString() const
	{
		return *m_ptrString;
	}
	inline LmId GetLmId() const
	{
		return m_lmId;
	}
	
	// do it properly. needed for insert & finding of words in dictionary
	inline int Compare(const Factor &compare) const
	{
		if (m_ptrString < compare.m_ptrString)
			return -1;
		if (m_ptrString > compare.m_ptrString)
			return 1;

		if (m_language < compare.m_language)
			return -1;
		if (m_language > compare.m_language)
			return 1;

		if (m_factorType < compare.m_factorType)
			return -1;
		if (m_factorType > compare.m_factorType)
			return 1;

		return 0;
	}
	
	inline bool operator<(const Factor &compare) const
	{ 
		return Compare(compare) < 0;
	}
	
	TO_STRING

};

