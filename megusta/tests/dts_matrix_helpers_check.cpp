#include "../src/dts/shape.h"
#include <cassert>
#include <cmath>
using namespace megusta::dts;
int main() {
    TSIntegerSet set;
    assert(!set.test(TSIntegerSet::capacity));
    set.set(TSIntegerSet::capacity-1); assert(set.test(TSIntegerSet::capacity-1));
    set.clear(TSIntegerSet::capacity-1); assert(!set.test(TSIntegerSet::capacity-1));
    float matrix[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    assert(affineLinearDeterminant(matrix)==1.0L);
    const float point[3]={2,3,4}; float output[4]={};
    transformPoint3(matrix,point,output);
    assert(output[0]==2 && output[1]==3 && output[2]==4);
    const float column[4]={5,6,7,8}; setMatrixColumn3(matrix,1,column);
    getMatrixColumn3(matrix,1,output);
    assert(output[0]==5 && output[1]==6 && output[2]==7);
    getMatrixColumn4(matrix,1,output); assert(output[3]==0);
    const float row[4]={9,10,11,12}; setMatrixRow4(matrix,2,row);
    assert(matrix[8]==9 && matrix[9]==10 && matrix[10]==11 && matrix[11]==12);
    float transposed[16]={1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    transposeMatrix4(transposed); assert(transposed[1]==5 && transposed[4]==2 && transposed[14]==12);
    float angles[3]={0,0,0}; initializeEulerMatrix(matrix,angles);
    assert(matrix[0]==1 && matrix[5]==1 && matrix[10]==1 && matrix[15]==1);
    assert(bitmapPowerOfTwoDimension(0)==0x80000000u);
    assert(bitmapPowerOfTwoDimension(1)==1u && bitmapPowerOfTwoDimension(3)==4u);
    assert(bitmapPowerOfTwoDimension(16)==16u);
}
