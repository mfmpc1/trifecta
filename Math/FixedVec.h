/*
 * FixedVec.h
 *
 */

#ifndef MATH_FIXEDVEC_H_
#define MATH_FIXEDVEC_H_

#include <string>
#include <array>
using namespace std;

#include "Tools/octetStream.h"
#include "Tools/random.h"
#include "field_types.h"

template<class T> class ReplicatedMC;
template<class T> class ReplicatedInput;
template<class T> class ReplicatedPrivateOutput;
template<class T> class Replicated;

template <class T, int L>
class FixedVec
{
    array<T, L> v;

public:
    typedef T value_type;
    typedef FixedVec Scalar;

    static const int length = L;

    static int size()
    {
        return L * T::size();
    }
    static string type_string()
    {
        return T::type_string() + "^" + to_string(L);
    }
    static string type_short()
    {
        return string(1, T::type_char());
    }
    static DataFieldType field_type()
    {
        return T::field_type();
    }

    template<class U, class V>
    static FixedVec Mul(const FixedVec<U, L>& a, const V& b)
    {
        FixedVec res;
        for (int i = 0; i < L; i++)
            res[i] = T::Mul(a[i], b);
        return res;
    }

    FixedVec<T, L>(const T& other = 0)
    {
        for (auto& x : v)
            x = other;
    }

    FixedVec<T, L>(long other) :
            FixedVec<T, L>(T(other))
    {
    }

    template<class U>
    FixedVec<T, L>(const FixedVec<U, L>& other)
    {
        for (int i = 0; i < L; i++)
            v[i] = other[i];
    }

    FixedVec<T, L>(const array<T, L>& other)
    {
        v = other;
    }

    const array<T, L>& get() const
    {
        return v;
    }

    T& operator[](int i)
    {
        return v[i];
    }
    const T& operator[](int i) const
    {
        return v[i];
    }

    void assign(const T& other)
    {
        for (auto& x : v)
            x = other;
    }
    void assign(const char* buffer)
    {
        for (int i = 0; i < L; i++)
            v[i].assign(buffer + i * T::size());
    }

    void assign_zero()
    {
        for (auto& x : v)
            x = 0;
    }
    void assign_one()
    {
        assign(1);
    }

    void add(const FixedVec<T, L>& x, const FixedVec<T, L>& y)
    {
        for (int i = 0; i < L; i++)
            v[i] = x.v[i] + y.v[i];
    }
    void sub(const FixedVec<T, L>& x, const FixedVec<T, L>& y)
    {
        for (int i = 0; i < L; i++)
            v[i] = x.v[i] - y.v[i];
    }

    void mul(const FixedVec<T, L>& x, const FixedVec<T, L>& y)
    {
        for (int i = 0; i < L; i++)
            v[i].mul(x.v[i], y.v[i]);
    }

    void add(const FixedVec<T, L>& x)
    {
        add(*this, x);
    }

    void add(octetStream& os)
    {
        for (int i = 0; i < L; i++)
            v[i].add(os);
    }

    void negate()
    {
        for (auto& x : v)
            x = -x;
    }

    bool equal(const FixedVec<T, L>& x) const
    {
        for (int i = 0; i < L; i++)
            if (v[i] != x[i])
                return false;
        return true;
    }

    bool operator!=(const FixedVec& other) const
    {
        return not equal(other);
    }

    bool is_zero()
    {
        return equal(0);
    }
    bool is_one()
    {
        return equal(1);
    }

    FixedVec<T, L>operator+(const FixedVec<T, L>& other) const
    {
        FixedVec<T, L> res;
        res.add(*this, other);
        return res;
    }

    FixedVec<T, L>operator-(const FixedVec<T, L>& other) const
    {
        FixedVec<T, L> res;
        res.sub(*this, other);
        return res;
    }

    FixedVec<T, L>operator*(const FixedVec<T, L>& other) const
    {
        FixedVec<T, L> res;
        res.mul(*this, other);
        return res;
    }

    FixedVec<T, L>operator/(const FixedVec<T, L>& other) const
    {
        FixedVec<T, L> res;
        for (int i = 0; i < L; i++)
            res[i] = v[i] / other[i];
        return res;
    }

    FixedVec<T, L>operator^(const FixedVec<T, L>& other) const
    {
        FixedVec<T, L> res;
        for (int i = 0; i < L; i++)
            res[i] = v[i] ^ other[i];
        return res;
    }

    FixedVec<T, L>operator&(const FixedVec<T, L>& other) const
    {
        FixedVec<T, L> res;
        for (int i = 0; i < L; i++)
            res[i] = v[i] & other[i];
        return res;
    }

    FixedVec<T, L>operator~() const
    {
        FixedVec<T, L> res;
        for (int i = 0; i < L; i++)
            res[i] = ~v[i];
        return res;
    }

    FixedVec<T, L>& operator+=(const FixedVec<T, L>& other)
    {
        add(other);
        return *this;
    }

    FixedVec<T, L>& operator-=(const FixedVec<T, L>& other)
    {
        sub(*this, other);
        return *this;
    }

    FixedVec<T, L>& operator*=(const FixedVec<T, L>& other)
    {
        *this = *this * other;
        return *this;
    }

    FixedVec<T, L>& operator/=(const FixedVec<T, L>& other)
    {
        *this = *this / other;
        return *this;
    }

    FixedVec<T, L>& operator^=(const FixedVec<T, L>& other)
    {
        for (int i = 0; i < L; i++)
            v[i] ^= other[i];
        return *this;
    }

    FixedVec<T, L>& operator&=(const FixedVec<T, L>& other)
    {
        for (int i = 0; i < L; i++)
            v[i] &= other[i];
        return *this;
    }

    FixedVec<T, L> operator<<(int i) const
    {
        FixedVec<T ,L> res;
        for (int j = 0; j < L; j++)
            res[j] = v[j] << i;
        return res;
    }

    FixedVec<T, L> operator>>(int i) const
    {
        FixedVec<T ,L> res;
        for (int j = 0; j < L; j++)
            res[j] = v[j] >> i;
        return res;
    }

    FixedVec<T, L>& operator>>=(int i)
    {
        *this = *this >> i;
        return *this;
    }

    T sum() const
    {
        T res = v[0];
        for (int i = 1; i < L; i++)
            res += v[i];
        return res;
    }

    FixedVec<T, L> extend_bit() const
    {
        FixedVec<T, L> res;
        for (int i = 0; i < L; i++)
            res[i] = v[i].extend_bit();
        return res;
    }

    FixedVec<T, L> mask(int n_bits) const
    {
        FixedVec<T, L> res;
        for (int i = 0; i < L; i++)
            res[i] = v[i].mask(n_bits);
        return res;
    }

    void randomize(PRNG& G)
    {
        for (auto& x : v)
            x.randomize(G);
    }

    void almost_randomize(PRNG& G)
    {
        for (auto& x : v)
            x.almost_randomize(G);
    }

    void randomize_to_sum(const T& sum, PRNG& G)
    {
        T s = 0;
        for (int i = 1; i < L; i++)
        {
            v[i].randomize(G);
            s += v[i];
        }
        v[0] = sum - s;
    }

    void force_to_bit()
    {
        for (auto& x : v)
            x.force_to_bit();
    }

    FixedVec get_bit(int i)
    {
        return (*this >> i) & 1;
    }

    void output(ostream& s, bool human) const
    {
        for (auto& x : v)
            x.output(s, human);
    }
    void input(istream& s, bool human)
    {
        for (auto& x : v)
            x.input(s, human);
    }

    void pack(octetStream& os) const
    {
        for (auto& x : v)
            x.pack(os);
    }
    void unpack(octetStream& os)
    {
        for (auto& x : v)
            x.unpack(os);
    }
};

template <class U, class T, int L>
FixedVec<T, L> operator*(const U& a, const FixedVec<T, L>& b)
{
    return b * T(a);
}

template <class T, int L>
ostream& operator<<(ostream& os, const FixedVec<T, L>& v)
{
    for (int i = 0; i < L; i++)
    {
        os << v[i];
        if (i < L - 1)
            os << ",";
    }
    return os;
}

#endif /* MATH_FIXEDVEC_H_ */
