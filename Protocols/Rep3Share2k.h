/*
 * Rep3Share2k.h
 *
 */

#ifndef PROTOCOLS_REP3SHARE2K_H_
#define PROTOCOLS_REP3SHARE2K_H_

#include "Rep3Share.h"
#include "ReplicatedInput.h"
#include "Math/Z2k.h"
#include "GC/square64.h"

template<class T> class ReplicatedPrep2k;

template<int K>
class Rep3Share2 : public Rep3Share<SignedZ2<K>>
{
    typedef SignedZ2<K> T;

public:
    typedef Replicated<Rep3Share2> Protocol;
    typedef ReplicatedMC<Rep3Share2> MAC_Check;
    typedef MAC_Check Direct_MC;
    typedef ReplicatedInput<Rep3Share2> Input;
    typedef ::PrivateOutput<Rep3Share2> PrivateOutput;
    typedef ReplicatedPrep2k<Rep3Share2> LivePrep;
    typedef Rep3Share2 Honest;

    typedef GC::SemiHonestRepSecret bit_type;

    Rep3Share2()
    {
    }
    template<class U>
    Rep3Share2(const FixedVec<U, 2>& other)
    {
        FixedVec<T, 2>::operator=(other);
    }

    template<class U>
    static void split(vector<U>& dest, const vector<int>& regs,
            int n_bits, const Rep3Share2* source, int n_inputs, Player& P)
    {
        int my_num = P.my_num();
        assert(n_bits <= 64);
        int unit = GC::Clear::N_BITS;
        for (int k = 0; k < DIV_CEIL(n_inputs, unit); k++)
        {
            int start = k * unit;
            int m = min(unit, n_inputs - start);

            switch (regs.size() / n_bits)
            {
            case 3:
                for (int i = 0; i < n_bits; i++)
                    dest.at(regs.at(3 * i + my_num) + k) = {};

                for (int i = 0; i < 2; i++)
                {
                    square64 square;

                    for (int j = 0; j < m; j++)
                        square.rows[j] = Integer(source[j + start][i]).get();

                    square.transpose(m, n_bits);

                    for (int j = 0; j < n_bits; j++)
                    {
                        auto &dest_reg = dest.at(
                                regs.at(3 * j + ((my_num + 2 - i) % 3)) + k);
                        dest_reg[1 - i] = 0;
                        dest_reg[i] = square.rows[j];
                    }
                }
                break;
            case 2:
            {
                ReplicatedInput<U> input(P);
                input.reset_all(P);
                if (P.my_num() == 0)
                {
                    square64 square;
                    for (int j = 0; j < m; j++)
                        square.rows[j] = Integer(source[j + start].sum()).get();
                    square.transpose(m, n_bits);
                    for (int j = 0; j < n_bits; j++)
                        input.add_mine(square.rows[j], m);
                }
                else
                    for (int j = 0; j < n_bits; j++)
                        input.add_other(0);

                input.exchange();
                for (int j = 0; j < n_bits; j++)
                    dest.at(regs.at(2 * j) + k) = input.finalize(0, m);

                if (P.my_num() == 0)
                    for (int j = 0; j < n_bits; j++)
                        dest.at(regs.at(2 * j + 1) + k) = {};
                else
                {
                    square64 square;
                    for (int j = 0; j < m; j++)
                        square.rows[j] = Integer(source[j + start][P.my_num() - 1]).get();
                    square.transpose(m, n_bits);
                    for (int j = 0; j < n_bits; j++)
                    {
                        auto& dest_reg = dest.at(regs.at(2 * j + 1) + k);
                        dest_reg[P.my_num() - 1] = square.rows[j];
                        dest_reg[2 - P.my_num()] = {};
                    }
                }
                break;
            }
            default:
                throw runtime_error("number of split summands not implemented");
            }
        }
    }
};

#endif /* PROTOCOLS_REP3SHARE2K_H_ */
