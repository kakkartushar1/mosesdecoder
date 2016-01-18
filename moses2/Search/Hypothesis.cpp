/*
 * Hypothesis.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include <stdlib.h>
#include <deque>
#include "Hypothesis.h"
#include "Manager.h"
#include "../InputPath.h"
#include "../System.h"
#include "../Scores.h"
#include "../Sentence.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses2
{

//size_t g_numHypos = 0;

Hypothesis *Hypothesis::Create(MemPool &pool, Manager &mgr)
{
//	++g_numHypos;
	Hypothesis *ret;

	Recycler<Hypothesis*> &recycler = mgr.GetHypoRecycle();
	ret = recycler.Get();
	if (ret) {
		// got new hypo from recycler. Do nothing
	}
	else {
		ret = new (pool.Allocate<Hypothesis>()) Hypothesis(pool, mgr.system);
		recycler.Keep(ret);
	}
	return ret;
}

void Hypothesis::Prefetch() const
{
  //__builtin_prefetch(hypo);
  __builtin_prefetch(m_ffStates);
  __builtin_prefetch(m_ffStates[0]);
  __builtin_prefetch(m_ffStates[1]);
  __builtin_prefetch(m_ffStates[2]);
  __builtin_prefetch(m_scores);
  __builtin_prefetch(&m_estimatedScore);
  __builtin_prefetch(&m_currTargetWordsRange);
}

Hypothesis::Hypothesis(MemPool &pool, const System &system)
:m_currTargetWordsRange()
{
	m_scores = new (pool.Allocate<Scores>()) Scores(system, pool, system.featureFunctions.GetNumScores());

	// FF states
	const std::vector<const StatefulFeatureFunction*> &sfffs = system.featureFunctions.GetStatefulFeatureFunctions();
	size_t numStatefulFFs = sfffs.size();
	m_ffStates = (FFState **) pool.Allocate(sizeof(FFState*) * numStatefulFFs);

    BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
    	size_t statefulInd = sfff->GetStatefulInd();
    	FFState *state = sfff->BlankState(pool);
    	m_ffStates[statefulInd] = state;
    }
}

Hypothesis::~Hypothesis() {
	// TODO Auto-generated destructor stub
}

void Hypothesis::Init(Manager &mgr, const InputPath &path, const TargetPhrase &tp, const Bitmap &bitmap)
{
	m_mgr = &mgr;
	m_targetPhrase = &tp;
	m_sourceCompleted = &bitmap;
	m_path = &path;
	m_prevHypo = NULL;

	m_currTargetWordsRange.SetStartPos(NOT_FOUND);
	m_currTargetWordsRange.SetEndPos(NOT_FOUND);

	m_estimatedScore = 0;
	m_scores->Reset(mgr.system);
}

void Hypothesis::Init(Manager &mgr, const Hypothesis &prevHypo,
		const InputPath &path,
		const TargetPhrase &tp,
		const Bitmap &bitmap,
		SCORE estimatedScore)
{
	m_mgr = &mgr;
	m_targetPhrase = &tp;
	m_sourceCompleted = &bitmap;
	m_path = &path;
	m_prevHypo = &prevHypo;

	m_currTargetWordsRange.SetStartPos(prevHypo.m_currTargetWordsRange.GetEndPos() + 1);
	m_currTargetWordsRange.SetEndPos(prevHypo.m_currTargetWordsRange.GetEndPos() + tp.GetSize());

	m_estimatedScore = estimatedScore;

	m_scores->Reset(mgr.system);
	m_scores->PlusEquals(mgr.system, prevHypo.GetScores());
	m_scores->PlusEquals(mgr.system, GetTargetPhrase().GetScores());
}

size_t Hypothesis::hash() const
{
  size_t numStatefulFFs = GetManager().system.featureFunctions.GetStatefulFeatureFunctions().size();
  size_t seed;

  // coverage
  seed = (size_t) m_sourceCompleted;

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
	const FFState *state = m_ffStates[i];
	size_t hash = state->hash();
	boost::hash_combine(seed, hash);
  }
  return seed;

}

bool Hypothesis::operator==(const Hypothesis &other) const
{
	size_t numStatefulFFs = GetManager().system.featureFunctions.GetStatefulFeatureFunctions().size();
  // coverage
  if (m_sourceCompleted != other.m_sourceCompleted) {
	return false;
 }

  // states
  for (size_t i = 0; i < numStatefulFFs; ++i) {
	const FFState &thisState = *m_ffStates[i];
	const FFState &otherState = *other.m_ffStates[i];
	if (thisState != otherState) {
	  return false;
	}
  }
  return true;

}

void Hypothesis::OutputToStream(std::ostream &out) const
{
  if (m_prevHypo) {
	  m_prevHypo->OutputToStream(out);
  }

  if (GetTargetPhrase().GetSize()) {
	  const Phrase &phrase = GetTargetPhrase();
	  out << phrase << " ";
  }

  if (m_path->range.GetStartPos() != NOT_FOUND) {
	  if (m_mgr->system.reportSegmentation == 1) {
		  // just report phrase segmentation
		  out << "|"  << m_path->range.GetStartPos() << "-" << m_path->range.GetEndPos() << "| ";
	  }
	  else if (m_mgr->system.reportSegmentation == 2) {
		  // more detailed info about every segment
		  out << "|";

		  // phrase segmentation
		  out << m_path->range.GetStartPos() << "-" << m_path->range.GetEndPos() << ",";

		  // score breakdown
		  m_scores->Debug(out, m_mgr->system);

		  out << "| ";
	  }
  }
}

std::ostream& operator<<(std::ostream &out, const Hypothesis &obj)
{
	// coverage
	out << obj.GetBitmap() << " " << obj.GetInputPath().range << " ";

	// states
	const std::vector<const StatefulFeatureFunction*> &sfffs = obj.GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
	size_t numStatefulFFs = sfffs.size();
	for (size_t i = 0; i < numStatefulFFs; ++i) {
		const FFState &state = *obj.GetState(i);
		out << "(" << state << ") ";
	}

	// string
	obj.OutputToStream(out);
	out << " ";
	out << "fc=" << obj.GetFutureScore() << " ";
	obj.GetScores().Debug(out, obj.GetManager().system);
	return out;
}

void Hypothesis::EmptyHypothesisState(const InputType &input)
{
	const std::vector<const StatefulFeatureFunction*>  &sfffs = GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
	  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
		  size_t statefulInd = sfff->GetStatefulInd();
		  FFState *state = m_ffStates[statefulInd];
		  sfff->EmptyHypothesisState(*state, GetManager(), input, *this);
	  }
}

void Hypothesis::EvaluateWhenApplied()
{
  const std::vector<const StatefulFeatureFunction*>  &sfffs = GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  EvaluateWhenApplied(*sfff);
  }
  //cerr << *this << endl;
}

void Hypothesis::EvaluateWhenApplied(const StatefulFeatureFunction &sfff)
{
	  size_t statefulInd = sfff.GetStatefulInd();
	  const FFState *prevState = m_prevHypo->GetState(statefulInd);
	  FFState *thisState = m_ffStates[statefulInd];
	  assert(prevState);
	  sfff.EvaluateWhenApplied(GetManager(), *this, *prevState, *m_scores, *thisState);

}

void Hypothesis::EvaluateWhenAppliedNonBatch()
{
  const std::vector<const StatefulFeatureFunction*>  &sfffs = GetManager().system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  size_t statefulInd = sfff->GetStatefulInd();
	  const FFState *prevState = m_prevHypo->GetState(statefulInd);
	  FFState *thisState = m_ffStates[statefulInd];
	  assert(prevState);
	  sfff->EvaluateWhenAppliedNonBatch(GetManager(), *this, *prevState, *m_scores, *thisState);
  }
  //cerr << *this << endl;
}

/** recursive - pos is relative from start of sentence */
const Word &Hypothesis::GetWord(size_t pos) const {
  const Hypothesis *hypo = this;
  while (pos < hypo->GetCurrTargetWordsRange().GetStartPos()) {
    hypo = hypo->GetPrevHypo();
    UTIL_THROW_IF2(hypo == NULL, "Previous hypothesis should not be NULL");
  }
  return hypo->GetCurrWord(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
}

void Hypothesis::Swap(Hypothesis &other)
{
	/*
	Swap(m_targetPhrase, other.m_targetPhrase);
	Swap(m_sourceCompleted, other.m_sourceCompleted);
	Swap(m_range, other.m_range);
	Swap(m_prevHypo, other.m_prevHypo);
	Swap(m_ffStates, other.m_ffStates);
	Swap(m_estimatedScore, other.m_estimatedScore);
	Swap(m_currTargetWordsRange, other.m_currTargetWordsRange);
	*/
}

}
