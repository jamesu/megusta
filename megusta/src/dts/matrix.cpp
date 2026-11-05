#include "matrix.h"
#include "shape.h"
#include <cmath>
#include <cstring>

namespace megusta { namespace dts {
namespace {
float x87Sin(float angle) {
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__)) && !defined(MEGUSTA_PORTABLE_NUMERIC)
    const long double input=angle;
    long double result;
    __asm__ volatile("fldt %1\n\tfsin\n\tfstpt %0" : "=m"(result) : "m"(input) : "st");
    return static_cast<float>(result);
#else
    return static_cast<float>(std::sin(static_cast<long double>(angle)));
#endif
}
float x87Cos(float angle) {
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__)) && !defined(MEGUSTA_PORTABLE_NUMERIC)
    const long double input=angle;
    long double result;
    __asm__ volatile("fldt %1\n\tfcos\n\tfstpt %0" : "=m"(result) : "m"(input) : "st");
    return static_cast<float>(result);
#else
    return static_cast<float>(std::cos(static_cast<long double>(angle)));
#endif
}
}

void multiplyMatrix4x4(const float* left,const float* right,float* output) {
    float r[16];
    r[0]=left[3]*right[12]+left[1]*right[4]+right[8]*left[2]+left[0]*right[0];
    r[1]=right[13]*left[3]+left[1]*right[5]+left[0]*right[1]+right[9]*left[2];
    r[2]=right[14]*left[3]+left[1]*right[6]+right[10]*left[2]+left[0]*right[2];
    r[3]=right[11]*left[2]+right[3]*left[0]+left[1]*right[7]+left[3]*right[15];
    r[4]=left[4]*right[0]+right[8]*left[6]+left[7]*right[12]+left[5]*right[4];
    r[5]=right[9]*left[6]+right[13]*left[7]+left[4]*right[1]+right[5]*left[5];
    r[6]=left[4]*right[2]+right[10]*left[6]+right[14]*left[7]+left[5]*right[6];
    r[7]=left[5]*right[7]+left[6]*right[11]+left[7]*right[15]+left[4]*right[3];
    r[8]=left[8]*right[0]+right[8]*left[10]+left[11]*right[12]+left[9]*right[4];
    r[9]=right[9]*left[10]+right[13]*left[11]+left[8]*right[1]+right[5]*left[9];
    r[10]=left[8]*right[2]+right[10]*left[10]+right[14]*left[11]+left[9]*right[6];
    r[11]=left[9]*right[7]+left[10]*right[11]+left[11]*right[15]+left[8]*right[3];
    r[12]=left[12]*right[0]+right[8]*left[14]+left[15]*right[12]+left[13]*right[4];
    r[13]=right[9]*left[14]+right[13]*left[15]+left[12]*right[1]+right[5]*left[13];
    r[14]=left[12]*right[2]+right[10]*left[14]+right[14]*left[15]+left[13]*right[6];
    r[15]=left[13]*right[7]+left[14]*right[11]+left[15]*right[15]+left[12]*right[3];
    std::memcpy(output,r,sizeof(r));
}

long double affineLinearDeterminant(const float* m) {
    return (static_cast<long double>(m[10])*m[5]-static_cast<long double>(m[9])*m[6])*m[0]+
        (static_cast<long double>(m[6])*m[1]-static_cast<long double>(m[2])*m[5])*m[8]+
        (static_cast<long double>(m[2])*m[9]-static_cast<long double>(m[10])*m[1])*m[4];
}

void invertAffineMatrix(const float* source,float* output) {
    const float a=source[0],b=source[1],c=source[2];
    const float d=source[4],e=source[5],f=source[6];
    const float g=source[8],h=source[9],i=source[10];
    const float reciprocal=static_cast<float>(1.0L/affineLinearDeterminant(source));
    for (int index=0;index<16;++index) output[index]=0.0f;
    output[0]=(e*i-f*h)*reciprocal; output[1]=(c*h-b*i)*reciprocal; output[2]=(b*f-c*e)*reciprocal;
    output[4]=(f*g-d*i)*reciprocal; output[5]=(a*i-c*g)*reciprocal; output[6]=(c*d-a*f)*reciprocal;
    output[8]=(d*h-e*g)*reciprocal; output[9]=(b*g-a*h)*reciprocal; output[10]=(a*e-b*d)*reciprocal;
    output[15]=1.0f;
    const float tx=-source[3],ty=-source[7],tz=-source[11];
    output[3]=output[0]*tx+output[1]*ty+output[2]*tz;
    output[7]=output[4]*tx+output[5]*ty+output[6]*tz;
    output[11]=output[8]*tx+output[9]*ty+output[10]*tz;
}

void invertAffineMatrixInPlace(float* matrix) {
    const float bottom[4]={matrix[12],matrix[13],matrix[14],matrix[15]};
    float inverse[16];
    invertAffineMatrix(matrix,inverse);
    std::memcpy(matrix,inverse,12*sizeof(float));
    std::memcpy(matrix+12,bottom,sizeof(bottom));
}

void transformPoint3(const float* matrix,const float* point,float* output) {
    output[0]=matrix[0]*point[0]+matrix[1]*point[1]+matrix[2]*point[2]+matrix[3];
    output[1]=matrix[5]*point[1]+matrix[4]*point[0]+matrix[6]*point[2]+matrix[7];
    output[2]=matrix[9]*point[1]+matrix[8]*point[0]+matrix[10]*point[2]+matrix[11];
}

void transformVector3(const float* matrix,const float* vector,float* output) {
    output[0]=matrix[0]*vector[0]+matrix[4]*vector[1]+matrix[8]*vector[2];
    // The middle term follows the saved TSLastDetail_chooseView operand order.
    output[1]=matrix[1]*vector[0]+vector[1]*matrix[5]+matrix[9]*vector[2];
    output[2]=matrix[2]*vector[0]+matrix[6]*vector[1]+matrix[10]*vector[2];
}

float* DTSQuatF::setMatrix(float* matrix) const {
    for (int index=0;index<16;++index) matrix[index]=(index%5==0) ? 1.0f : 0.0f;
    if (std::fabs(x)<0.0001f && std::fabs(y)<0.0001f && std::fabs(z)<0.0001f) return matrix;
    const float xx2=(x+x)*x,xy2=(y+y)*x,xz2=(z+z)*x;
    const float yy2=(y+y)*y,yz2=(z+z)*y,zz2=(z+z)*z;
    const float xw2=(x+x)*w,yw2=(y+y)*w,zw2=(z+z)*w;
    matrix[0]=1.0f-(zz2+yy2); matrix[4]=xy2-zw2; matrix[8]=xz2+yw2;
    matrix[1]=xy2+zw2; matrix[5]=1.0f-(zz2+xx2); matrix[9]=yz2-xw2;
    matrix[2]=xz2-yw2; matrix[6]=yz2+xw2; matrix[10]=1.0f-(yy2+xx2);
    return matrix;
}

void applyAlignedScale(const float* scale,float* matrix) {
    for (int row=0;row<4;++row)
        for (int column=0;column<3;++column) matrix[row*4+column]*=scale[column];
}

void applyArbitraryScale(const DTSArbitraryScale* scale,float* matrix) {
    float rotation[16],inverse[16],scaled[16],combined[16],input[16];
    scale->rotation.setMatrix(rotation);
    invertAffineMatrix(rotation,inverse);
    const float factors[3]={scale->factors.x,scale->factors.y,scale->factors.z};
    applyAlignedScale(factors,rotation);
    std::memcpy(scaled,rotation,sizeof(scaled));
    multiplyMatrix4x4(scaled,inverse,combined);
    std::memcpy(input,matrix,sizeof(input));
    multiplyMatrix4x4(input,combined,matrix);
}

float* initializeEulerMatrix(float* matrix,const float* angles) {
    const float x=angles[0],y=angles[1],z=angles[2];
    if (x==0.0f && y==0.0f && z==0.0f) {
        for (int i=0;i<16;++i) matrix[i]=(i%5==0) ? 1.0f : 0.0f;
        return matrix;
    }
    const float sx=x87Sin(x),cx=x87Cos(x),sy=x87Sin(y),cy=x87Cos(y),sz=x87Sin(z),cz=x87Cos(z);
    matrix[3]=matrix[7]=matrix[11]=matrix[12]=matrix[13]=matrix[14]=0.0f;
    matrix[0]=cz*cy-sz*sy*sx; matrix[1]=cz*sy*sx+sz*cy; matrix[2]=-sy*cx;
    matrix[4]=-sz*cx; matrix[5]=cz*cx; matrix[6]=sx;
    matrix[8]=sz*cy*sx+cz*sy; matrix[9]=sz*sy-cz*cy*sx; matrix[10]=cy*cx;
    matrix[15]=1.0f;
    return matrix;
}

long double matrixAtan2X87(float y,float x) {
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__)) && !defined(MEGUSTA_PORTABLE_NUMERIC)
    long double result;
    __asm__ volatile("flds %1\n\tflds %2\n\tfpatan\n\tfstpt %0"
                     : "=m"(result) : "m"(y), "m"(x) : "st");
    return result;
#else
    return std::atan2(static_cast<long double>(y),static_cast<long double>(x));
#endif
}
} }

extern "C" void FUN_004d0130(float* matrix) {
    megusta::dts::transposeMatrix4(matrix);
}
extern "C" long double FUN_004d01f0(const float* matrix) {
    return megusta::dts::affineLinearDeterminant(matrix);
}
extern "C" void FUN_004d0450(const float* matrix,const float* point,float* output) {
    megusta::dts::transformPoint3(matrix,point,output);
}
extern "C" float* MEGUSTA_DTS_THISCALL FUN_004d0530(float* matrix,const float* angles) {
    return megusta::dts::initializeEulerMatrix(matrix,angles);
}
extern "C" void MEGUSTA_DTS_THISCALL FUN_004d0550(const float* matrix,int column,float* output) {
    megusta::dts::getMatrixColumn4(matrix,column,output);
}
extern "C" void MEGUSTA_DTS_THISCALL FUN_004d0580(const float* matrix,int column,float* output) {
    megusta::dts::getMatrixColumn3(matrix,column,output);
}
