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

#include <vector>
#include <string>
#include "Word.h"
#include "Phrase.h"

class Sentence : public Phrase
{
protected:
	long m_translationId;

	Sentence()
	{
	}
public:
	Sentence(Language language)
		:Phrase(language)
	{
	}

	// for db stuff
	long GetTranslationId()
	{
		return m_translationId;;
	}
	void SetTranslationId(long translationId)
	{	// for db stuff;
		m_translationId = translationId;
	}
};

