// SPDX-License-Identifier: GPL-2.0

#pragma once

namespace megusta { namespace dts {
struct DTSArbitraryScale;

// Shared 4x4 operations. Multiplication keeps the recovered engine term order
// and supports output aliasing either input.
void multiplyMatrix4x4(const float* left,const float* right,float* output);
long double affineLinearDeterminant(const float* matrix);
void invertAffineMatrix(const float* source,float* output);
void invertAffineMatrixInPlace(float* matrix);
void transformPoint3(const float* matrix,const float* point,float* output);
void transformVector3(const float* matrix,const float* vector,float* output);
float* initializeEulerMatrix(float* matrix,const float* angles);
long double matrixAtan2X87(float y,float x);
void applyAlignedScale(const float* scale,float* matrix);
void applyArbitraryScale(const DTSArbitraryScale* scale,float* matrix);

inline void transposeMatrix4(float* matrix) {
    const auto swap=[&](int a,int b) { const float value=matrix[a]; matrix[a]=matrix[b]; matrix[b]=value; };
    swap(1,4); swap(2,8); swap(3,12); swap(6,9); swap(7,13); swap(11,14);
}
inline void applyUniformScale(float scale,float* matrix) {
    const float factors[3]={scale,scale,scale};
    applyAlignedScale(factors,matrix);
}
inline void getMatrixColumn4(const float* matrix,int column,float* output) {
    output[0]=matrix[column]; output[1]=matrix[column+4];
    output[2]=matrix[column+8]; output[3]=matrix[column+12];
}
inline void getMatrixColumn3(const float* matrix,int column,float* output) {
    output[0]=matrix[column]; output[1]=matrix[column+4]; output[2]=matrix[column+8];
}
inline void setMatrixColumn3(float* matrix,int column,const float* value) {
    matrix[column]=value[0]; matrix[column+4]=value[1]; matrix[column+8]=value[2];
}
inline void setMatrixRow4(float* matrix,int row,const float* value) {
    const int first=row*4;
    matrix[first]=value[0]; matrix[first+1]=value[1];
    matrix[first+2]=value[2]; matrix[first+3]=value[3];
}
} }
