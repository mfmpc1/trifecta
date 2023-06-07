#ifndef _bigint
#define _bigint

#include <iostream>
using namespace std;

#include <stddef.h>
#include <mpirxx.h>

#include "Exceptions/Exceptions.h"
#include "Tools/int.h"
#include "Tools/random.h"
#include "Tools/octetStream.h"
#include "Tools/avx_memcpy.h"

enum ReportType
{
  CAPACITY,
  USED,
  MINIMAL,
  REPORT_TYPE_MAX
};

template<int X, int L>
class gfp_;
class gmp_random;
class Integer;
template<int K> class Z2;
template<int K> class SignedZ2;
template<int L> class fixint;

namespace GC
{
  class Clear;
}

class bigint : public mpz_class
{
public:
  static thread_local bigint tmp;
  static thread_local gmp_random random;

  // workaround for GCC not always initializing thread_local variables
  static void init_thread() { tmp = 0; }

  template<class T>
  static mpf_class get_float(T v, T p, T z, T s);
  template<class U, class T>
  static void output_float(U& o, const mpf_class& x, T nan);

  bigint() : mpz_class() {}
  template <class T>
  bigint(const T& x) : mpz_class(x) {}
  template<int X, int L>
  bigint(const gfp_<X, L>& x);
  template <int K>
  bigint(const Z2<K>& x);
  template <int K>
  bigint(const SignedZ2<K>& x);
  template <int L>
  bigint(const fixint<L>& x) : bigint(typename fixint<L>::super(x)) {}
  bigint(const Integer& x);
  bigint(const GC::Clear& x);

  bigint& operator=(int n);
  bigint& operator=(long n);
  bigint& operator=(word n);
  template<int X, int L>
  bigint& operator=(const gfp_<X, L>& other);

  void allocate_slots(const bigint& x) { *this = x; }
  int get_min_alloc() { return get_mpz_t()->_mp_alloc; }

  void negate() { mpz_neg(get_mpz_t(), get_mpz_t()); }

  void mul(const bigint& x, const bigint& y) { *this = x * y; }

#ifdef REALLOC_POLICE
  ~bigint() { lottery(); }
  void lottery();

  bigint& operator-=(const bigint& y)
  {
    if (rand() % 10000 == 0)
      if (get_mpz_t()->_mp_alloc < abs(y.get_mpz_t()->_mp_size) + 1)
        throw runtime_error("insufficient allocation");
    ((mpz_class&)*this) -= y;
    return *this;
  }
  bigint& operator+=(const bigint& y)
  {
    if (rand() % 10000 == 0)
      if (get_mpz_t()->_mp_alloc < abs(y.get_mpz_t()->_mp_size) + 1)
        throw runtime_error("insufficient allocation");
    ((mpz_class&)*this) += y;
    return *this;
  }
#endif

  int numBits() const
  { return mpz_sizeinbase(get_mpz_t(), 2); }

  void generateUniform(PRNG& G, int n_bits, bool positive = false)
  { G.get_bigint(*this, n_bits, positive); }

  void pack(octetStream& os) const { os.store(*this); }
  void unpack(octetStream& os)     { os.get(*this); };

  size_t report_size(ReportType type) const;
};


void inline_mpn_zero(mp_limb_t* x, mp_size_t size);
void inline_mpn_copyi(mp_limb_t* dest, const mp_limb_t* src, mp_size_t size);

#include "Z2k.h"


inline bigint& bigint::operator=(int n)
{
  mpz_class::operator=(n);
  return *this;
}

inline bigint& bigint::operator=(long n)
{
  mpz_class::operator=(n);
  return *this;
}

inline bigint& bigint::operator=(word n)
{
  mpz_class::operator=(n);
  return *this;
}

template<int K>
bigint::bigint(const Z2<K>& x)
{
  mpz_import(get_mpz_t(), Z2<K>::N_WORDS, -1, sizeof(mp_limb_t), 0, 0, x.get_ptr());
}

template<int K>
bigint::bigint(const SignedZ2<K>& x)
{
  mpz_import(get_mpz_t(), Z2<K>::N_WORDS, -1, sizeof(mp_limb_t), 0, 0, x.get_ptr());
  if (x.negative())
  {
    bigint::tmp = 1;
    bigint::tmp <<= K;
    *this -= bigint::tmp;
  }
}

template<int X, int L>
bigint::bigint(const gfp_<X, L>& x)
{
  *this = x;
}

template<int X, int L>
bigint& bigint::operator=(const gfp_<X, L>& x)
{
  to_bigint(*this, x);
  return *this;
}


/**********************************
 *       Utility Functions        *
 **********************************/

inline int gcd(const int x,const int y)
{
  bigint& xx = bigint::tmp = x;
  return mpz_gcd_ui(NULL,xx.get_mpz_t(),y);
}


inline bigint gcd(const bigint& x,const bigint& y)
{ 
  bigint g;
  mpz_gcd(g.get_mpz_t(),x.get_mpz_t(),y.get_mpz_t());
  return g;
}


inline void invMod(bigint& ans,const bigint& x,const bigint& p)
{
  mpz_invert(ans.get_mpz_t(),x.get_mpz_t(),p.get_mpz_t());
}

inline int numBits(const bigint& m)
{
  return m.numBits();
}



inline int numBits(long m)
{
  bigint& te = bigint::tmp = m;
  return mpz_sizeinbase(te.get_mpz_t(),2);
}



inline int numBytes(const bigint& m)
{
  return mpz_sizeinbase(m.get_mpz_t(),256);
}





inline int probPrime(const bigint& x)
{
  gmp_randstate_t rand_state;
  gmp_randinit_default(rand_state);
  int ans=mpz_probable_prime_p(x.get_mpz_t(),rand_state,40,0);
  gmp_randclear(rand_state);
  return ans;
}


inline void bigintFromBytes(bigint& x,octet* bytes,int len)
{
#ifdef REALLOC_POLICE
  if (rand() % 10000 == 0)
    if (x.get_mpz_t()->_mp_alloc < ((len + 7) / 8))
      throw runtime_error("insufficient allocation");
#endif
  mpz_import(x.get_mpz_t(),len,1,sizeof(octet),0,0,bytes);
}


inline void bytesFromBigint(octet* bytes,const bigint& x,unsigned int len)
{
  size_t ll;
  mpz_export(bytes,&ll,1,sizeof(octet),0,0,x.get_mpz_t());
  if (ll>len)
    { throw invalid_length(); }
  for (unsigned int i=ll; i<len; i++)
    { bytes[i]=0; }
}


inline int isOdd(const bigint& x)
{
  return mpz_odd_p(x.get_mpz_t());
}


bigint sqrRootMod(const bigint& x,const bigint& p);

bigint powerMod(const bigint& x,const bigint& e,const bigint& p);

// Assume e>=0
int powerMod(int x,int e,int p);

inline int Hwt(int N)
{
  int result=0;
  while(N)
    { result++;
      N&=(N-1);
    }
  return result;
}

template <class T>
int limb_size();

#endif

