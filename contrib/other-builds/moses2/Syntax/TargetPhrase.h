/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "../TargetPhrase.h"

namespace Moses2
{
namespace Syntax
{

class TargetPhrase : public Moses2::TargetPhrase
{
	  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);
public:

protected:
	  Word m_lhs;

};

}

}
