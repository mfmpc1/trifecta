/*
 * VectorInput.h
 *
 */

#ifndef GC_VECTORINPUT_H_
#define GC_VECTORINPUT_H_

#include "Protocols/ReplicatedInput.h"

namespace GC
{

template<class T>
class VectorInput : public InputBase<T>
{
    typename T::part_type::Input part_input;
    deque<int> input_lengths;

public:
    VectorInput(typename T::MAC_Check&, Preprocessing<T>&, Player& P) :
            part_input(0, P)
    {
        part_input.reset_all(P);
    }

    void reset(int player)
    {
        part_input.reset(player);
    }

    void add_mine(const typename T::open_type& input, int n_bits)
    {
        for (int i = 0; i < n_bits; i++)
            part_input.add_mine(input.get_bit(i));
        input_lengths.push_back(n_bits);
    }

    void add_other(int)
    {
    }

    void send_mine()
    {
        part_input.send_mine();
    }

    void exchange()
    {
        part_input.exchange();
    }

    T finalize_mine()
    {
        T res;
        res.resize_regs(input_lengths.front());
        for (int i = 0; i < input_lengths.front(); i++)
            res.get_reg(i) = part_input.finalize_mine();
        input_lengths.pop_front();
        return res;
    }

    void finalize_other(int player, T& target, octetStream&, int n_bits)
    {
        target.resize_regs(n_bits);
        for (int i = 0; i < n_bits; i++)
            part_input.finalize_other(player, target.get_reg(i),
                    part_input.InputBase<typename T::part_type>::os[player]);
    }
};

} /* namespace GC */

#endif /* GC_VECTORINPUT_H_ */
