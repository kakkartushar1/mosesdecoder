/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

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
#ifndef _MIRA_OPTIMISER_H_
#define _MIRA_OPTIMISER_H_

#include <vector>

#include "ScoreComponentCollection.h"


namespace Mira {
  
  class Optimiser {
    public:
      Optimiser() {}
      virtual int updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector<std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector<std::vector<float> >& losses,
                         const std::vector<Moses::ScoreComponentCollection>& oracleScores) = 0;
  };
 
  class DummyOptimiser : public Optimiser {
    public:
      virtual int updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector< std::vector<float> >& losses,
                         const std::vector<Moses::ScoreComponentCollection>& oracleScores)
                         { return 0; }
  };
 
  class Perceptron : public Optimiser {
    public:
       

      virtual int updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector< std::vector<float> >& losses,
                         const std::vector<Moses::ScoreComponentCollection>& oracleScores);
  };

  class MiraOptimiser : public Optimiser {
   public:
	  MiraOptimiser() :
		  Optimiser() { }

	  MiraOptimiser(size_t n, bool hildreth, float marginScaleFactor, bool onlyViolatedConstraints, float clipping, bool fixedClipping, bool regulariseHildrethUpdates) :
		  Optimiser(),
		  m_n(n),
		  m_hildreth(hildreth),
		  m_marginScaleFactor(marginScaleFactor),
		  m_onlyViolatedConstraints(onlyViolatedConstraints),
		  m_c(clipping),
		  m_fixedClipping(fixedClipping),
		  m_regulariseHildrethUpdates(regulariseHildrethUpdates) { }

     ~MiraOptimiser() {}
   
      virtual int updateWeights(Moses::ScoreComponentCollection& weights,
      						  const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
      						  const std::vector< std::vector<float> >& losses,
      						  const std::vector< Moses::ScoreComponentCollection>& oracleScores);
      float computeDelta(Moses::ScoreComponentCollection& currWeights,
      				const Moses::ScoreComponentCollection featureValuesDiff,
      				float loss_jk,
      				float j,
      				float k,
      				std::vector< float>& alphas);
      void update(Moses::ScoreComponentCollection& currWeights, Moses::ScoreComponentCollection& featureValueDiffs, const float delta);

      void setOracleIndices(std::vector<size_t> oracleIndices) {
    	  m_oracleIndices= oracleIndices;
      }
  
   private:
      // number of hypotheses used for each nbest list (number of hope, fear, best model translations)
      size_t m_n;

      // whether or not to use the Hildreth algorithm in the optimisation step
      bool m_hildreth;

      // scaling the margin to regularise updates
      float m_marginScaleFactor;

      // add only violated constraints to the optimisation problem
      bool m_onlyViolatedConstraints;

      // clipping threshold for SMO to regularise updates
      float m_c;

      // use a fixed clipping threshold with SMO  (instead of adaptive)
      bool m_fixedClipping;

      // regularise Hildreth updates
      bool m_regulariseHildrethUpdates;

      // index of oracle translation in hypothesis matrix
      std::vector<size_t> m_oracleIndices;
  };
}

#endif
