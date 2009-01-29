#pragma once

#include <map>
#include "GibbsOperator.h"
#include "ScoreComponentCollection.h"

namespace Moses {

class Hypothesis;
class TranslationOptionCollection;
class TranslationOption;

  
class Sample {
 private:
  int source_size;
  Hypothesis* target_head;
  Hypothesis* target_tail;

  Hypothesis* source_head;
  Hypothesis* source_tail;

  ScoreComponentCollection feature_values;
  
  std::map<size_t, Hypothesis*>  sourceIndexedHyps;
  void SetSourceIndexedHyps(Hypothesis* h);
  void CopyTgtSidePtrs(Hypothesis* currHyp, Hypothesis* newHyp);
  void CopySrcSidePtrs(Hypothesis* currHyp, Hypothesis* newHyp);

 public:
  Sample(Hypothesis* target_head);
  int GetSourceSize() { return source_size; }
  Hypothesis* GetHypAtSourceIndex(size_t );
  const Hypothesis* GetSampleHypothesis() const {
    return source_head;
  }
  const ScoreComponentCollection& GetFeatureValues() const {
    return feature_values;
  }
  void FlipNodes(size_t, size_t);
  void AddNode(const TranslationOption&);
  
};

class Sampler {
 private:

 public:
  void Run(Hypothesis* starting, const TranslationOptionCollection* options) ;

};

/**
  * Used by the operators to collect samples, for example to count ngrams, or just to print
  * them out. 
  **/
class SampleCollector {
  public:
    virtual void collect(Sample& sample) = 0;
};

class PrintSampleCollector  : public virtual SampleCollector {
  public:
    virtual void collect(Sample& sample);
};

}



