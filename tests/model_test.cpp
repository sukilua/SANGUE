#include "model.hpp"
#include <iostream>
#include <cstdlib>
using namespace sangue;
int main(){
    unsigned checks=0;auto require=[&](bool ok,const char* what){++checks;if(!ok){std::cerr<<what<<'\n';std::exit(1);}};
    View v;v.axes={1,0,0,0,1,0,0,0,1};v.fov=90;v.width=800;v.height=600;
    auto center=v.project({0,0,-10});require(center&&std::abs(center->x-400)<.001f&&std::abs(center->y-300)<.001f,"projection center");
    require(!v.project({0,0,10}),"reject behind camera");v.axes[1]=1;require(!v.valid(),"reject nonorthogonal camera");v.axes[1]=0;
    Frame f;f.stamp=Clock::now();require(fresh(f,f.stamp+std::chrono::milliseconds(24)),"fresh pose");require(!fresh(f,f.stamp+std::chrono::milliseconds(25)),"expired pose");require(!fresh(f,f.stamp-std::chrono::milliseconds(1)),"future pose");
    Pose a;a.id=9;a.points[Head]=V3{0,0,-10};Pose b=a;b.id=10;
    require(unique({a,a,b,Pose{}}).size()==2,"deduplicate stable ids");
    f.view=v;f.poses={a};Settings s;require(!target(f,s),"unknown target blocked by default");s.unknownTargets=true;require(target(f,s).has_value(),"explicit unknown target selection");
    View moved=v;moved.anchor={1000000000,-1000000000,0};View next=moved;next.anchor[0]-=100;
    require(std::abs(rebase({1,2,3},moved,next).x-101)<.001f,"camera origin rebasing preserves position");
    s.bone=Count;require(!target(f,s),"invalid bone rejected");
    s.bone=Head;s.rage=true;s.calibrated=true;f.focused=true;
    require(aimState(f,s,true,f.stamp)=="Pronto para entrada","module and held key suffice without arm latch");
    require(aimState(f,s,false,f.stamp)=="Segure a tecla de ativação configurada","key release pauses movement");
    f.focused=false;require(aimState(f,s,true,f.stamp)=="Aguardando foco no jogo","focus loss pauses movement");f.focused=true;
    require(aimState(f,s,true,f.stamp+std::chrono::milliseconds(25))=="Amostra ausente ou expirada","stale frame cannot move mouse");
    s.unknownTargets=false;require(aimState(f,s,true,f.stamp)=="Bloqueado: alvos sem equipe não autorizados","unknown target rejection explained");
    s.unknownTargets=true;Settings zoom=s;zoom.calibrationFov=60;require(aimState(f,zoom,true,f.stamp)=="Zoom diferente da calibração; recalibre","changed zoom requires calibration");s.rage=false;require(aimState(f,s,true,f.stamp)=="Rage desligado","module off pauses movement");std::cout<<checks<<" checks passed\n";
}
