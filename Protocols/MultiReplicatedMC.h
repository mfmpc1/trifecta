//
// Created by Sina Faraji on 2020-09-03.
//

#ifndef MP_SPDZ_MULTIREPLICATEDMC_H
#define MP_SPDZ_MULTIREPLICATEDMC_H

#include "ReplicatedMC.h"

template <class T>
class MultiReplicatedMC : public ReplicatedMC<T>
{

    void prepare(const vector<T>& S);
    void finalize(vector<typename T::open_type>& values, const vector<T>& S, const Player& P);

public:
    // emulate MAC_Check
    MultiReplicatedMC(const typename T::value_type& _ = {}, int __ = 0, int ___ = 0)
    { (void)_; (void)__; (void)___; }

    // emulate Direct_MAC_Check
    MultiReplicatedMC(const typename T::value_type& _, Names& ____, int __ = 0, int ___ = 0)
    { (void)_; (void)__; (void)___; (void)____; }

    void POpen(vector<typename T::open_type>& values,const vector<T>& S,const Player& P);

    MultiReplicatedMC& get_part_MC()
    {
        return *this;
    }

};

#endif //MP_SPDZ_MULTIREPLICATEDMC_H
