#include "aim.hpp"
#include <iostream>
#include <cstdlib>
using namespace sangue;
View oriented(float y=0,float p=0){View v;float a=y*.01745329252f,b=p*.01745329252f;v.axes={std::cos(a),0,-std::sin(a),0,1,0,std::sin(a),0,std::cos(a)};
    if(p!=0)v.axes={1,0,0,0,std::cos(b),-std::sin(b),0,std::sin(b),std::cos(b)};v.fov=90;v.width=1920;v.height=1080;return v;}
int main(){unsigned n=0;auto check=[&](bool ok,const char* message){++n;if(!ok){std::cerr<<message<<'\n';std::exit(1);}};
    auto now=Clock::now();Settings s;s.rage=true;s.unknownTargets=true;
    Frame f;f.focused=true;f.stamp=now;f.view=oriented();Pose a;a.id=1;a.points[Head]=V3{1,0,-10};f.poses={a};
    Stabilizer first;auto baseline=first.update(f,s,now);check(baseline.x>0&&baseline.y==0,"rightward target moves right");
    for(auto size:std::array<std::array<int,2>,7>{{{640,480},{800,600},{1280,720},{1920,1080},{2560,1440},{3840,2160},{1080,1920}}}){
        f.view.width=size[0];f.view.height=size[1];Stabilizer controller;auto step=controller.update(f,s,now);check(step.x==baseline.x,"same angular correction across resolutions");}
    f.view=oriented();Settings half=s;half.strength=.5f;Stabilizer second;auto lower=second.update(f,half,now);check(lower.x>0&&lower.x<baseline.x,"strength changes correction without a calibration stage");
    Settings reverse=s;reverse.invertX=true;Stabilizer inverted;check(inverted.update(f,reverse,now).x==-baseline.x,"inverted sensitivity preserved");
    check(first.update(f,s,now+std::chrono::milliseconds(2)).x==0,"same cached snapshot is not sent twice");
    check(first.update(f,s,now+std::chrono::milliseconds(9)).x==0,"cached sample remains unchanged");
    f.stamp=now+std::chrono::milliseconds(1);f.view=oriented(1.f);check(first.update(f,s,f.stamp).x>0,"new camera response is processed without fixed 8ms pause");f.stamp=now;f.view=oriented();
    Stabilizer retry;auto initial=retry.update(f,s,now);retry.rejected();check(retry.update(f,s,now+std::chrono::milliseconds(1)).x==initial.x,"Windows rejection does not latch the controller permanently");
    Stabilizer continuous;int activeCycles=0;int lastMovement=-1;
    for(int i=0;i<160;++i){Frame live=f;live.stamp=now+std::chrono::milliseconds(i*2);auto step=continuous.update(live,s,live.stamp);if(step.x){++activeCycles;lastMovement=i;}}
    check(activeCycles>20&&lastMovement>140,"fresh samples continue correcting beyond initial acquisition without camera latch");
    Pose b=a;b.id=2;b.points[Head]=V3{1.01f,0,-10};f.poses={a,b};Stabilizer lock;check(lock.update(f,s,now).target==1,"closest first target");
    f.poses[0].points[Head]=V3{1.01f,0,-10};f.poses[1].points[Head]=V3{1,0,-10};f.stamp=now+std::chrono::milliseconds(9);check(lock.update(f,s,f.stamp).target==1,"retain target despite small ranking noise");
    f.poses={a};Stabilizer closed;float angle=0;
    for(int i=0;i<150;++i){auto t=now+std::chrono::milliseconds(8*i);f.stamp=t;f.view=oriented(angle);auto step=closed.update(f,s,t);check(step.x>=0,"closed-loop correction must not oscillate across center");angle+=step.x*.1f;}
    check(std::abs(angle-5.710593f)<.12f,"controller converges near target without alternating motion");
    Frame centered=f;centered.view=oriented();centered.stamp=now;centered.poses[0].points[Head]=V3{.0001f,0,-10};Stabilizer deadzone;check(deadzone.update(centered,s,now).x==0,"ignore sub-count center noise");
    centered.stamp=now+std::chrono::milliseconds(1);centered.poses[0].points[Head]=V3{1,0,-10};check(deadzone.update(centered,s,centered.stamp).x>0,"centered target moving again resumes correction immediately");
    Stabilizer moving;float cameraAngle=0;int positive=0,negative=0;float lastError=0;
    for(int i=0;i<700;++i){auto t=now+std::chrono::milliseconds(i*8);float desired=4.f*std::sin(i*.025f),r=desired*.01745329252f;Frame live;live.stamp=t;live.focused=true;live.view=oriented(cameraAngle);Pose person;person.id=99;person.points[Head]=V3{10*std::sin(r),0,-10*std::cos(r)};live.poses={person};auto step=moving.update(live,s,t);cameraAngle+=step.x*.1f;positive+=step.x>0;negative+=step.x<0;lastError=std::abs(cameraAngle-desired);}
    check(positive>50&&negative>50&&lastError<1.f,"continuous moving-target tracking reverses direction without release or restart");
    Frame gone=f;gone.stamp=now;gone.poses.clear();check(moving.update(gone,s,now).target==0,"lost target clears lock");gone.poses={a};gone.view=oriented();gone.stamp=now+std::chrono::milliseconds(1);check(moving.update(gone,s,gone.stamp).x>0,"target reacquired without resetting module");
    f.stamp=now;f.view=oriented();f.poses={a};Settings invalid=s;invalid.strength=0;
    check(deadzone.update(f,invalid,now).x==0,"invalid strength cannot send input");
    Stabilizer zoomTracking;int zoomCycles=0;
    for(int i=0;i<180;++i){
        f.stamp=now+std::chrono::milliseconds(i*8);f.view=oriented();
        f.view.fov=i<40?90.f:i<100?40.f:60.f;
        f.poses[0].points[Head]=V3{.2f,0,-10};
        check(aimState(f,s,true,f.stamp)=="Pronto para entrada","zoom change must not block tracking");
        auto step=zoomTracking.update(f,s,f.stamp);zoomCycles+=step.x>0;
    }
    check(zoomCycles>150,"corrections continue before, during and after ADS zoom changes");
    Stabilizer fractional;int smallMoves=0;
    for(int i=0;i<100;++i){
        f.stamp=now+std::chrono::milliseconds(i);f.view=oriented();f.poses[0].points[Head]=V3{.0241f,0,-10};
        smallMoves+=fractional.update(f,s,f.stamp).x>0;
    }
    check(smallMoves>0,"fractional corrections accumulate instead of stalling outside the center zone");
    f.stamp=now;f.view=oriented();f.poses[0].points[Head]=V3{0,1,-10};Stabilizer vertical;
    auto up=vertical.update(f,s,now);check(up.x==0&&up.y<0,"upward target moves up");
    Settings invertedY=s;invertedY.invertY=true;Stabilizer reverseY;
    check(reverseY.update(f,invertedY,now).y==-up.y,"vertical inversion does not need measured sensitivity");
    f.focused=false;check(vertical.update(f,s,now).target==0,"focus loss clears target");
    f.focused=true;check(vertical.update(f,s,now+std::chrono::milliseconds(25)).target==0,"expired data cannot move mouse");
    std::cout<<n<<" controller and synthetic tracking checks passed\n";
}
