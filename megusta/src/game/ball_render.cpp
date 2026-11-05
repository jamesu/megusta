#include "ball.h"
#include <cmath>
namespace megusta {
namespace {
struct Quaternion { float x,y,z,w; };
long double sqrtFloat(float n) { return std::sqrt(static_cast<long double>(n)); }
void axisAngleToQuaternion(Quaternion& q,const float* axisAngle) {
    float half=axisAngle[3]*0.5;
    float sine=static_cast<float>(std::sin(static_cast<long double>(half)));
    long double cosine=std::cos(static_cast<long double>(half));
    q.x=sine*axisAngle[0]; q.y=sine*axisAngle[1]; q.z=sine*axisAngle[2]; q.w=static_cast<float>(cosine);
}
void quaternionToPackedMat3(const Quaternion& q,float* out) {
    if (std::fabs(q.x)<1e-6 && std::fabs(q.y)<1e-6 && std::fabs(q.z)<1e-6) {
        out[7]=out[5]=out[6]=out[2]=out[4]=out[1]=0; out[8]=out[3]=out[0]=1; return;
    }
    float x=q.x+q.x,y=q.y+q.y,z=q.z+q.z;
    float xw=x*q.w,yw=y*q.w,zw=z*q.w;
    float xx=x*q.x,xy=y*q.x,xz=z*q.x,yy=y*q.y,yz=z*q.y,zz=z*q.z;
    out[0]=1.0-(zz+yy); out[1]=xy-zw; out[4]=xz+yw;
    out[2]=xy+zw; out[3]=1.0-(zz+xx); out[5]=yz-xw;
    out[6]=xz-yw; out[7]=yz+xw; out[8]=1.0-(yy+xx);
}
void multiplyPackedMat3(const float* a,const float* b,float* out) {
    out[0]=b[6]*a[4]+a[1]*b[2]+a[0]*b[0];
    out[1]=b[1]*a[0]+b[7]*a[4]+a[1]*b[3];
    out[4]=b[4]*a[0]+b[8]*a[4]+a[1]*b[5];
    out[2]=a[5]*b[6]+a[2]*b[0]+a[3]*b[2];
    out[3]=a[2]*b[1]+a[3]*b[3]+a[5]*b[7];
    out[5]=a[2]*b[4]+a[3]*b[5]+a[5]*b[8];
    out[6]=a[7]*b[2]+b[6]*a[8]+a[6]*b[0];
    out[7]=b[1]*a[6]+b[3]*a[7]+b[7]*a[8];
    out[8]=b[4]*a[6]+b[5]*a[7]+b[8]*a[8];
}
}
void Ball::updateRollingOrientation() {
    Quaternion axis{static_cast<float>(0.0-velocity[1]),static_cast<float>(velocity[0]-0.0),0,0};
    long double length=sqrtFloat(axis.x*axis.x+axis.y*axis.y+0.0);
    if (length!=0.0L) {
        long double inverse=1.0L/length;
        axis.x=static_cast<float>(inverse*axis.x); axis.y=static_cast<float>(inverse*axis.y); axis.z=static_cast<float>(inverse*axis.z);
    }
    long double speed=sqrtFloat(velocity[2]*velocity[2]+velocity[1]*velocity[1]+velocity[0]*velocity[0]);
    if (speed>0.25L) speed=0.25L;
    float axisAngle[4]={axis.x,axis.y,axis.z,static_cast<float>((1.0L/(0.5L+static_cast<long double>(0.1)))*speed)};
    Quaternion q; axisAngleToQuaternion(q,axisAngle);
    float rotation[9], old[9]; quaternionToPackedMat3(q,rotation);
    for (int i=0;i<9;++i) old[i]=rollingRotation[i];
    multiplyPackedMat3(rotation,old,rollingRotation);
}
void Ball::updateTrail() {
    sgMat4 matrix; sgMakeTransMat4(matrix,position[0],position[1],position[2]);
    matrix[0][0]=rollingRotation[0]; matrix[0][1]=rollingRotation[2]; matrix[0][2]=rollingRotation[6];
    matrix[1][0]=rollingRotation[1]; matrix[1][1]=rollingRotation[3]; matrix[1][2]=rollingRotation[7];
    matrix[2][0]=rollingRotation[4]; matrix[2][1]=rollingRotation[5]; matrix[2][2]=rollingRotation[8];
    matrix[0][3]=matrix[1][3]=matrix[2][3]=0;
    if (!(flags&10u)) {
        for (auto& slot:trail) slot.advance();
        trailHead=(trailHead+1)%4; trail[trailHead].start(matrix);
        float previous[3]={position[0],position[1],position[2]};
        for (auto& slot:trail) if (slot.transform && slot.active && slot.frame==1) {
            previous[0]=slot.position[0]; previous[1]=slot.position[1]; previous[2]=slot.position[2]; break;
        }
        float dx=position[0]-previous[0], dy=position[1]-previous[1], dz=position[2]-previous[2];
        float distance=static_cast<float>(sqrtFloat(dx*dx+dy*dy+dz*dz));
        if (distance<24.0f) {
            dx=previous[0]-position[0]; dy=previous[1]-position[1]; dz=previous[2]-position[2];
            if (distance>2.0f) {
                long double length=sqrtFloat(dx*dx+dz*dz+dy*dy);
                if (length!=0.0L) { long double factor=2.0L/length; dx=static_cast<float>(factor*dx); dy=static_cast<float>(factor*dy); dz=static_cast<float>(factor*dz); }
            }
            for (auto& slot:trail) if (slot.transform && slot.active && slot.frame>0) {
                slot.transform->getTransform(matrix);
                float factor=static_cast<float>(slot.frame)*0.33333334f;
                matrix[3][0]=factor*dx+position[0]; matrix[3][1]=factor*dy+position[1]; matrix[3][2]=dz*factor+position[2];
                slot.transform->setTransform(matrix);
            }
            return;
        }
    }
    resetTrail();
}
}
