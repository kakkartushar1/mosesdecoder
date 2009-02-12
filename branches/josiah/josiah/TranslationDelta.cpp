/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include "TranslationDelta.h"

using namespace std;

namespace Moses {
  
  
  std::map<LanguageModel*,LanguageModelCache> TranslationDelta::m_cache;
  
  float LanguageModelCache::GetValue(const std::vector<const Word*>& ngram) {
    EntryListIterator* entryListIter = m_listPointers[ngram];
    float score;
    if (!entryListIter) {
      //cache miss
      if ((int)m_listPointers.size() >= m_maxSize) {
        //too many entries
        Entry lruEntry = m_entries.back();
        m_entries.pop_back();
        delete m_listPointers[lruEntry.first];
        m_listPointers.erase(lruEntry.first);
      }
      score = m_languageModel->GetValue(ngram);
      m_entries.push_front(Entry(ngram,score));
      entryListIter = new EntryListIterator();
      *entryListIter = m_entries.begin();
      m_listPointers[ngram] = entryListIter;
    } else {
      //cache hit
      Entry entry  = *(*entryListIter);
      m_entries.erase(*entryListIter);
      m_entries.push_front(entry);
      *entryListIter = m_entries.begin();
      score = entry.second;
    } 
    return score;
  }

/**
  Compute the change in language model score by adding this target phrase
  into the hypothesis at the given target position.
  **/
void  TranslationDelta::addLanguageModelScore(const vector<Word>& targetWords, const Phrase& targetPhrase, const WordsRange& targetSegment) {
  const LMList& languageModels = StaticData::Instance().GetAllLM();
  for (LMList::const_iterator i = languageModels.begin(); i != languageModels.end(); ++i) {
    LanguageModel* lm = *i;
    /*
    map<LanguageModel*,LanguageModelCache>::iterator lmci = m_cache.find(lm);
    if (lmci == m_cache.end()) {
      m_cache.insert(pair<LanguageModel*,LanguageModelCache>(lm,LanguageModelCache(lm)));
    }*/
    size_t order = lm->GetNGramOrder();
    vector<const Word*> lmcontext(targetPhrase.GetSize() + 2*(order-1));
    
    int start = targetSegment.GetStartPos() - (order-1);
    
    //fill in the pre-context
    size_t contextPos = 0;

    for (size_t i = 0; i < order-1; ++i) {
      if (start+(int)i < 0) {
        lmcontext[contextPos++] = &(lm->GetSentenceStartArray());
      } else {
        lmcontext[contextPos++] = &(targetWords[i+start]);
      }
    }
    
    //fill in the target phrase
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      lmcontext[contextPos++] = &(targetPhrase.GetWord(i));
    }
    
    //fill in the postcontext
    size_t eoscount = 0;
    for (size_t i = 0; i < order-1; ++i) {
      size_t targetPos = i + targetSegment.GetEndPos() + 1;
      if (targetPos >= targetWords.size()) {
        lmcontext[contextPos++] = &(lm->GetSentenceEndArray());
        ++eoscount;
      } else {
        lmcontext[contextPos++] = &(targetWords[targetPos]);
      }
    }
    
    //debug
    IFVERBOSE(3) {
      VERBOSE(3,"Segment: " << targetSegment << " phrase: " << targetPhrase << endl);
      VERBOSE(3,"LM context ");
      for (size_t j = 0;  j < lmcontext.size(); ++j) {
        if (j == order-1) {
          VERBOSE(3, "[ ");
        }
        if (j == (targetPhrase.GetSize() + (order-1))) {
          VERBOSE(3,"] ");
        }
        VERBOSE(3,*(lmcontext[j]) << " ");
        
      }
      VERBOSE(3,endl);
    }
    
    //score lm
    double lmscore = 0;
    vector<const Word*> ngram;
    //remember to only include max of 1 eos marker
    size_t maxend = min(lmcontext.size(), lmcontext.size() - (eoscount-1));
    for (size_t ngramstart = 0; ngramstart < lmcontext.size() - (order -1); ++ngramstart) {
      ngram.clear();
      for (size_t j = ngramstart; j < ngramstart+order && j < maxend; ++j) {
        ngram.push_back(lmcontext[j]);
      }
      lmscore += lm->GetValue(ngram);
      //cache disabled for now
      //lmscore += m_cache.find(lm)->second.GetValue(ngram);
      
    }
    VERBOSE(2,"Language model score: " << lmscore << endl); 
    m_scores.Assign(lm,lmscore);
  }
}

void TranslationDelta::initScoresSingleUpdate(const vector<Word>& targetWords, const TranslationOption* option, const WordsRange& targetSegment) {
  //translation scores
  m_scores.PlusEquals(option->GetScoreBreakdown());
        
  //don't worry about reordering because they don't change
        
  //word penalty
  float penalty = -((int)option->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
        
        
  addLanguageModelScore(targetWords, option->GetTargetPhrase(), targetSegment);
        
  //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);
  
  VERBOSE(2, "Single Update: Scores " << m_scores << endl);
  VERBOSE(2,"Single Update: Total score is  " << m_score << endl);  
}

void TranslationDelta::initScoresPairedUpdate(const vector<Word>& targetWords, const TranslationOption* leftOption,
      const TranslationOption* rightOption, const WordsRange& targetSegment, const Phrase& targetPhrase) {
  //translation scores
  m_scores.PlusEquals(leftOption->GetScoreBreakdown());
  m_scores.PlusEquals(rightOption->GetScoreBreakdown());
  
  //don't worry about reordering because they don't change
  
  //word penalty
  float penalty = -((int)leftOption->GetTargetPhrase().GetSize()) -((int)rightOption->GetTargetPhrase().GetSize());
  m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
  
  addLanguageModelScore(targetWords, targetPhrase, targetSegment);
  
  //weight the scores
  const vector<float> & weights = StaticData::Instance().GetAllWeights();
  m_score = m_scores.InnerProduct(weights);

  VERBOSE(2, "Paired Update: Scores " << m_scores << endl);
  VERBOSE(2,"Paired Update: Total score is  " << m_score << endl);  

}


TranslationUpdateDelta::TranslationUpdateDelta(const vector<Word>& targetWords,  const TranslationOption* option , 
                                               const WordsRange& targetSegment) :
    m_option(option)  {
  initScoresSingleUpdate(targetWords,option,targetSegment);
}

void TranslationUpdateDelta::apply(Sample& sample, const TranslationDelta& noChangeDelta) {
  m_scores.MinusEquals(noChangeDelta.getScores());
  sample.ChangeTarget(*m_option,m_scores);
}


MergeDelta::MergeDelta(const vector<Word>& targetWords, const TranslationOption* option, const WordsRange& targetSegment) :
    m_option(option) {
  initScoresSingleUpdate(targetWords,option,targetSegment);
}

void MergeDelta::apply(Sample& sample, const TranslationDelta& noChangeDelta) {
  m_scores.MinusEquals(noChangeDelta.getScores());
  sample.MergeTarget(*m_option,m_scores);
}

PairedTranslationUpdateDelta::PairedTranslationUpdateDelta(const vector<Word>& targetWords,
   const TranslationOption* leftOption, const TranslationOption* rightOption, 
   const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment) :
    m_leftOption(leftOption), m_rightOption(rightOption){
    //For lm-scores treat as one large target segment, since the lmcontext may overlap, depending on the lm order
      
    WordsRange targetSegment(min(leftTargetSegment.GetStartPos(), rightTargetSegment.GetStartPos()), max(leftTargetSegment.GetEndPos(), rightTargetSegment.GetEndPos()));
    
    auto_ptr<Phrase> targetPhrase; 
    if (leftTargetSegment < rightTargetSegment) {
      targetPhrase = auto_ptr<Phrase>(new Phrase(leftOption->GetTargetPhrase()));
      //include potential words between the two target segments
      for (size_t i = leftTargetSegment.GetEndPos()+1; i < rightTargetSegment.GetStartPos(); ++i) {
        targetPhrase->AddWord(targetWords[i]);
      }
      targetPhrase->Append(rightOption->GetTargetPhrase());                    
    }
    else {
      targetPhrase = auto_ptr<Phrase>(new Phrase(rightOption->GetTargetPhrase()));
      //include potential words between the two target segments
      for (size_t i = rightTargetSegment.GetEndPos()+1; i < leftTargetSegment.GetStartPos(); ++i) {
        targetPhrase->AddWord(targetWords[i]);
      }
      targetPhrase->Append(leftOption->GetTargetPhrase());
    }
      
      
    initScoresPairedUpdate(targetWords,leftOption,rightOption,targetSegment,*targetPhrase);
}

void PairedTranslationUpdateDelta::apply(Sample& sample, const TranslationDelta& noChangeDelta) {
  m_scores.MinusEquals(noChangeDelta.getScores());
  sample.ChangeTarget(*m_leftOption,m_scores);
  ScoreComponentCollection emptyScores;
  sample.ChangeTarget(*m_rightOption,emptyScores);
}

SplitDelta::SplitDelta(const vector<Word>& targetWords,
                       const TranslationOption* leftOption, const TranslationOption* rightOption, const WordsRange& targetSegment) :
    m_leftOption(leftOption), m_rightOption(rightOption) {
  Phrase targetPhrase (leftOption->GetTargetPhrase());
  const Phrase& rightTargetPhrase = rightOption->GetTargetPhrase();
  for (size_t i = 0; i < rightTargetPhrase.GetSize(); ++i) {
    targetPhrase.AddWord(rightTargetPhrase.GetWord(i));
  }
  initScoresPairedUpdate(targetWords,leftOption,rightOption,targetSegment,targetPhrase);
}

void SplitDelta::apply(Sample& sample, const TranslationDelta& noChangeDelta) {
  m_scores.MinusEquals(noChangeDelta.getScores());
  sample.SplitTarget(*m_leftOption,*m_rightOption,m_scores);
}
  
void FlipDelta::apply(Sample& sample, const TranslationDelta& noChangeDelta) {
  //cout << "Applying " << m_prevTgtHypo->GetNextHypo()->GetCurrTargetWordsRange() << " and " <<
  //    m_nextTgtHypo->GetPrevHypo()->GetCurrTargetWordsRange() << endl;
  m_scores.MinusEquals(noChangeDelta.getScores());
  //sample.FlipNodes(m_leftTgtOption->GetSourceWordsRange().GetStartPos(), m_rightTgtOption->GetSourceWordsRange().GetStartPos(), m_scores);
  sample.FlipNodes(*m_leftTgtOption, *m_rightTgtOption, m_prevTgtHypo, m_nextTgtHypo, m_scores);
  //cout << "Applied " << endl;
}

    
FlipDelta::FlipDelta(const vector<Word>& targetWords,  const TranslationOption* leftTgtOption , 
                         const TranslationOption* rightTgtOption,  const Hypothesis* prevTgtHypo, const Hypothesis* nextTgtHypo, const WordsRange& leftTargetSegment, const WordsRange& rightTargetSegment, float totalDistortion ) :
    m_leftTgtOption(leftTgtOption), m_rightTgtOption(rightTgtOption), m_prevTgtHypo(const_cast<Hypothesis*> (prevTgtHypo)), m_nextTgtHypo(const_cast<Hypothesis*> (nextTgtHypo))  {
    
    //translation scores
    m_scores.PlusEquals(m_leftTgtOption->GetScoreBreakdown());
    m_scores.PlusEquals(m_rightTgtOption->GetScoreBreakdown());
    
    //word penalty
    float penalty = -((int) m_leftTgtOption->GetTargetPhrase().GetSize() + (int) m_rightTgtOption->GetTargetPhrase().GetSize());
    m_scores.Assign(StaticData::Instance().GetWordPenaltyProducer(),penalty);
    
    //LM
    WordsRange targetSegment = leftTargetSegment;
    targetSegment.SetStartPos(min(leftTargetSegment.GetStartPos(),rightTargetSegment.GetStartPos()) );
    targetSegment.SetEndPos(max(leftTargetSegment.GetEndPos(), rightTargetSegment.GetEndPos()) );
    
    Phrase targetPhrase(m_leftTgtOption->GetTargetPhrase());

    if (leftTargetSegment < rightTargetSegment) {
       for (size_t i =leftTargetSegment.GetEndPos()+1 ; i < rightTargetSegment.GetStartPos(); ++i) {
        targetPhrase.AddWord(targetWords[i]);
       }
    }
    else {
       for (size_t i =rightTargetSegment.GetEndPos()+1 ; i < leftTargetSegment.GetStartPos(); ++i) {
        targetPhrase.AddWord(targetWords[i]);
       }
    } 
    //include potential words between the two target segments
    targetPhrase.Append(m_rightTgtOption->GetTargetPhrase());

    
    addLanguageModelScore(targetWords, targetPhrase, targetSegment);
    
    //linear distortion
    const DistortionScoreProducer *dsp = StaticData::Instance().GetDistortionScoreProducer();
    m_scores.PlusEquals(dsp, totalDistortion);      
      
    //weight the scores
    const vector<float> & weights = StaticData::Instance().GetAllWeights();
    m_score = m_scores.InnerProduct(weights);
    
    VERBOSE(2, "Flip delta: Scores " << m_scores << endl);
    VERBOSE(2,"Flip delta: Total score is  " << m_score << endl);  
  }  
  
}//namespace
