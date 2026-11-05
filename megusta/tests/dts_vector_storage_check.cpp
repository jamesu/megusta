#include "../src/dts/vector_storage.h"
#include <cassert>

int main() {
    using megusta::dts::DTSVectorStorage;
    DTSVectorStorage storage;
    assert(megusta::dts::DTS_Vector12_ctor(&storage,17)==&storage);
    assert(storage.count==0 && storage.capacity==32 && storage.data);
    megusta::dts::DTS_Vector4_incrementSize(&storage,5);
    assert(storage.count==5 && storage.capacity==32 && storage.data);
    megusta::dts::DTS_Vector4_incrementSize(&storage,28);
    assert(storage.count==33 && storage.capacity==48 && storage.data);
    megusta::dts::DTS_Vector4_setSize(&storage,3);
    assert(storage.count==3 && storage.capacity==16 && storage.data);
    assert(storage.resize(0,4));
    assert(storage.count==0 && storage.capacity==0 && storage.data==nullptr);
}
