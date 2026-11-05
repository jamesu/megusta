#include "engine_math.h"
#include <cmath>

namespace megusta { float binding_math_sqrtf(float value); }

namespace megusta::engine_math {

void gameMath_storeVec3_adapter(float* output,float x,float y,float z) {
    output[0]=x; output[1]=y; output[2]=z;
}

long double gameMath_distanceVec3_impl(const float* first,const float* second) {
    const float x=first[0]-second[0];
    const float y=first[1]-second[1];
    const float z=first[2]-second[2];
    return megusta::binding_math_sqrtf(x*x+y*y+z*z);
}

void engineMath_normalizeVec3Unchecked_impl(float* vector) {
    const float length=megusta::binding_math_sqrtf(
        vector[2]*vector[2]+vector[1]*vector[1]+vector[0]*vector[0]);
    const long double reciprocal=1.0L/length;
    vector[0]=static_cast<float>(reciprocal*vector[0]);
    vector[1]=static_cast<float>(reciprocal*vector[1]);
    vector[2]=static_cast<float>(reciprocal*vector[2]);
}

void engineMatrix_identity_impl(float* matrix) {
    for(int i=0;i<16;++i) matrix[i]=0.0f;
    matrix[0]=matrix[5]=matrix[10]=matrix[15]=1.0f;
}

void engineMatrix_fromEulerAngles_impl(const float* angles,float* matrix) {
    if(angles[0]==0.0f && angles[1]==0.0f && angles[2]==0.0f) {
        engineMatrix_identity_impl(matrix);
        return;
    }
    const float sx=static_cast<float>(std::sin(static_cast<long double>(angles[0])));
    const float cx=static_cast<float>(std::cos(static_cast<long double>(angles[0])));
    const float sy=static_cast<float>(std::sin(static_cast<long double>(angles[1])));
    const float cy=static_cast<float>(std::cos(static_cast<long double>(angles[1])));
    const float sz=static_cast<float>(std::sin(static_cast<long double>(angles[2])));
    const float cz=static_cast<float>(std::cos(static_cast<long double>(angles[2])));
    matrix[0]=cz*cy-sz*sy*sx;
    matrix[1]=cz*sy*sx+sz*cy;
    matrix[2]=-(sy*cx);
    matrix[3]=0.0f;
    matrix[4]=-(sz*cx);
    matrix[5]=cz*cx;
    matrix[6]=0.0f;
    matrix[7]=0.0f;
    matrix[8]=sz*cy*sx+cz*sy;
    matrix[9]=sz*sy-cz*cy*sx;
    matrix[10]=cy*cx;
    matrix[11]=0.0f;
    matrix[12]=matrix[13]=matrix[14]=0.0f;
    matrix[15]=1.0f;
}

void engineMatrix_scaleRows3_impl(float* matrix,const float* scale) {
    for(int row=0;row<3;++row) {
        const int offset=row*4;
        matrix[offset+0]*=scale[0];
        matrix[offset+1]*=scale[1];
        matrix[offset+2]*=scale[2];
    }
    matrix[12]*=scale[0]; matrix[13]*=scale[1]; matrix[14]*=scale[2];
}

void engineMatrix_multiply4x4_impl(const float* first,const float* second,float* output) {
    float result[16];
    for(int row=0;row<4;++row) {
        for(int column=0;column<4;++column) {
            const int i=row*4;
            result[i+column]=first[i+0]*second[column+0] +
                             first[i+1]*second[column+4] +
                             first[i+2]*second[column+8] +
                             first[i+3]*second[column+12];
        }
    }
    for(int i=0;i<16;++i) output[i]=result[i];
}

void engineMatrix_transformVector3_impl(const float* matrix,const float* vector,float* output) {
    const float x=vector[0],y=vector[1],z=vector[2];
    output[0]=matrix[0]*x+matrix[1]*y+matrix[2]*z;
    output[1]=matrix[4]*x+matrix[5]*y+matrix[6]*z;
    output[2]=matrix[8]*x+matrix[9]*y+matrix[10]*z;
}

void engineMatrix_inverseAffine_impl(float* matrix) {
    const float a=matrix[0],b=matrix[1],c=matrix[2];
    const float d=matrix[4],e=matrix[5],f=matrix[6];
    const float g=matrix[8],h=matrix[9],i=matrix[10];
    const long double determinant=static_cast<long double>(a)*(e*i-f*h)-
                                  static_cast<long double>(b)*(d*i-f*g)+
                                  static_cast<long double>(c)*(d*h-e*g);
    const float reciprocal=static_cast<float>(1.0L/determinant);
    float inverse[12];
    inverse[0]=(e*i-f*h)*reciprocal;
    inverse[1]=(c*h-b*i)*reciprocal;
    inverse[2]=(b*f-c*e)*reciprocal;
    inverse[4]=(f*g-d*i)*reciprocal;
    inverse[5]=(a*i-c*g)*reciprocal;
    inverse[6]=(c*d-a*f)*reciprocal;
    inverse[8]=(d*h-e*g)*reciprocal;
    inverse[9]=(b*g-a*h)*reciprocal;
    inverse[10]=(a*e-b*d)*reciprocal;
    const float tx=matrix[3],ty=matrix[7],tz=matrix[11];
    inverse[3]=-(inverse[0]*tx+inverse[1]*ty+inverse[2]*tz);
    inverse[7]=-(inverse[4]*tx+inverse[5]*ty+inverse[6]*tz);
    inverse[11]=-(inverse[8]*tx+inverse[9]*ty+inverse[10]*tz);
    for(int row=0;row<3;++row)
        for(int column=0;column<4;++column)
            matrix[row*4+column]=inverse[row*4+column];
}

float* engineMatrix_ctorOptionalIdentity_impl(float* matrix,char initializeIdentity) {
    if(initializeIdentity) engineMatrix_identity_impl(matrix);
    return matrix;
}

void engineMatrix_setColumn4_impl(float* matrix,int column,const float* value) {
    matrix[column+0]=value[0]; matrix[column+4]=value[1];
    matrix[column+8]=value[2]; matrix[column+12]=value[3];
}

int engineMath_absInt_impl(int value) { return value<0 ? -value : value; }
int engineMath_maxInt_impl(int first,int second) { return first<=second ? second : first; }
long double engineMath_clampFloat_impl(float value,float minimum,float maximum) {
    if(value<minimum) return minimum;
    if(maximum<value) return maximum;
    return value;
}
long double engineMath_absFloat_impl(float value) { return std::fabs(static_cast<long double>(value)); }

long double engineMath_atanfX87_impl(float value) { return std::atan(static_cast<long double>(value)); }
long double engineMath_atan2_impl(float y,float x) {
    return std::atan2(static_cast<long double>(y),static_cast<long double>(x));
}
long double engineMath_sin_impl(float value) { return std::sin(static_cast<long double>(value)); }
long double engineMath_cos_impl(float value) { return std::cos(static_cast<long double>(value)); }
long double engineMath_tanfX87_impl(float value) { return std::tan(static_cast<long double>(value)); }
double engineMath_powX87_adapter(double base,double exponent) { return std::pow(base,exponent); }
long double engineMath_ceilFloat_adapter(float value) { return std::ceil(static_cast<long double>(value)); }
long double engineMath_floorFloat_adapter(float value) { return std::floor(static_cast<long double>(value)); }
double engineMath_frexp_adapter(double value,int* exponent) { return std::frexp(value,exponent); }
double engineMath_ldexp_adapter(double value,int exponent) { return std::ldexp(value,exponent); }
long double engineMath_expX87_impl(double value) { return std::exp(static_cast<long double>(value)); }
long double engineMath_logX87_impl(double value) {
    const double conversion=0.6931471805599453;
    return static_cast<long double>(conversion)*static_cast<long double>(value);
}
long double engineMath_log10X87_impl(double value) {
    const double conversion=0.3010299956639812;
    return static_cast<long double>(conversion)*static_cast<long double>(value);
}
double engineMath_modf_adapter(double value,double* integerPart) { return std::modf(value,integerPart); }
double engineMath_fmodX87_adapter(double value,double divisor) { return std::fmod(value,divisor); }
double engineMath_hypot_adapter(double x,double y) { return std::hypot(x,y); }

} // namespace megusta::engine_math
