//
// Created by Sina Faraji on 2020-09-03.
//

#ifndef MP_SPDZ_MULTIREPLICATEDMC_HPP
#define MP_SPDZ_MULTIREPLICATEDMC_HPP

template<class T>
void MultiReplicatedMC<T>::POpen(vector<typename T::open_type>& values,
                            const vector<T>& S, const Player& P)
{
    prepare(S);
    P.pass_around(this->to_send, this->o, 1);
    finalize(values, S, P);
}


template<class T>
void MultiReplicatedMC<T>::prepare(const vector<T>& S)
{
    assert(T::length == 2);
    this->o.reset_write_head();
    this->to_send.reset_write_head();
    this->to_send.reserve(S.size() * T::value_type::size());
    for (auto& x : S){
        x[0].pack(this->to_send);
    }
}

template<class T>
void MultiReplicatedMC<T>::finalize(vector<typename T::open_type>& values,
                               const vector<T>& S, const Player& P)
{
    values.resize(S.size());
    for (size_t i = 0; i < S.size(); i++)
    {
        typename T::open_type tmp;
        tmp.unpack(this->o);
        if (P.my_num() == 0)
            values[i] = S[i][0] + tmp;
        else
            values[i] = S[i][1] + tmp;
    }
}

#endif //MP_SPDZ_MULTIREPLICATEDMC_HPP
