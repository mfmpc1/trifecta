/*
 * MalicousRepParty.cpp
 *
 */

#ifndef GC_SHARETHREAD_HPP_
#define GC_SHARETHREAD_HPP_

#include <GC/ShareThread.h>
#include "GC/ShareParty.h"
#include "Math/Setup.h"

#include "Processor/Data_Files.hpp"

namespace GC
{

template<class T>
StandaloneShareThread<T>::StandaloneShareThread(int i, ThreadMaster<T>& master) :
        ShareThread<T>(master.N, master.opts, usage), Thread<T>(i, master)
{
}

template<class T>
ShareThread<T>::ShareThread(const Names& N, OnlineOptions& opts, DataPositions& usage) :
        P(0), MC(0), protocol(0), DataF(
                opts.live_prep ?
                        *static_cast<Preprocessing<T>*>(new typename T::LivePrep(
                                usage, *this)) :
                        *static_cast<Preprocessing<T>*>(new Sub_Data_Files<T>(N,
                                get_prep_sub_dir<T>(PREP_DIR, N.num_players()), usage)))
{
}

template<class T>
ShareThread<T>::ShareThread(const Names& N, OnlineOptions& opts, Player& P,
        typename T::mac_key_type mac_key, DataPositions& usage) :
        ShareThread(N, opts, usage)
{
    pre_run(P, mac_key);
}

template<class T>
ShareThread<T>::~ShareThread()
{
    delete &DataF;
    if (MC)
        delete MC;
    if (protocol)
        delete protocol;
}

template<class T>
void ShareThread<T>::pre_run(Player& P, typename T::mac_key_type mac_key)
{
    this->P = &P;
    if (singleton)
        throw runtime_error("there can only be one");
    singleton = this;
    protocol = new typename T::Protocol(*this->P);
    MC = this->new_mc(mac_key);
    DataF.set_protocol(*this->protocol);
}

template<class T>
void StandaloneShareThread<T>::pre_run()
{
    ShareThread<T>::pre_run(*Thread<T>::P, ShareParty<T>::s().mac_key);
    usage.set_num_players(Thread<T>::P->num_players());
}

template<class T>
void ShareThread<T>::post_run()
{
    MC->Check(*this->P);
#ifndef INSECURE
#ifdef VERBOSE
    cerr << "Removing used pre-processed data" << endl;
#endif
    DataF.prune();
#endif
}

template<class T>
void ShareThread<T>::and_(Processor<T>& processor,
        const vector<int>& args, bool repeat)
{
    auto& protocol = this->protocol;
//    TODO check args
//    processor.check_args(args, 4);
    protocol->init_mul(DataF, *this->MC);
    for (size_t i = 0; i < args.size(); i += args[i]+3)
    {
        int fanin_num = args[i];
        int n_bits = args[i + 1];
        vector<int> fanin;
        for (int k = 0; k < fanin_num; ++k) {
            fanin.push_back(args[i + 3 + k]);
        }
//        T y_ext;
        for (int j = 0; j < DIV_CEIL(n_bits, T::default_length); j++)
        {
            // TODO if we use andrs
            int n;
            if (repeat)
                cout << "repeat" << endl;
            else
                n = min(T::default_length, n_bits - j * T::default_length);
            vector<T> operands;
            for (int k = 0; k < fanin_num; ++k) {
                operands.push_back(processor.S[fanin[k] + j].mask(n));
            }
            protocol->prepare_mul(operands, n);
        }
    }

    protocol->exchange();

    for (size_t i = 0; i < args.size(); i += args[i]+3)
    {
        int n_bits = args[i + 1];
        int out = args[i + 2];
        for (int j = 0; j < DIV_CEIL(n_bits, T::default_length); j++)
        {
            int n = min(T::default_length, n_bits - j * T::default_length);
            processor.S[out + j] = protocol->finalize_mul(n).mask(n);
        }
    }
}

template<class T>
void ShareThread<T>::xors(Processor<T>& processor, const vector<int>& args)
{
    processor.check_args(args, 4);
    for (size_t i = 0; i < args.size(); i += 4)
    {
        int n_bits = args[i];
        int out = args[i + 1];
        int left = args[i + 2];
        int right = args[i + 3];
        for (int j = 0; j < DIV_CEIL(n_bits, T::default_length); j++)
        {
            int n = min(T::default_length, n_bits - j * T::default_length);
            processor.S[out + j].xor_(n, processor.S[left + j],
                    processor.S[right + j]);
        }
    }
}

} /* namespace GC */

#endif
