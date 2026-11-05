#pragma once

namespace megusta::engine_math {

void gameMath_storeVec3_adapter(float* output,float x,float y,float z);
long double gameMath_distanceVec3_impl(const float* first,const float* second);
void engineMath_normalizeVec3Unchecked_impl(float* vector);

void engineMatrix_fromEulerAngles_impl(const float* angles,float* matrix);
void engineMatrix_identity_impl(float* matrix);
void engineMatrix_inverseAffine_impl(float* matrix);
void engineMatrix_scaleRows3_impl(float* matrix,const float* scale);
void engineMatrix_multiply4x4_impl(const float* first,const float* second,float* output);
void engineMatrix_transformVector3_impl(const float* matrix,const float* vector,float* output);
float* engineMatrix_ctorOptionalIdentity_impl(float* matrix,char initializeIdentity);
void engineMatrix_setColumn4_impl(float* matrix,int column,const float* value);

int engineMath_absInt_impl(int value);
int engineMath_maxInt_impl(int first,int second);
long double engineMath_clampFloat_impl(float value,float minimum,float maximum);
long double engineMath_absFloat_impl(float value);
long double engineMath_atanfX87_impl(float value);
long double engineMath_atan2_impl(float y,float x);
long double engineMath_sin_impl(float value);
long double engineMath_cos_impl(float value);
long double engineMath_tanfX87_impl(float value);
double engineMath_powX87_adapter(double base,double exponent);
long double engineMath_ceilFloat_adapter(float value);
long double engineMath_floorFloat_adapter(float value);
double engineMath_frexp_adapter(double value,int* exponent);
double engineMath_ldexp_adapter(double value,int exponent);
long double engineMath_expX87_impl(double value);
long double engineMath_logX87_impl(double value);
long double engineMath_log10X87_impl(double value);
double engineMath_modf_adapter(double value,double* integerPart);
double engineMath_fmodX87_adapter(double value,double divisor);
double engineMath_hypot_adapter(double x,double y);

} // namespace megusta::engine_math
