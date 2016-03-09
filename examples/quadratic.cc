#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>

#include <malloc.h>

#include <VecCore>
#include "Timer.h"

using namespace vecCore;

#ifdef VECCORE_ENABLE_VC
using Backend = backend::VcVector;
#else
using Backend = backend::Basic;
#endif

static const Int_t N = (8 * 1024 * 1024);
static const UInt64_t K = Backend::VectorSize(Backend::Float_v());

// solve ax2 + bx + c = 0, return number of roots found

template <typename T>
Int_t QuadSolve(T a, T b, T c, T &x1, T &x2) {
  T delta = b * b - 4.0 * a * c;

  if (delta < 0.0)
    return 0;

  if (delta < NumericLimits<T>::Epsilon()) {
    x1 = x2 = -0.5 * b / a;
    return 1;
  }

  if (b >= 0.0) {
    x1 = -0.5 * (b + math::Sqrt(delta)) / a;
    x2 = c / (a * x1);
  } else {
    x2 = -0.5 * (b - math::Sqrt(delta)) / a;
    x1 = c / (a * x2);
  }

  return 2;
}

// SIMD version

// solve ax2 + bx + c = 0, return number of roots found
// solves many equation simultaneously, depending on value of Float_v::Size

template <class Backend>
void QuadSolveSIMD(typename Backend::Float_v const &a,
                   typename Backend::Float_v const &b,
                   typename Backend::Float_v const &c,
                   typename Backend::Float_v &x1,
                   typename Backend::Float_v &x2,
                   typename Backend::Int32_v &roots)
{
  using Float_v = typename Backend::Float_v;
  using Int32_v = typename Backend::Int32_v;
  using FMask = typename Backend::template Mask_v<Float_v>;
  using IMask = typename Backend::template Mask_v<Int32_v>;

  roots = 0;
  Float_v delta = b * b - Float_v(4.0) * a * c;

  FMask mask0(delta < Float_v(0.0));
  FMask mask2(delta >= NumericLimits<Float_v>::Epsilon());

  roots((IMask)mask2) = 2;

  FMask mask = (b >= Float_v(0.0));

  Float_v sign = Blend(mask, Float_v(-1.0), Float_v(1.0));
  Float_v root = (-b + sign * math::Sqrt(delta)) / (Float_v(2.0) * a);

  x1(mask2 && mask) = root;
  x2(mask2 && !mask) = root;

  root = c / (a * Blend(mask, x1, x2));

  x2(mask2 && mask) = root;
  x1(mask2 && !mask) = root;

  FMask mask1 = !(mask0 || mask2);

  if (IsEmpty(mask1))
    return;

  roots((IMask)mask1) = 1;

  root = -Float_v(0.5) * b / a;

  x1(mask1) = root;
  x2(mask1) = root;
}

int main(int argc, char *argv[]) {
  Float_t *a = (Float_t *)memalign(64, N * sizeof(Float_t));
  Float_t *b = (Float_t *)memalign(64, N * sizeof(Float_t));
  Float_t *c = (Float_t *)memalign(64, N * sizeof(Float_t));

  Int_t *roots = (Int_t *)memalign(64, N * sizeof(Int_t));
  Float_t *x1 = (Float_t *)memalign(64, N * sizeof(Float_t));
  Float_t *x2 = (Float_t *)memalign(64, N * sizeof(Float_t));

  srand48(time(NULL));
  Int_t index = (Int_t)((N - 100) * drand48());

  for (Int_t i = 0; i < N; i++) {
    a[i] = 10.0 * (drand48() - 0.5);
    b[i] = 10.0 * (drand48() - 0.5);
    c[i] = 50.0 * (drand48() - 0.5);
    x1[i] = 0.0;
    x2[i] = 0.0;
    roots[i] = 0;
  }

  Timer<milliseconds> timer;

  for (Int_t i = 0; i < N; i++) {
    roots[i] = QuadSolve(a[i], b[i], c[i], x1[i], x2[i]);
  }

  Double_t t = timer.Elapsed();

  // print random result to ensure scalar and vector backends give same result

  for (Int_t i = index; i < index + 10; i++) {
    printf("%d: a = % 8.3f, b = % 8.3f, c = % 8.3f,"
           " roots = %d, x1 = % 8.3f, x2 = % 8.3f\n",
            i, a[i], b[i], c[i], roots[i],
            roots[i] > 0 ? x1[i] : 0,
            roots[i] > 1 ? x2[i] : 0);
  }

  printf("\nelapsed time = %.3lfms (scalar code)\n", t);

  timer.Start();

  for (Int_t i = 0; i < N; i += Int_t(K)) {
    QuadSolveSIMD<Backend>(
      (Backend::Float_v &)(a[i]),
      (Backend::Float_v &)(b[i]),
      (Backend::Float_v &)(c[i]),
      (Backend::Float_v &)(x1[i]),
      (Backend::Float_v &)(x2[i]),
      (Backend::Int32_v &)(roots[i])
    );
  }

  t = timer.Elapsed();

  for (Int_t i = index; i < index + 10; i++) {
    printf("%d: a = % 8.3f, b = % 8.3f, c = % 8.3f,"
           " roots = %d, x1 = % 8.3f, x2 = % 8.3f\n",
            i, a[i], b[i], c[i], roots[i],
            roots[i] > 0 ? x1[i] : 0,
            roots[i] > 1 ? x2[i] : 0);
  }

  printf("\nelapsed time = %.3lfms (vector backend)\n", t);

  return 0;
}
