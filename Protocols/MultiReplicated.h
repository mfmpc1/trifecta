/*
 * Replicated.h
 *
 */

#ifndef PROTOCOLS_MULTIREPLICATED_H_
#define PROTOCOLS_MULTIREPLICATED_H_

#include <assert.h>
#include <vector>
#include <array>
using namespace std;

#include "Tools/octetStream.h"
#include "Tools/random.h"
#include "Tools/PointerVector.h"
#include "Networking/Player.h"
#include "Replicated.h"

template<class T> class SubProcessor;
template<class T> class ReplicatedMC;
template<class T> class MultiReplicatedInput;
template<class T> class ReplicatedPrivateOutput;
template<class T> class Share;
template<class T> class Rep3Share;
template<class T> class MAC_Check_Base;
template<class T> class Preprocessing;

class MultiReplicatedBase
{
public:
    array<PRNG, 4> shared_prngs;

    Player& P;

    MultiReplicatedBase(Player& P);
    MultiReplicatedBase(Player& P, array<PRNG, 4>& prngs);

    MultiReplicatedBase branch();

    int get_n_relevant_players() { return P.num_players() - 1; }
};


template <class T>
class MultiReplicated : public MultiReplicatedBase, public ProtocolBase<T>
{
    vector<octetStream> os;
    PointerVector<vector<T>> and_operands;
    PointerVector<typename T::clear> add_shares;
    PointerVector<typename T::clear> add_shares_2;
    PointerVector<typename T::clear> next_relative_rnds;
    PointerVector<typename T::clear> prev_relative_rnds;
    typename T::clear dotprod_share;

public:
    typedef ReplicatedMC<T> MAC_Check;
    typedef ReplicatedInput<T> Input;

    static const bool uses_triples = false;

    MultiReplicated(Player& P);
    MultiReplicated(const MultiReplicatedBase& other);

    static void assign(T& share, const typename T::clear& value, int my_num)
    {
        assert(T::length == 2);
        share.assign_zero();
        if (my_num < 2)
            share[my_num] = value;
    }

    void init_mul(SubProcessor<T>* proc);
    void init_mul(Preprocessing<T>& prep, typename T::MAC_Check& MC);

    void init_mul();
    typename T::clear prepare_mul(const vector<T>& operands, int n = -1);
    void exchange();
    T finalize_mul(int n = -1);

    void randomize_comm_elem_0(const typename T::clear& share, int n = -1, int fanin = -1);
    void randomize_comm_elem_1(const typename T::clear& share, int n = -1, int fanin = -1);
    void randomize_comm_elem_2(const typename T::clear& share, int n = -1, int fanin = -1);
    void randomize_comm_elem_3(const typename T::clear& share, int n = -1, int fanin = -1);

    void init_dotprod(SubProcessor<T>* proc);
    void prepare_dotprod(const T& x, const T& y);
    void next_dotprod();
    T finalize_dotprod(int length);

    void trunc_pr(const vector<int>& regs, int size, SubProcessor<T>& proc);

    T get_random();

    void start_exchange();
    void stop_exchange();
};

#endif /* PROTOCOLS_REPLICATED_H_ */
