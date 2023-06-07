/*
 * Replicated.cpp
 *
 */

#ifndef PROTOCOLS_MULTIREPLICATED_HPP_
#define PROTOCOLS_MULTIREPLICATED_HPP_

#include "MultiReplicated.h"
#include "Processor/Processor.h"
#include "Tools/benchmarking.h"

#include "SemiShare.h"
#include "SemiMC.h"
#include "ReplicatedInput.h"
#include "Rep3Share2k.h"

#include "SemiMC.hpp"
#include "Math/Z2k.hpp"
#include <sys/time.h>
#include <ctime>
#include <chrono>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

template<class T>
MultiReplicated<T>::MultiReplicated(Player &P) : MultiReplicatedBase(P) {
    assert(T::length == 2);
}

template<class T>
MultiReplicated<T>::MultiReplicated(const MultiReplicatedBase &other) :
        MultiReplicatedBase(other) {
}

inline MultiReplicatedBase::MultiReplicatedBase(Player &P) : P(P) {
    assert(P.num_players() == 3);
    if (not P.is_encrypted())
        insecure("unencrypted communication");
    shared_prngs[0].ReSeed();
    shared_prngs[1].ReSeed();
    octetStream os1, os2;
    os1.append(shared_prngs[0].get_seed(), SEED_SIZE);
    os2.append(shared_prngs[1].get_seed(), SEED_SIZE);
    P.send_relative(-1, os1);
    P.send_relative(1, os2);
    P.receive_relative(1, os1);
    P.receive_relative(-1, os2);
    shared_prngs[2].SetSeed(os1.get_data());
    shared_prngs[3].SetSeed(os2.get_data());
}

inline MultiReplicatedBase::MultiReplicatedBase(Player &P, array<PRNG, 4> &prngs) :
        P(P) {
    for (int i = 0; i < 2; i++)
        shared_prngs[i].SetSeed(prngs[i]);
}

inline MultiReplicatedBase MultiReplicatedBase::branch() {
    return {P, shared_prngs};
}

template<class T>
void MultiReplicated<T>::init_mul(SubProcessor<T> *proc) {
    (void) proc;
    init_mul();
}

template<class T>
void MultiReplicated<T>::init_mul(Preprocessing<T> &prep, typename T::MAC_Check &MC) {
    (void) prep, (void) MC;
    init_mul();
}

template<class T>
void MultiReplicated<T>::init_mul() {

    os.resize(4);
    for (auto &o : os)
        o.reset_write_head();
    add_shares.clear();
    add_shares_2.clear();
    and_operands.clear();
}


template<class T>
inline typename T::clear MultiReplicated<T>::prepare_mul(const vector <T> &operands, int n) {

    typename T::value_type comm_elem;
    typename T::value_type comm_elem1;
    vector <T> saved_operands;
    if (operands.size() == 2) {

        T x = operands[0];
        T y = operands[1];

        saved_operands.push_back(x);
        saved_operands.push_back(y);
        and_operands.push_back(saved_operands);

        if (P.my_num() == 0) {
            comm_elem = x[0] * y[0];

            randomize_comm_elem_0(comm_elem, n, 2);
        } else if (P.my_num() == 1) {
            comm_elem = x[0] * y[1] + x[1] * y[0];
            randomize_comm_elem_1(comm_elem, n, 2);
        } else {
            comm_elem = x[0] * y[1] + x[1] * y[0] + x[0] * y[0];
            randomize_comm_elem_2(comm_elem, n, 2);
        }
    }
    if (operands.size() == 3) {

        T x0 = operands[0];
        T x1 = operands[1];
        T x2 = operands[2];
        saved_operands.push_back(x0);
        saved_operands.push_back(x1);
        saved_operands.push_back(x2);
        and_operands.push_back(saved_operands);


        if (P.my_num() == 0) {

            typename T::clear rnd;
            rnd.randomize(shared_prngs[3], n);
            add_shares.push_back(rnd);

            comm_elem = x0[0] * x1[0] * x2[0];
            randomize_comm_elem_0(comm_elem, n, 3);

            comm_elem = x0[0] * x1[0];
            randomize_comm_elem_0(comm_elem, n, 3);

            comm_elem = x0[0] * x2[0];
            randomize_comm_elem_0(comm_elem, n, 3);

            comm_elem = x1[0] * x2[0];
            randomize_comm_elem_0(comm_elem, n, 3);


        } else if (P.my_num() == 1) {

            typename T::clear rnd;
            rnd.randomize(shared_prngs[2], n);
            add_shares.push_back(rnd);

            comm_elem = x0[0] * x1[0] * x2[0];
            randomize_comm_elem_1(comm_elem, n, 3);

            comm_elem = x0[0] * x1[0];
            randomize_comm_elem_1(comm_elem, n, 3);

            comm_elem = x0[0] * x2[0];
            randomize_comm_elem_1(comm_elem, n, 3);

            comm_elem = x1[0] * x2[0];
            randomize_comm_elem_1(comm_elem, n, 3);

        } else {

            typename T::clear rnd[4];
            typename T::clear rnd_1[4];

            for (int i = 0; i < 4; i++) {
                rnd[i].randomize(shared_prngs[2], n);
            }

            for (int i = 0; i < 4; i++) {
                rnd_1[i].randomize(shared_prngs[3], n);
            }

            comm_elem = rnd[0] + rnd[1] * x2[0] + rnd[2] * x1[0] + rnd[3] * x0[0] + x0[1] * x1[0] * x2[0] +
                        x0[0] * x1[1] * x2[0] + x0[0] * x1[0] * x2[1];
            randomize_comm_elem_3(comm_elem, n, 3);

            comm_elem = rnd_1[0] + rnd_1[1] * x2[1] + rnd_1[2] * x1[1] + rnd_1[3] * x0[1] + x0[0] * x1[1] * x2[1] +
                        x0[1] * x1[0] * x2[1] + x0[1] * x1[1] * x2[0];
            randomize_comm_elem_2(comm_elem, n, 3);
        }


    }
    if (operands.size() == 4) {

        T x0 = operands[0];
        T x1 = operands[1];
        T x2 = operands[2];
        T x3 = operands[3];
        saved_operands.push_back(x0);
        saved_operands.push_back(x1);
        saved_operands.push_back(x2);
        saved_operands.push_back(x3);
        and_operands.push_back(saved_operands);

        if (P.my_num() == 0) {

            typename T::clear rnd;
            rnd.randomize(shared_prngs[3], n);
            add_shares.push_back(rnd);

            comm_elem = x0[0] * x1[0] * x2[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x0[0] * x1[0] * x2[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x0[0] * x1[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x0[0] * x2[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x1[0] * x2[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x0[0] * x1[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x0[0] * x2[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x0[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x1[0] * x2[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x1[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

            comm_elem = x2[0] * x3[0];
            randomize_comm_elem_0(comm_elem, n, 4);

        } else if (P.my_num() == 1) {

            typename T::clear rnd;
            rnd.randomize(shared_prngs[2], n);
            add_shares.push_back(rnd);

            comm_elem = x0[0] * x1[0] * x2[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x0[0] * x1[0] * x2[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x0[0] * x1[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x0[0] * x2[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x1[0] * x2[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x0[0] * x1[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x0[0] * x2[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x0[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x1[0] * x2[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x1[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);

            comm_elem = x2[0] * x3[0];
            randomize_comm_elem_1(comm_elem, n, 4);
        } else {

            typename T::clear rnd[11];
            typename T::clear rnd_1[11];

            for (int i = 0; i < 11; i++) {
                rnd[i].randomize(shared_prngs[2], n);
            }

            for (int i = 0; i < 11; i++) {
                rnd_1[i].randomize(shared_prngs[3], n);
            }

            comm_elem = rnd[0] + rnd[1] * x3[0] + rnd[2] * x2[0] + rnd[3] * x1[0] + rnd[4] * x0[0] +
                        rnd[5] * x2[0] * x3[0] + rnd[6] * x1[0] * x3[0] + rnd[7] * x1[0] * x2[0] +
                        rnd[8] * x0[0] * x3[0] +
                        rnd[9] * x0[0] * x2[0] + rnd[10] * x0[0] * x1[0] +
                        x0[1] * x1[0] * x2[0] * x3[0] + x0[0] * x1[1] * x2[0] * x3[0] + x0[0] * x1[0] * x2[1] * x3[0] +
                        x0[0] * x1[0] * x2[0] * x3[1];
            randomize_comm_elem_3(comm_elem, n, 4);

            comm_elem = rnd_1[0] + rnd_1[1] * x3[1] + rnd_1[2] * x2[1] + rnd_1[3] * x1[1] + rnd_1[4] * x0[1] +
                        rnd_1[5] * x2[1] * x3[1] + rnd_1[6] * x1[1] * x3[1] + rnd_1[7] * x1[1] * x2[1] +
                        rnd_1[8] * x0[1] * x3[1] +
                        rnd_1[9] * x0[1] * x2[1] + rnd_1[10] * x0[1] * x1[1] +
                        x0[0] * x1[1] * x2[1] * x3[1] + x0[1] * x1[0] * x2[1] * x3[1] + x0[1] * x1[1] * x2[0] * x3[1] +
                        x0[1] * x1[1] * x2[1] * x3[0];
            randomize_comm_elem_2(comm_elem, n, 4);


        }

    }
    if (operands.size() > 4) {

        int fanin = operands.size();

        for (int i = 0; i < fanin; i++) {
            saved_operands.push_back(operands[i]);
        }
        and_operands.push_back(saved_operands);

        if (P.my_num() == 0 || P.my_num() == 1) {
            typename T::clear rnd;
            if (P.my_num() == 0)
                rnd.randomize(shared_prngs[3], n);
            else
                rnd.randomize(shared_prngs[2], n);

            add_shares.push_back(rnd);

            for (int i = (1 << fanin) - 1; i > 0; i--) {
                int indicator[fanin];
                int count = 0;
                for (int j = 0; j < fanin; j++) {
                    indicator[j] = (i >> j) & 1;
                    if (indicator[j] == 1) count++;
                }
                if (count == 0 || count == 1) continue;
                comm_elem = 1;
                for (int j = 0; j < fanin; j++) {
                    if (indicator[j] == 1)
                        comm_elem = comm_elem * operands[j][0];
                }
                if (P.my_num() == 0)
                    randomize_comm_elem_0(comm_elem, n, fanin);
                else
                    randomize_comm_elem_1(comm_elem, n, fanin);
            }
        } else {

            for (int i = (1 << fanin) - 1; i > 0; i--) {
                int indicator[fanin];
                int count = 0;
                for (int j = 0; j < fanin; j++) {
                    indicator[j] = (i >> j) & 1;
                    if (indicator[j] == 1) count++;
                }
                if (count == 0 || count == 1) continue;
                typename T::clear rnd;
                rnd.randomize(shared_prngs[2], n);
                for (int j = 0; j < fanin; j++) {
                    if (indicator[j] == 0)
                        rnd = rnd * operands[j][0];
                }
                comm_elem = comm_elem + rnd;
            }
            for (int i = 0; i < fanin; i++) {
                typename T::clear term = 1;
                for (int j = 0; j < fanin; j++) {
                    if (i == j)
                        term = term * operands[j][1];
                    else
                        term = term * operands[j][0];
                }
                comm_elem = comm_elem + term;
            }
            randomize_comm_elem_3(comm_elem, n, fanin);

            comm_elem = 0;
            for (int i = (1 << fanin) - 1; i > 0; i--) {
                int indicator[fanin];
                int count = 0;
                for (int j = 0; j < fanin; j++) {
                    indicator[j] = (i >> j) & 1;
                    if (indicator[j] == 1) count++;
                }
                if (count == 0 || count == 1) continue;
                typename T::clear rnd;
                rnd.randomize(shared_prngs[3], n);
                for (int j = 0; j < fanin; j++) {
                    if (indicator[j] == 0)
                        rnd = rnd * operands[j][1];
                }
                comm_elem = comm_elem + rnd;
            }
            for (int i = 0; i < fanin; i++) {
                typename T::clear term = 1;
                for (int j = 0; j < fanin; j++) {
                    if (i == j)
                        term = term * operands[j][0];
                    else
                        term = term * operands[j][1];
                }
                comm_elem = comm_elem + term;
            }
            randomize_comm_elem_2(comm_elem, n, fanin);


        }
    }

    return comm_elem1;
}

template<class T>
inline void MultiReplicated<T>::randomize_comm_elem_0(const typename T::clear &comm_elem,
                                                      int n, int fanin) {

    if (fanin == 2) {

        auto elem = comm_elem;
        typename T::clear rnd;

        rnd.randomize(shared_prngs[0], n);
        add_shares_2.push_back(elem);
        add_shares_2.push_back(rnd);

        elem += rnd;
        elem.pack(os[0], n);

    } else {

        auto elem = comm_elem;
        typename T::clear rnd;

        rnd.randomize(shared_prngs[0], n);

        elem += rnd;
        elem.pack(os[0], n);

    }
}

template<class T>
inline void MultiReplicated<T>::randomize_comm_elem_1(const typename T::clear &comm_elem,
                                                      int n, int fanin) {

    if (fanin == 2) {
        auto elem = comm_elem;
        typename T::clear rnd, rnd_1;
        rnd.randomize(shared_prngs[1], n);
        rnd_1.randomize(shared_prngs[2], n);
        add_shares_2.push_back(elem + rnd_1);
        add_shares_2.push_back(rnd + rnd_1);
        elem += rnd;
        elem.pack(os[0], n);
    } else {
        auto elem = comm_elem;
        typename T::clear rnd;

        rnd.randomize(shared_prngs[1], n);

        elem += rnd;
        elem.pack(os[0], n);
    }
}

template<class T>
inline void MultiReplicated<T>::randomize_comm_elem_2(const typename T::clear &comm_elem,
                                                      int n, int fanin) {
    if (fanin == 2) {
        auto elem = comm_elem;
        typename T::clear rnd;
        typename T::clear rnd_1[2];
        rnd_1[0].randomize(shared_prngs[3], n);
        rnd_1[1].randomize(shared_prngs[2], n);
        rnd.randomize(shared_prngs[0], n);
        elem += rnd;

        add_shares_2.push_back(rnd + rnd_1[0]);
        add_shares_2.push_back(elem + rnd_1[1]);
        elem.pack(os[0], n);
    } else {
        auto elem = comm_elem;
        typename T::clear rnd;
        rnd.randomize(shared_prngs[1], n);
        elem += rnd;

        add_shares.push_back(elem);
        elem.pack(os[1], n);
    }
}

template<class T>
inline void MultiReplicated<T>::randomize_comm_elem_3(const typename T::clear &comm_elem,
                                                      int n, int fanin) {
    if (fanin == 2) {
        auto elem = comm_elem;
        typename T::clear rnd;
        rnd.randomize(shared_prngs[0], n);
        elem += rnd;

        add_shares_2.push_back(rnd);
        add_shares_2.push_back(elem);
        elem.pack(os[0], n);
    } else {
        auto elem = comm_elem;
        typename T::clear rnd;
        rnd.randomize(shared_prngs[0], n);
        elem += rnd;

        add_shares.push_back(elem);
        elem.pack(os[0], n);
    }
}


template<class T>
void MultiReplicated<T>::exchange() {
    auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (P.my_num() == 0) {
        P.send_relative(1, os[0]);
        P.receive_relative(-1, os[2]);
        P.receive_relative(1, os[1]);
    }
    else if (P.my_num() == 1) {
	P.send_relative(-1, os[0]);
	P.receive_relative(-1, os[1]);
        P.receive_relative(1, os[2]);
    }
    else if (P.my_num() == 2) {
        P.send_relative(1, os[0]);
        P.send_relative(-1, os[1]);
    }
}

template<class T>
void MultiReplicated<T>::start_exchange() {
    P.send_relative(1, os[0]);
}

template<class T>
void MultiReplicated<T>::stop_exchange() {
    P.receive_relative(-1, os[1]);
}

template<class T>
inline T MultiReplicated<T>::finalize_mul(int n) {

    T result;
    vector <T> saved_operands = and_operands.next();


    if (saved_operands.size() == 2) {
        if (P.my_num() == 0) {
            typename T::clear received_prev, received_next;
            received_next.unpack(os[1], n);
            received_prev.unpack(os[2], n);
            result[0] = add_shares_2.next() + received_next + received_prev;
            result[1] = received_prev + add_shares_2.next();
        } else if (P.my_num() == 1) {
            typename T::clear received;
            received.unpack(os[1], n);
            result[0] = received + add_shares_2.next();
            result[1] = add_shares_2.next();
        } else if (P.my_num() == 2) {

            result[0] = add_shares_2.next();
            result[1] = add_shares_2.next();
        }
    } else if (saved_operands.size() == 3) {
        T x0 = saved_operands[0];
        T x1 = saved_operands[1];
        T x2 = saved_operands[2];
        if (P.my_num() == 0) {
            typename T::clear c1, c2, c3, c4, beta, rnd;
            c1.unpack(os[1], n);
            c2.unpack(os[1], n);
            c3.unpack(os[1], n);
            c4.unpack(os[1], n);
            beta.unpack(os[2], n);
            rnd = add_shares.next();
            result[0] = c1 + c2 * x2[1] + c3 * x1[1] + c4 * x0[1] + x0[0] * x1[1] * x2[1] + x0[1] * x1[0] * x2[1] +
                        x0[1] * x1[1] * x2[0] + rnd;
            result[1] = beta;



        } else if (P.my_num() == 1) {
            typename T::clear c1, c2, c3, c4, alpha, rnd;

            c1.unpack(os[1], n);
            c2.unpack(os[1], n);
            c3.unpack(os[1], n);
            c4.unpack(os[1], n);
            alpha.unpack(os[2], n);
            rnd = add_shares.next();

            result[0] = c1 + c2 * x2[1] + c3 * x1[1] + c4 * x0[1] + x0[0] * x1[1] * x2[1] + x0[1] * x1[0] * x2[1] +
                        x0[1] * x1[1] * x2[0] + rnd;
            result[1] = alpha;

        } else if (P.my_num() == 2) {
            typename T::clear beta, alpha;

            beta = add_shares.next();
            alpha = add_shares.next();

            result[0] = alpha;
            result[1] = beta;

        }
    } else if (saved_operands.size() == 4) {


        T x0 = saved_operands[0];
        T x1 = saved_operands[1];
        T x2 = saved_operands[2];
        T x3 = saved_operands[3];

        if (P.my_num() == 0) {
            typename T::clear c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, beta, rnd;

            c1.unpack(os[1], n);
            c2.unpack(os[1], n);
            c3.unpack(os[1], n);
            c4.unpack(os[1], n);
            c5.unpack(os[1], n);
            c6.unpack(os[1], n);
            c7.unpack(os[1], n);
            c8.unpack(os[1], n);
            c9.unpack(os[1], n);
            c10.unpack(os[1], n);
            c11.unpack(os[1], n);
            beta.unpack(os[2], n);

            result[0] = c1 + c2 * x3[1] + c3 * x2[1] + c4 * x1[1] + c5 * x0[1] +
                        c6 * x2[1] * x3[1] + c7 * x1[1] * x3[1] + c8 * x1[1] * x2[1] + c9 * x0[1] * x3[1] +
                        c10 * x0[1] * x2[1] + c11 * x0[1] * x1[1] +
                        x0[0] * x1[1] * x2[1] * x3[1] + x0[1] * x1[0] * x2[1] * x3[1] + x0[1] * x1[1] * x2[0] * x3[1] +
                        x0[1] * x1[1] * x2[1] * x3[0] + x0[1] * x1[1] * x2[1] * x3[1] + add_shares.next();
            result[1] = beta;


        } else if (P.my_num() == 1) {
            typename T::clear c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, alpha, rnd;

            c1.unpack(os[1], n);
            c2.unpack(os[1], n);
            c3.unpack(os[1], n);
            c4.unpack(os[1], n);
            c5.unpack(os[1], n);
            c6.unpack(os[1], n);
            c7.unpack(os[1], n);
            c8.unpack(os[1], n);
            c9.unpack(os[1], n);
            c10.unpack(os[1], n);
            c11.unpack(os[1], n);
            alpha.unpack(os[2], n);

            result[0] = c1 + c2 * x3[1] + c3 * x2[1] + c4 * x1[1] + c5 * x0[1] +
                        c6 * x2[1] * x3[1] + c7 * x1[1] * x3[1] + c8 * x1[1] * x2[1] + c9 * x0[1] * x3[1] +
                        c10 * x0[1] * x2[1] + c11 * x0[1] * x1[1] +
                        x0[0] * x1[1] * x2[1] * x3[1] + x0[1] * x1[0] * x2[1] * x3[1] + x0[1] * x1[1] * x2[0] * x3[1] +
                        x0[1] * x1[1] * x2[1] * x3[0] + x0[1] * x1[1] * x2[1] * x3[1] + add_shares.next();
            result[1] = alpha;


        } else if (P.my_num() == 2) {
            typename T::clear beta, alpha;

            beta = add_shares.next();
            alpha = add_shares.next();
            result[0] = alpha;
            result[1] = beta;


        }

    } else {

        int fanin = saved_operands.size();

        if (P.my_num() == 0 || P.my_num() == 1) {
            result[0] = 0;
            int com_count = (1 << fanin) - fanin - 1;
            typename T::clear c[com_count];
            typename T::clear second_share;
            for (int i = 0; i < com_count; i++) {
                c[i].unpack(os[1], n);
            }
            second_share.unpack(os[2], n);

            int term_counter = 0;
            for (int i = (1 << fanin) - 1; i > 0; i--) {
                int indicator[fanin];
                int count = 0;
                for (int j = 0; j < fanin; j++) {
                    indicator[j] = (i >> j) & 1;
                    if (indicator[j] == 1) count++;
                }
                if (count == 0 || count == 1) continue;
                typename T::clear term = c[term_counter];
                for (int j = 0; j < fanin; j++) {
                    if (indicator[j] == 0)
                        term = term * saved_operands[j][1];
                }
                result[0] = result[0] + term;
                term_counter++;
            }
            for (int i = 0; i < fanin; i++) {
                typename T::clear term = 1;
                for (int j = 0; j < fanin; j++) {
                    if (i == j)
                        term = term * saved_operands[j][0];
                    else
                        term = term * saved_operands[j][1];
                }
                result[0] = result[0] + term;
            }
            if ((fanin & 1) == 0) {
                typename T::clear term = 1;
                for (int i = 0; i < fanin; i++) {
                    term = term * saved_operands[i][1];
                }
                result[0] = result[0] + term;
            }
            result[0] = result[0] + add_shares.next();
            result[1] = second_share;
        } else {
            typename T::clear beta, alpha;

            beta = add_shares.next();
            alpha = add_shares.next();
            result[0] = alpha;
            result[1] = beta;
        }
    }

    return result;
}

template<class T>
inline void MultiReplicated<T>::init_dotprod(SubProcessor<T> *proc) {
    init_mul(proc);
    dotprod_share.assign_zero();
}

template<class T>
inline void MultiReplicated<T>::prepare_dotprod(const T &x, const T &y) {
    dotprod_share += x.local_mul(y);
}

template<class T>
inline void MultiReplicated<T>::next_dotprod() {
    prepare_reshare(dotprod_share);
    dotprod_share.assign_zero();
}

template<class T>
inline T MultiReplicated<T>::finalize_dotprod(int length) {
    (void) length;
    this->counter++;
    return finalize_mul();
}

template<class T>
T MultiReplicated<T>::get_random() {
    T res;
    for (int i = 0; i < 2; i++)
        res[i].randomize(shared_prngs[i]);
    return res;
}

template<class T>
void MultiReplicated<T>::trunc_pr(const vector<int> &regs, int size,
                                  SubProcessor<T> &proc) {
    ::trunc_pr(regs, size, proc);
}

#endif
