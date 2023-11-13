//
// Credit Charles Bouillageut
//
#include "../mitm_sequential.hpp"
#include <iostream>

/******************************************************************************/
/* Simple concrete example                                                    */
/******************************************************************************/





//
class IntDomain : AbstractDomain<unsigned int> {
public:
  using A = IntDomain;
  using t = unsigned int;
  template<class PRNG>
  void randomize(t &x, PRNG &p) const { x = p.rand(); }
  bool is_equal(const t &x, const t &y) const { return x == y; };
  const int length = 4;
  const static size_t n_elements = (1LL<<16);

  inline void next(t& x)  const { x = x+1; }
  static void serialize(const t &x, void *out) {
    t *tmp = static_cast<t*>(out);
    *tmp = x;
  }
  static void unserialize(t &x, void *in)  {
    t *tmp = static_cast<t*>(in);
    x = *tmp;
  }
  static uint64_t hash(const t &x)  { return x; }
  static uint64_t hash_extra(const t &x)  { return 0; }
};

class Problem : AbstractProblem<IntDomain, IntDomain, IntDomain> {
public:
  IntDomain dom_A, dom_B, dom_C;
  using A = IntDomain;
  using A_t = unsigned int;

  using B = IntDomain;
  using B_t = unsigned int;

  using C = IntDomain;
  using C_t = unsigned int;



  Problem(IntDomain& dom_A, IntDomain& dom_B, IntDomain& dom_C)
    : dom_A{dom_A}
    , dom_B{dom_B}
    , dom_C{dom_C}
  {};

  static inline void f(const unsigned int &x, unsigned int &y)
  {
    y = 42 * x * x + 1337;
  }
};

/******************************************************************************/

int main()
{
  IntDomain dom;          // create an instance of the domain
  Problem pb(dom, dom, dom);        // create an instance of the problem

  /*
   * I looked at the assembler code generated by gcc:
   * f is correctly inlined starting with -O1
   */
  std::pair result = collision(pb);

  std::cout << "x = " << result.first << " and y = " << result.second << std::endl;
  unsigned int fx, fy;
  Problem::f(result.first, fx);
  Problem::f(result.second, fy);
  std::cout << "f(x) = " << fx << " and f(y) = " << fy << std::endl;

  assert(fx == fy && result.first != result.second);
  return 0;
}
