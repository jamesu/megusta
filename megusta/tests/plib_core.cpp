// Integration/link checks for the evidenced SG/UL provider. These are expected
// value checks, not a comparison with the original executable's FPU behavior.
#include "sg.h"
#include <cstdio>
#include <cstring>
#include <cmath>

static int failures=0,warnings=0;
static void check(bool condition,const char* name) {
    if(!condition) { std::fprintf(stderr,"FAIL %s\n",name); ++failures; }
}
static bool approximately(float a,float b) {return std::fabs(double(a)-b)<1e-5;}
static void warning(ulSeverity severity,char* message) {
    if(severity==UL_WARNING && message && *message) ++warnings;
}
int main() {
    sgVec3 x={1,0,0},y={0,1,0},z;
    sgVectorProductVec3(z,x,y);check(z[0]==0 && z[1]==0 && z[2]==1,"cross product");
    sgVec3 a={0,0,0},b={1,0,0},c={0,1,0};
    sgMakeNormal(z,a,b,c);check(approximately(z[2],1),"normal");
    sgMat4 rotation,translation,product,pre,post,inverse;
    sgVec3 axis={0,0,1},offset={2,3,4};
    sgMakeRotMat4(rotation,90,axis);check(approximately(rotation[0][1],1),"axis angle");
    sgMakeTransMat4(translation,offset);check(translation[3][2]==4,"vector translation");
    sgMakeTransMat4(pre,2,3,4);check(pre[3][1]==3,"scalar translation");
    sgMultMat4(product,translation,rotation);
    check(approximately(product[3][0],2) && approximately(product[3][1],3) && approximately(product[0][1],1),"matrix product expected value");
    // PLIB pre/post naming follows its original operand convention.
    sgCopyMat4(post,rotation);sgPostMultMat4(post,translation);
    sgCopyMat4(pre,translation);sgPreMultMat4(pre,rotation);
    for(int i=0;i<4;++i)for(int j=0;j<4;++j) {
        check(approximately(product[i][j],post[i][j]),"postmultiply");
        check(approximately(product[i][j],pre[i][j]),"premultiply");
    }
    sgTransposeNegateMat4(inverse,translation);
    check(inverse[3][0]==-2 && inverse[3][1]==-3 && inverse[3][2]==-4,"rigid inverse");
    sgMakeCoordMat4(product,2,3,4,0,0,0);
    check(product[3][0]==2 && product[0][0]==1,"coordinate matrix");
    sgVec3 eye={2,3,4},center={2,4,4},up={0,0,1};
    sgMakeLookAtMat4(product,eye,center,up);
    check(product[3][0]==2 && product[3][1]==3 && product[3][2]==4,"look-at origin");
    sgBox box,other;sgVec3 lo={-2,-3,-4},hi={2,3,4};
    box.extend(lo);box.extend(hi);other.extend(&box);
    check(other.getMin()[2]==-4 && other.getMax()[1]==3,"box point/box extension");
    sgSphere sphere;sphere.setCenter(0,0,0);sphere.setRadius(1);
    other.empty();other.extend(&sphere);
    check(other.getMin()[0]==-1 && other.getMax()[2]==1,"box sphere extension");
    sgSphere points;points.extend(lo);points.extend(hi);
    check(points.getRadius()>5,"sphere point extension");
    sgSphere enclosing;enclosing.extend(&box);
    check(enclosing.getRadius()>5,"sphere box extension");
    sphere.setCenter(20,0,0);enclosing.extend(&sphere);
    check(enclosing.getRadius()>10,"sphere sphere extension");
    sgFrustum frustum;frustum.setNearFar(1,100);frustum.setFOV(90,90);
    sphere.setCenter(0,0,-5);sphere.setRadius(0.1f);
    check(frustum.contains(&sphere)==SG_INSIDE,"inside frustum");
    sphere.setCenter(0,0,5);check(frustum.contains(&sphere)==SG_OUTSIDE,"outside frustum");
    ulSetErrorCallback(warning);frustum.setNearFar(1,1.05f);
    // MG-PLIB-027: the executable writes this warning to stderr directly.
    check(warnings==0,"frustum warning bypasses UL callback");
    ulClearError();check(!*ulGetError(),"clear warning");ulSetErrorCallback(nullptr);
    check(ulStrEqual("Model.ASX","model.asx"),"ASCII name comparison");
    check(ulStrEqual(nullptr,"") && !ulStrEqual("a/b","a\\b"),"name comparison edges");
    std::printf("PLIB SG/UL integration: %d failures; pointer=%u\n",failures,unsigned(sizeof(void*)));
    return failures?1:0;
}
