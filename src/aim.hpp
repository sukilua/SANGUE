#pragma once
#include "model.hpp"
namespace sangue {
struct MouseStep{int x{},y{};uint64_t target{};};
class Stabilizer{
    uint64_t locked{};Clock::time_point lastSample{},lastUpdate{};std::string state;
    float carryX{},carryY{};bool centered{};int lastBone{-1};float lastStrength{};bool lastInvertX{},lastInvertY{};
public:
    void reset(){locked=0;lastSample={};lastUpdate={};state.clear();carryX=carryY=0;centered=false;lastBone=-1;}
    void rejected(){lastSample={};lastUpdate={};carryX=carryY=0;state="Entrada recusada pelo Windows; nova tentativa permitida";}
    const std::string& status()const{return state;}
    MouseStep update(const Frame& f,const Settings& s,Clock::time_point now){
        if(aimState(f,s,true,now)!="Pronto para entrada"){reset();return {};}
        if(lastBone!=s.bone||lastStrength!=s.strength||lastInvertX!=s.invertX||lastInvertY!=s.invertY){reset();lastBone=s.bone;lastStrength=s.strength;lastInvertX=s.invertX;lastInvertY=s.invertY;}
        if(lastSample==f.stamp){state="Acompanhamento ativo / aguardando amostra nova";return {0,0,locked};}lastSample=f.stamp;
        const float radius=s.radius*f.view.height/1080.f;float best=radius*radius;std::optional<V2> point;uint64_t next=0;
        for(const auto& pose:f.poses)if(s.bone>=0&&s.bone<Count&&pose.points[s.bone]){auto p=f.view.project(*pose.points[s.bone]);if(!p)continue;float x=p->x-f.view.width*.5f,y=p->y-f.view.height*.5f,d=x*x+y*y;
            if(pose.id==locked&&d<=radius*radius){point=p;next=pose.id;break;}if(d<best){best=d;point=p;next=pose.id;}}
        if(!point){reset();state="Aguardando alvo dentro do FOV";return {};}
        if(locked!=next){locked=next;carryX=carryY=0;centered=false;lastUpdate={};}
        MouseStep result{0,0,locked};
        const float normalized=1080.f/f.view.height;
        const float x=(point->x-f.view.width*.5f)*normalized,y=(point->y-f.view.height*.5f)*normalized;
        const float zone=centered?1.2f:.8f;
        if(std::abs(x)<=zone&&std::abs(y)<=zone){centered=true;carryX=carryY=0;lastUpdate=now;state="Acompanhando alvo / centro alinhado";return result;}
        centered=false;
        const float dt=lastUpdate==Clock::time_point{}?.008f:std::clamp(std::chrono::duration<float>(now-lastUpdate).count(),.0001f,.025f);lastUpdate=now;
        const float fraction=1-std::exp(-18.f*dt),maximum=5000.f*dt*s.strength;
        auto axis=[&](float error,float& carry){if(std::abs(error)<=.8f){carry=0;return 0;}if(carry*error<0)carry=0;
            const float requested=std::clamp(error*fraction*s.strength,-maximum,maximum)+carry;
            const int step=std::clamp(int(std::round(requested)),-250,250);carry=requested-step;return step;};
        result.x=axis(x,carryX)*(s.invertX?-1:1);result.y=axis(y,carryY)*(s.invertY?-1:1);
        state="Acompanhamento contínuo ativo";return result;
    }
};
}
