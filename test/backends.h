TEST_BACKEND(Scalar);
TEST_BACKEND(ScalarWrapper);

#ifdef VECCORE_ENABLE_VC
TEST_BACKEND(VcScalar);
TEST_BACKEND(VcVector);
TEST_BACKEND_P(VcSimdArray4, VcSimdArray<4>);
TEST_BACKEND_P(VcSimdArray8, VcSimdArray<8>);
#endif

#ifdef VECCORE_ENABLE_UMESIMD
TEST_BACKEND(UMESimd);
TEST_BACKEND_P(UMESimdArray2, UMESimdArray<2>);
#ifdef __AVX2__
TEST_BACKEND_P(UMESimdArray4, UMESimdArray<4>);
TEST_BACKEND_P(UMESimdArray8, UMESimdArray<8>);
#endif
#endif

#ifdef VECCORE_ENABLE_STD_SIMD
TEST_BACKEND(SIMDScalar);
TEST_BACKEND(SIMDNative);
TEST_BACKEND_P(SIMDVector4, SIMDVector<4>);
TEST_BACKEND_P(SIMDVector8, SIMDVector<8>);
#endif
