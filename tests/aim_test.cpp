#include "aim.hpp"
#include <iostream>
#include <cstdlib>
using namespace sangue;
View oriented(float y=0,float p=0){View v;float a=y*.01745329252f,b=p*.01745329252f;v.axes={std::cos(a),0,-std::sin(a),0,1,0,std::sin(a),0,std::cos(a)};
    if(p!=0)v.axes={1,0,0,0,std::cos(b),-std::sin(b),0,std::sin(b),std::cos(b)};v.fov=90;v.width=1920;v.height=1080;return v;}
int main(){unsigned n=0;auto check=[&](bool ok,const char* message){++n;if(!ok){std::cerr<<message<<'\n';std::exit(1);}};
    auto now=Clock::now();Settings s;s.rage=true;s.unknownTargets=true;s.calibrated=true;
    Frame f;f.focused=true;f.stamp=now;f.view=oriented();Pose a;a.id=1;a.points[Head]=V3{1,0,-10};f.poses={a};
    Stabilizer first;auto baseline=first.update(f,s,now);check(baseline.x>0&&baseline.y==0,"rightward target moves right");
    for(auto size:std::array<std::array<int,2>,7>{{{640,480},{800,600},{1280,720},{1920,1080},{2560,1440},{3840,2160},{1080,1920}}}){
        f.view.width=size[0];f.view.height=size[1];Stabilizer controller;auto step=controller.update(f,s,now);check(step.x==baseline.x,"same angular correction across resolutions");}
    f.view=oriented();Settings half=s;half.degreesX=.2f;Stabilizer second;auto lower=second.update(f,half,now);check(lower.x*2==baseline.x,"calibrated sensitivity scales mouse counts");
    Settings reverse=s;reverse.degreesX=-.1f;Stabilizer inverted;check(inverted.update(f,reverse,now).x==-baseline.x,"inverted sensitivity preserved");
    check(first.update(f,s,now+std::chrono::milliseconds(2)).x==0,"do not repeat movement every worker tick");
    check(first.update(f,s,now+std::chrono::milliseconds(9)).x==0,"wait for observable camera response");
    Pose b=a;b.id=2;b.points[Head]=V3{1.01f,0,-10};f.poses={a,b};Stabilizer lock;check(lock.update(f,s,now).target==1,"closest first target");
    f.poses[0].points[Head]=V3{1.01f,0,-10};f.poses[1].points[Head]=V3{1,0,-10};check(lock.update(f,s,now+std::chrono::milliseconds(9)).target==1,"retain target despite small ranking noise");
    f.poses={a};Stabilizer closed;float angle=0;
    for(int i=0;i<150;++i){auto t=now+std::chrono::milliseconds(8*i);f.stamp=t;f.view=oriented(angle);auto step=closed.update(f,s,t);check(step.x>=0,"closed-loop correction must not oscillate across center");angle+=step.x*s.degreesX;}
    check(std::abs(angle-5.710593f)<.12f,"controller converges near target without alternating motion");
    Frame centered=f;centered.view=oriented();centered.stamp=now;centered.poses[0].points[Head]=V3{.0001f,0,-10};Stabilizer deadzone;check(deadzone.update(centered,s,now).x==0,"ignore sub-count center noise");
    Settings missing=s;missing.calibrated=false;check(deadzone.update(f,missing,now).x==0,"uncalibrated movement blocked");
    Calibration c;c.start(now);check(c.tick(oriented(),false,now+std::chrono::seconds(1)).busy,"calibration allows time to return to game");
    check(c.tick(oriented(),true,now+std::chrono::milliseconds(3000)).move.x==16,"horizontal calibration sample");
    check(c.tick(oriented(1.6f),true,now+std::chrono::milliseconds(3220)).move.x==-16,"horizontal restoration");
    check(c.tick(oriented(),true,now+std::chrono::milliseconds(3440)).move.y==16,"vertical calibration sample");
    check(c.tick(oriented(0,-3.2f),true,now+std::chrono::milliseconds(3660)).move.y==-16,"vertical restoration");
    auto complete=c.tick(oriented(),true,now+std::chrono::milliseconds(3880));check(complete.save&&std::abs(complete.x-.1f)<.001f&&std::abs(complete.y+.2f)<.001f,"both sensitivity axes measured");
    c.start(now);c.tick(oriented(),true,now+std::chrono::seconds(3));auto canceled=c.tick(oriented(),false,now+std::chrono::milliseconds(3100));check(!canceled.busy&&!canceled.save&&!canceled.move.x&&!canceled.move.y,"focus loss cancels calibration without input");
    std::cout<<n<<" stabilization, resolution and calibration checks passed\n";
}
