#include "runtime.hpp"
#include <iostream>
using namespace sangue;
template<size_t N,class T>void put(std::array<uint8_t,N>& buffer,size_t at,T value){std::memcpy(buffer.data()+at,&value,sizeof(value));}
int main(){
    void* reserve=VirtualAlloc(nullptr,0x8b7e000,MEM_RESERVE,PAGE_READWRITE);if(!reserve)return 1;
    const auto base=reinterpret_cast<uint64_t>(reserve);
    if(!VirtualAlloc(reinterpret_cast<void*>(base+0x8b7c000),0x1000,MEM_COMMIT,PAGE_READWRITE)){VirtualFree(reserve,0,MEM_RELEASE);return 2;}
    int result=0;
    try{
        std::array<uint8_t,16> manager{};std::array<uint8_t,8> sentinel{};std::array<uint8_t,32> first{},second{};
        std::array<uint8_t,0xf0> component{};std::array<uint8_t,0x1c0> entity{};std::array<uint8_t,0xe0> skeleton{};std::array<uint8_t,32> pose{};
        std::array<uint8_t,6*0xa0> records{};std::array<std::array<uint8_t,96>,5> descriptors{};std::array<std::array<char,96>,5> names{};std::array<uint64_t,5> interns{};
        auto address=[](const auto& value){return reinterpret_cast<uint64_t>(value.data());};
        const auto root=address(manager);std::memcpy(reinterpret_cast<void*>(base+0x8b7cd90),&root,8);
        put(manager,8,address(sentinel));put(sentinel,0,address(first));put(first,0,address(second));put(second,0,address(sentinel));put(first,0x18,address(component));put(second,0x18,address(component));
        put(component,0x40,address(entity));put(component,0xe8,address(skeleton));put(skeleton,0xd8,address(pose));put(pose,0x10,address(records));put(pose,0x18,address(records)+records.size());put(entity,0x130,uint8_t(0x10));
        std::array<float,12> transform{1,0,0,0,1,0,0,0,1,0,0,-10};put(entity,0x58,transform);
        std::array<int32_t,3> origin{1000000000,1000000000,0};put(entity,0x1ac,origin);
        const char* labels[]={"Head","Neck","Chest","Spine1","Pelvis"};
        for(size_t i=0;i<5;++i){std::memcpy(names[i].data(),labels[i],std::strlen(labels[i])+1);interns[i]=reinterpret_cast<uint64_t>(names[i].data());put(descriptors[i],0x58,reinterpret_cast<uint64_t>(&interns[i]));put(records,i*0xa0+0x88,address(descriptors[i]));std::array<float,12> bone{1,0,0,0,1,0,0,0,1,0,float(5-i)*.2f,0};put(records,i*0xa0+0x30,bone);}
        // Record six intentionally has an invalid descriptor, while five useful bones remain valid.
        Memory self(GetCurrentProcessId());Frame f;f.view.anchor=origin;
        auto run=[&](Frame& frame){return sample(self,base,frame,Clock::now()+std::chrono::seconds(5));};
        auto actors=run(f);
        if(actors.size()!=1||!actors[0].points[Head]||f.readErrors!=2||f.nodes!=2)throw std::runtime_error("Bad descriptor discarded valid bones or duplicated an actor");
        if(std::abs(actors[0].points[Head]->x)>.001f||std::abs(actors[0].points[Head]->z+10)>.001f)throw std::runtime_error("Local/world composition failed");
        Frame moved;moved.view.anchor={999999900,1000000000,0};actors=run(moved);
        if(actors.size()!=1||std::abs(actors[0].points[Head]->x-100)>.001f)throw std::runtime_error("Large anchor delta lost precision");
        put(records,5*0xa0+0x88,address(descriptors[0]));Frame ambiguous;ambiguous.view.anchor=origin;actors=run(ambiguous);
        if(!actors.empty()||ambiguous.ambiguousBones!=2)throw std::runtime_error("Ambiguous semantic bone was selected arbitrarily");
        Frame expired;actors=sample(self,base,expired,Clock::now()-std::chrono::seconds(1));
        if(!actors.empty()||!expired.budgetExpired)throw std::runtime_error("Expired scan did not stop");
        std::cout<<"Partial bone failure, duplicate IDs, transforms, large anchors, ambiguous slots and scan deadline passed\n";
    }catch(const std::exception& e){std::cerr<<e.what();result=3;}
    VirtualFree(reserve,0,MEM_RELEASE);return result;
}
