/*
 * ReplicatedPrep.h
 *
 */

#ifndef GC_SEMIHONESTREPPREP_H_
#define GC_SEMIHONESTREPPREP_H_

#include "RepPrep.h"
#include "ShareSecret.h"

namespace GC
{

class SemiHonestRepPrep : public RepPrep<SemiHonestRepSecret>
{
public:
    SemiHonestRepPrep(DataPositions& usage, ShareThread<SemiHonestRepSecret>&) :
            RepPrep<SemiHonestRepSecret>(usage)
    {
    }
    SemiHonestRepPrep(DataPositions& usage, bool = false) :
            RepPrep<SemiHonestRepSecret>(usage)
    {
    }

    void buffer_triples() { throw not_implemented(); }
};

class SemiHonestMultiRepPrep : public RepPrep<SemiHonestMultiRepSecret>
{
public:
    SemiHonestMultiRepPrep(DataPositions& usage, ShareThread<SemiHonestMultiRepSecret>&) :
            RepPrep<SemiHonestMultiRepSecret>(usage)
    {
    }
    SemiHonestMultiRepPrep(DataPositions& usage, bool = false) :
            RepPrep<SemiHonestMultiRepSecret>(usage)
    {
    }

    void buffer_triples() { throw not_implemented(); }
};

} /* namespace GC */

#endif /* GC_SEMIHONESTREPPREP_H_ */
