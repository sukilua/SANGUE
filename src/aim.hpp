#pragma once
#include "model.hpp"
namespace sangue {
inline float yaw(const View& v){return std::atan2(-v.axes[2],v.axes[8])*57.2957795f;}
inline float pitch(const View& v){return std::asin(std::clamp(-v.axes[5],-1.f,1.f))*57.2957795f;}
inline float degrees(float angle){return std::remainder(angle,360.f);}
inline bool validSensitivity(float value){return std::isfinite(value)&&std::abs(value)>=.0001f&&std::abs(value)<=2.f;}
struct MouseStep{int x{},y{};uint64_t target{};};
class Stabilizer{
    uint64_t locked{};bool observed{};float lastErrorX{},lastErrorY{};std::array<float,9> lastAxes{};float carryX{},carryY{};bool centered{};int lastBone{-1};float lastX{},lastY{};
public:
    void reset(){locked=0;observed=false;carryX=carryY=0;centered=false;lastBone=-1;}
    MouseStep update(const Frame& f,const Settings& s,Clock::time_point now){
        if(!s.rage||!s.unknownTargets||!f.focused||!s.calibrated||!validSensitivity(s.degreesX)||!validSensitivity(s.degreesY)||!fresh(f,now)||!f.view.valid()){reset();return {};}
        if(lastBone!=s.bone||lastX!=s.degreesX||lastY!=s.degreesY){reset();lastBone=s.bone;lastX=s.degreesX;lastY=s.degreesY;}
        const float focal=f.view.height/(2*std::tan(f.view.fov*.00872664626f));
        const float radius=s.radius*f.view.height/1080.f;float best=radius*radius;std::optional<V2> point;uint64_t next=0;
        for(const auto& pose:f.poses)if(s.bone>=0&&s.bone<Count&&pose.points[s.bone]){auto p=f.view.project(*pose.points[s.bone]);if(!p)continue;float x=p->x-f.view.width*.5f,y=p->y-f.view.height*.5f,d=x*x+y*y;
            if(pose.id==locked&&d<=radius*radius){point=p;next=pose.id;break;}if(d<best){best=d;point=p;next=pose.id;}}
        if(!point){reset();return {};}
        if(locked!=next){locked=next;carryX=carryY=0;centered=false;observed=false;}
        MouseStep result{0,0,locked};
        const float x=std::atan((point->x-f.view.width*.5f)/focal)*57.2957795f,y=-std::atan((point->y-f.view.height*.5f)/focal)*57.2957795f;
        const float zoneX=std::max(.025f,std::abs(s.degreesX)*.55f),zoneY=std::max(.025f,std::abs(s.degreesY)*.55f);
        if(std::abs(x)<=zoneX*(centered?1.8f:1.f)&&std::abs(y)<=zoneY*(centered?1.8f:1.f)){centered=true;carryX=carryY=0;return result;}centered=false;
        if(observed){float change=0;for(size_t i=0;i<9;++i)change+=std::abs(f.view.axes[i]-lastAxes[i]);
            if(change<.00001f&&std::abs(x-lastErrorX)<.02f&&std::abs(y-lastErrorY)<.02f)return result;}
        constexpr float fraction=.7f;
        auto axis=[&](float angle,float gain,float zone,float& carry){if(std::abs(angle)<=zone){carry=0;return 0;}float error=angle/gain;if(carry*error<0)carry=0;
            float requested=std::clamp(angle*fraction,-3.f,3.f)/gain+carry;int step=int(std::round(requested));const int limit=int(std::floor(std::abs(error)));step=std::clamp(step,-limit,limit);carry=requested-step;return std::clamp(step,-2000,2000);};
        result.x=axis(x,s.degreesX,zoneX,carryX);result.y=axis(y,s.degreesY,zoneY,carryY);
        if(result.x||result.y){observed=true;lastAxes=f.view.axes;lastErrorX=x;lastErrorY=y;}return result;
    }
};
struct CalibrationTick{MouseStep move;bool save{},busy{};float x{},y{},fov{};std::string message;};
class Calibration{
    int stage{};Clock::time_point due{};View before;float x{},y{},referenceFov{};bool failed{};
public:
    bool busy()const{return stage!=0;}
    void start(Clock::time_point now){stage=1;due=now+std::chrono::seconds(3);failed=false;}
    void cancel(){stage=0;}
    CalibrationTick tick(const View& view,bool focused,Clock::time_point now){
        CalibrationTick out;out.busy=busy();if(!out.busy)return out;
        if(stage==1){out.message="Calibração: volte ao jogo e não mova o mouse";if(now<due)return out;if(!focused||!view.valid()){if(now>due+std::chrono::seconds(7)){stage=0;out.busy=false;out.message="Calibração cancelada: jogo sem foco";}return out;}before=view;referenceFov=view.fov;stage=2;due=now+std::chrono::milliseconds(220);out.move.x=16;return out;}
        if(!focused||!view.valid()){stage=0;out.busy=false;out.message="Calibração cancelada ao perder o foco";return out;}
        out.message="Medindo sensibilidade: não mova o mouse";if(now<due)return out;
        if(stage==2){x=degrees(yaw(view)-yaw(before))/16;failed=!validSensitivity(x);out.move.x=-16;stage=3;due=now+std::chrono::milliseconds(220);}
        else if(stage==3){if(failed){stage=0;out.busy=false;out.message="Calibração X falhou: movimento não confirmado";}else{before=view;out.move.y=16;stage=4;due=now+std::chrono::milliseconds(220);}}
        else if(stage==4){y=degrees(pitch(view)-pitch(before))/16;failed=!validSensitivity(y);out.move.y=-16;stage=5;due=now+std::chrono::milliseconds(220);}
        else{stage=0;out.busy=false;out.save=!failed;out.x=x;out.y=y;out.fov=referenceFov;out.message=failed?"Calibração Y falhou: movimento não confirmado":"Sensibilidade calibrada para esta configuração";}
        return out;
    }
};
}
