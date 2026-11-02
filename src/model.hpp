#pragma once
#include <array>
#include <vector>
#include <optional>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <string>
#include <algorithm>
#include <unordered_set>
namespace sangue {
using Clock=std::chrono::steady_clock;
struct V3{float x{},y{},z{};};
struct V2{float x{},y{};};
inline bool finite(V3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct View {
    V3 position;std::array<float,9> axes{};float fov{};int width{},height{};std::array<int32_t,3> anchor{};
    bool valid()const{
        if(!finite(position)||!std::isfinite(fov)||fov<=1||fov>=179||width<1||height<1)return false;
        for(int a=0;a<3;++a){float n=0;for(int b=0;b<3;++b){float v=axes[a*3+b];if(!std::isfinite(v))return false;n+=v*v;}if(n<.5f||n>1.5f)return false;}
        for(int a=0;a<3;++a)for(int b=a+1;b<3;++b){float d=0;for(int i=0;i<3;++i)d+=axes[a*3+i]*axes[b*3+i];if(std::abs(d)>.25f)return false;}return true;
    }
    std::optional<V2> project(V3 world)const{
        if(!valid()||!finite(world))return {};
        float d[3]={world.x-position.x,world.y-position.y,world.z-position.z},v[3]{};
        for(int a=0;a<3;++a)for(int b=0;b<3;++b)v[a]+=d[b]*axes[b*3+a];
        if(v[2]>=-.01f)return {};float k=height/(2*std::tan(fov*.00872664626f)*-v[2]);
        V2 p{width*.5f+v[0]*k,height*.5f-v[1]*k};if(!std::isfinite(p.x)||!std::isfinite(p.y))return {};return p;
    }
};
enum Bone{Head,Neck,Chest,Spine,Pelvis,UpperL,LowerL,HandL,UpperR,LowerR,HandR,ThighL,CalfL,FootL,ThighR,CalfR,FootR,Count};
inline constexpr int links[][2]={{Head,Neck},{Neck,Chest},{Chest,Spine},{Spine,Pelvis},{Chest,UpperL},{UpperL,LowerL},{LowerL,HandL},{Chest,UpperR},{UpperR,LowerR},{LowerR,HandR},{Pelvis,ThighL},{ThighL,CalfL},{CalfL,FootL},{Pelvis,ThighR},{ThighR,CalfR},{CalfR,FootR}};
struct Pose{uint64_t id{};std::array<std::optional<V3>,Count> points;};
struct Frame{View view;std::vector<Pose> poses;Clock::time_point stamp{};uint64_t serial{};uint32_t pid{};int x{},y{};bool focused{};unsigned nodes{},rejected{},readErrors{};float readMs{};std::string status="Aguardando jogo",error,hash;};
struct Settings{bool rage{},skeleton{},unknownTargets{};float radius{180},thickness{1.5f};int bone{Head},key{2};};
inline bool fresh(const Frame& f,Clock::time_point now){auto age=now-f.stamp;return f.stamp!=Clock::time_point{}&&age>=Clock::duration::zero()&&age<std::chrono::milliseconds(25);}
inline std::vector<Pose> unique(std::vector<Pose> p){std::unordered_set<uint64_t> ids;p.erase(std::remove_if(p.begin(),p.end(),[&](const Pose& v){return !v.id||!ids.insert(v.id).second;}),p.end());return p;}
inline std::optional<V2> target(const Frame& f,const Settings& s){
    if(!s.unknownTargets||s.bone<0||s.bone>=Count)return {};float best=s.radius*s.radius;std::optional<V2> answer;
    for(const auto& p:f.poses)if(p.points[s.bone]){auto q=f.view.project(*p.points[s.bone]);if(q){float dx=q->x-f.view.width*.5f,dy=q->y-f.view.height*.5f,d=dx*dx+dy*dy;if(d<best){best=d;answer=q;}}}return answer;
}
}
