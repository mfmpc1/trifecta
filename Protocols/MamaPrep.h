/*
 * MamaPrep.h
 *
 */

#ifndef PROTOCOLS_MAMAPREP_H_
#define PROTOCOLS_MAMAPREP_H_

#include "MascotPrep.h"

template<class T>
class MamaPrep : public OTPrep<T>
{
public:
    static void basic_setup(Player&) {};
    static void teardown() {};

    MamaPrep<T>(SubProcessor<T>* proc, DataPositions& usage);

    void buffer_triples();

};

#endif /* PROTOCOLS_MAMAPREP_H_ */