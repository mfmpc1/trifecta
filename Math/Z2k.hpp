/*
 * Z2k.cpp
 *
 */

#ifndef MATH_Z2K_HPP_
#define MATH_Z2K_HPP_

#include <Math/Z2k.h>
#include "Math/Integer.h"

template<int K>
const int Z2<K>::N_BITS;
template<int K>
const int Z2<K>::N_BYTES;

template<int K>
void Z2<K>::reqbl(int n)
{
	if (n < 0 && N_BITS != -(int)n)
	{
		throw Processor_Error(
				"Program compiled for rings of length " + to_string(-n)
				+ " but VM supports only "
				+ to_string(N_BITS));
	}
	else if (n > 0)
	{
		throw Processor_Error("Program compiled for fields not rings");
	}
}

template<int K>
bool Z2<K>::allows(Dtype dtype)
{
	return Integer::allows(dtype);
}

template<int K>
void Z2<K>::specification(octetStream& os)
{
	os.store(K);
}

template<int K>
Z2<K>::Z2(const bigint& x) : Z2()
{
	auto mp = x.get_mpz_t();
	memcpy(a, mp->_mp_d, min((size_t)N_BYTES, sizeof(mp_limb_t) * abs(mp->_mp_size)));
	if (mp->_mp_size < 0)
		*this = Z2<K>() - *this;
	normalize();
}

template<int K>
template<class T>
Z2<K>::Z2(const IntBase<T>& x) :
        Z2((uint64_t)x.get())
{
}

template<int K>
bool Z2<K>::get_bit(int i) const
{
	return 1 & (a[i / N_LIMB_BITS] >> (i % N_LIMB_BITS));
}

template<int K>
Z2<K> Z2<K>::operator&(const Z2<K>& other) const
{
    Z2<K> res;
    res.AND(*this, other);
    return res;
}

template<int K>
bool Z2<K>::operator==(const Z2<K>& other) const
{
#ifdef DEBUG_MPN
	for (int i = 0; i < N_WORDS; i++)
		cout << "cmp " << hex << a[i] << " " << other.a[i] << endl;
#endif
	return mpn_cmp(a, other.a, N_WORDS) == 0;
}

template<int K>
Z2<K>& Z2<K>::invert()
{
    if (get_bit(0) != 1)
        throw division_by_zero();

    Z2<K> res = 1;
    for (int i = 0; i < K; i++)
    {
        res += Z2<K>((Z2<K>(1) - Z2<K>::Mul(*this, res)).get_bit(i)) << i;
    }
    *this = res;
    return *this;
}

template<int K>
Z2<K> Z2<K>::sqrRoot()
{
	assert(a[0] % 8 == 1);
	Z2<K> res = 1;
	for (int i = 0; i < K - 1; i++)
	{
		res += Z2<K>((*this - Z2<K>::Mul(res, res)).get_bit(i + 1)) << i;
#ifdef DEBUG_SQR
		cout << "sqr " << dec << i << " " << hex << res << " " <<  res * res << " " << (*this - res * res) << endl;
#endif
	}
	return res;
}

template<int K>
void Z2<K>::AND(const Z2<K>& x, const Z2<K>& y)
{
	mpn_and_n(a, x.a, y.a, N_WORDS);
}

template<int K>
void Z2<K>::OR(const Z2<K>& x, const Z2<K>& y)
{
	mpn_ior_n(a, x.a, y.a, N_WORDS);
}

template<int K>
void Z2<K>::XOR(const Z2<K>& x, const Z2<K>& y)
{
	mpn_xor_n(a, x.a, y.a, N_WORDS);
}

template<int K>
void Z2<K>::input(istream& s, bool human)
{
	if (human)
	{
	    s >> bigint::tmp;
	    *this = bigint::tmp;
	}
	else
	    s.read((char*)a, N_BYTES);
}

template<int K>
void Z2<K>::output(ostream& s, bool human) const
{
	if (human)
	{
	    bigint::tmp = *this;
	    s << bigint::tmp;
	}
	else
		s.write((char*)a, N_BYTES);
}

template<int K>
SignedZ2<K>::SignedZ2(const Integer& other)
{
    if (K == 64)
        this->a[0] = other.get();
    else
        *this = SignedZ2<64>(other);
}

template <int K>
ostream& operator<<(ostream& o, const Z2<K>& x)
{
	x.output(o, true);
	return o;
}

template<int K>
istream& operator>>(istream& i, SignedZ2<K>& x)
{
    auto& tmp = bigint::tmp;
    i >> tmp;
    if (tmp.numBits() > K + 1)
        throw runtime_error(
                tmp.get_str() + " out of range for signed " + to_string(K)
                        + "-bit numbers");
    x = tmp;
    return i;
}

#endif
