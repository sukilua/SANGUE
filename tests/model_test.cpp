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
    s.bone=Count;require(!target(f,s),"invalid bone rejected");std::cout<<checks<<" checks passed\n";
}
