#include "../src/dts/state_transpose.h"
#include <cassert>

int main() {
    // Two tracks, three frames, with unrelated entries at each end.
    int states[] = {90, 10, 20, 11, 21, 12, 22, 91};
    assert(megusta::dts::rearrangeStates(states, sizeof(states), 1, 3, 2, sizeof(int)));
    int expected[] = {90, 10, 11, 12, 20, 21, 22, 91};
    for (unsigned i=0;i<8;++i) assert(states[i]==expected[i]);
    assert(!megusta::dts::rearrangeStates(states, sizeof(states), 2, 4, 2, sizeof(int)));
    assert(megusta::dts::rearrangeStates(nullptr, 0, 0, 3, 0, sizeof(int)));
    int originalOrder[] = {1,2,3,4};
    megusta::dts::rearrangeStates(0,2,2,originalOrder,sizeof(int));
    assert(originalOrder[0]==1 && originalOrder[1]==3 && originalOrder[2]==2 && originalOrder[3]==4);
}
