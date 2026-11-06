#include "directsound_internal.h"
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <limits>
namespace megusta { namespace audio { namespace ds {
namespace { Handle nextHandleToken=1; }
HandleNode* createHandleList() {
    auto* node=static_cast<HandleNode*>(std::malloc(sizeof(HandleNode)));
    if (node) { node->owner=node; node->payload=nullptr; node->next=node; node->previous=node; node->token=0; }
    return node;
}
void destroyHandleList(HandleNode* head) {
    while (head->next!=head) { auto* node=head->next; head->next=node->next; std::free(node); }
    std::free(head);
}
void* nextHandle(HandleNode* head,HandleNode** cursor) {
    auto* node=*cursor ? *cursor : head;
    *cursor=node->next;
    return (*cursor)->payload;
}
HandleNode* insertHandle(HandleNode* head) {
    auto* node=static_cast<HandleNode*>(std::malloc(sizeof(HandleNode)));
    if (node) {
        node->payload=nullptr;node->owner=head;node->token=nextHandleToken++;
        if (!nextHandleToken) nextHandleToken=1;
        if (!node->token) node->token=nextHandleToken++;
        node->previous=head;node->next=head->next;
        node->next->previous=node;node->previous->next=node;
    }
    return node;
}
void removeHandle(HandleNode* head,HandleNode* node) {
    if (node && node->owner==head) {
        node->previous->next=node->next;node->next->previous=node->previous;
        node->owner=nullptr;std::free(node);
    }
}
Handle handleValue(HandleNode* node) {
    if (!node) return 0;
#if UINTPTR_MAX == 0xffffffffu
    return static_cast<Handle>(reinterpret_cast<std::uintptr_t>(node));
#else
    return node->token;
#endif
}
HandleNode* findHandle(HandleNode* head,Handle wanted) {
    if (!head || wanted==0 || wanted==0xffffffffu) return nullptr;
    for (auto* node=head->next;node!=head;node=node->next)
        if (handleValue(node)==wanted) return node;
    return nullptr;
}
bool containsHandle(HandleNode* head,Handle wanted) { return findHandle(head,wanted)!=nullptr; }
void* handlePayload(HandleNode* head,Handle wanted) {
    auto* node=findHandle(head,wanted);
    return node ? node->payload : nullptr;
}
void setHandlePayload(HandleNode* head,HandleNode* node,void* payload) {
    if (node && node->owner==head) node->payload=payload;
}
std::int64_t truncateX87(long double value) {
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__)) && LDBL_MANT_DIG == 64 && !defined(MEGUSTA_PORTABLE_NUMERIC)
    unsigned short saved,truncate;
    std::int64_t result;
    asm volatile("fnstcw %0":"=m"(saved));
    truncate=saved|0x0c00;
    asm volatile("fldt %1; fldcw %2; fistpq %0; fldcw %3"
        :"=m"(result):"m"(value),"m"(truncate),"m"(saved):"st");
    return result;
#else
    if (!std::isfinite(value) || value < -0x1p63L || value >= 0x1p63L)
        return std::numeric_limits<std::int64_t>::min();
    return static_cast<std::int64_t>(value);
#endif
}
static long double log10X87(long double value) {
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__)) && LDBL_MANT_DIG == 64 && !defined(MEGUSTA_PORTABLE_NUMERIC)
    long double result;
    asm volatile("fldlg2; fldt %1; fyl2x; fstpt %0":"=m"(result):"m"(value):"st");
    return result;
#else
    return std::log10(static_cast<long double>(value));
#endif
}
int gainToAttenuation(double gain) {
    if (gain>=1.0) return 0;
    if (gain<=1e-5) return -10000;
    return static_cast<std::int32_t>(truncateX87(log10X87(gain)*2000.0));
}
int panToUnits(double pan) {
    if (pan>=1) return 10000;
    if (pan<=-1) return -10000;
    if (pan==0) return 0;
    if (pan>=0) return static_cast<std::int32_t>(truncateX87(log10X87(1.0L-pan)*-2000.0));
    return static_cast<std::int32_t>(truncateX87(log10X87(1.0L+pan)*2000.0));
}
void crossProduct(const float* left,const float* right,float* output) {
    output[0]=right[2]*left[1]-left[2]*right[1];
    output[1]=left[2]*right[0]-left[0]*right[2];
    output[2]=left[0]*right[1]-right[0]*left[1];
}
} } }
