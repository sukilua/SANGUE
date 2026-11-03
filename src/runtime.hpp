#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cctype>
#include <unordered_map>
#include "model.hpp"
#include "aim.hpp"
namespace sangue {
inline constexpr char ExpectedHash[]="ff080393961ac709090824356621358ae4f63dbe1c62ff636d528589cd097747";
inline std::wstring wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),w.data(),n);return w;}
struct Handle{HANDLE h{};Handle()=default;explicit Handle(HANDLE v):h(v){};~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;};
inline std::string fingerprint(const std::wstring& path){
    std::ifstream input(std::filesystem::path(path),std::ios::binary);if(!input)throw std::runtime_error("Executável inacessível");
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};if(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 indisponível");
    DWORD size=0,bytes=0;BCryptGetProperty(a,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&bytes,0);std::vector<UCHAR> object(size);std::array<UCHAR,32> digest{};
    if(BCryptCreateHash(a,&h,object.data(),size,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(a,0);throw std::runtime_error("SHA256 falhou");}
    std::array<char,65536> buffer{};NTSTATUS result=0;
    while(input){input.read(buffer.data(),buffer.size());auto count=input.gcount();if(count>0&&(result=BCryptHashData(h,reinterpret_cast<PUCHAR>(buffer.data()),static_cast<ULONG>(count),0))<0)break;}
    if(result>=0)result=BCryptFinishHash(h,digest.data(),DWORD(digest.size()),0);BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(a,0);
    if(result<0||input.bad())throw std::runtime_error("Leitura/hash incompleto");std::ostringstream out;for(auto v:digest)out<<std::hex<<std::setw(2)<<std::setfill('0')<<int(v);return out.str();
}
class Memory{
    Handle handle;
public:
    explicit Memory(DWORD pid):handle(OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)){if(!handle.h)throw std::runtime_error("OpenProcess: "+std::to_string(GetLastError()));}
    bool alive()const{DWORD code{};return GetExitCodeProcess(handle.h,&code)&&code==STILL_ACTIVE;}
    void bytes(uint64_t address,void* out,size_t n)const{SIZE_T count{};if(address<0x10000||address>0x7fffffffffffULL-n||!ReadProcessMemory(handle.h,reinterpret_cast<LPCVOID>(address),out,n,&count)||count!=n)throw std::runtime_error("ReadProcessMemory: "+std::to_string(GetLastError()));}
    template<class T>T read(uint64_t p)const{T v{};bytes(p,&v,sizeof(v));return v;}
    uint64_t ptr(uint64_t p)const{auto v=read<uint64_t>(p);if(v<0x10000||v>0x7fffffffffffULL)throw std::runtime_error("Ponteiro inválido");return v;}
};
inline int semantic(std::string s){
    s.erase(std::remove_if(s.begin(),s.end(),[](unsigned char c){return !std::isalnum(c);}),s.end());for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));
    auto ends=[&](const char* suffix){size_t n=std::strlen(suffix);return s.size()>=n&&s.compare(s.size()-n,n,suffix)==0;};
    if(ends("head"))return Head;if(ends("neck")||ends("neck1"))return Neck;if(ends("spine2")||ends("chest"))return Chest;if(ends("spine1"))return Spine;if(ends("pelvis")||ends("hips"))return Pelvis;
    const char* names[][2]={{"lupperarm","upperarml"},{"lforearm","lowerarml"},{"lhand","handl"},{"rupperarm","upperarmr"},{"rforearm","lowerarmr"},{"rhand","handr"},{"lthigh","thighl"},{"lcalf","calfl"},{"lfoot","footl"},{"rthigh","thighr"},{"rcalf","calfr"},{"rfoot","footr"}};
    for(int i=0;i<12;++i)if(ends(names[i][0])||ends(names[i][1]))return UpperL+i;return -1;
}
inline std::array<int32_t,3> anchor(const Memory& m,uint64_t p){auto a=m.read<std::array<int32_t,3>>(p+0x1ac);if(a==std::array<int32_t,3>{INT32_MAX,INT32_MAX,INT32_MAX})a={};return a;}
inline View camera(const Memory& m,uint64_t base,int width,int height){
    auto engine=m.ptr(base+0x80d5980),game=m.ptr(engine+0x58),player=m.ptr(game+0x58),cam=m.ptr(player+0xd88);
    View v;v.position=m.read<V3>(cam+0x7c);v.axes=m.read<std::array<float,9>>(cam+0x41c);v.fov=m.read<float>(cam+0x454);v.width=width;v.height=height;v.anchor=anchor(m,cam);if(!v.valid())throw std::runtime_error("Câmera incompatível");return v;
}
inline std::vector<Pose> sample(const Memory& m,uint64_t base,Frame& frame,Clock::time_point deadline=Clock::now()+std::chrono::milliseconds(12)){
    std::vector<Pose> result;std::unordered_map<uint64_t,int> names;
    auto manager=m.ptr(base+0x8b7cd90),head=m.ptr(manager+8),node=m.ptr(head);std::unordered_set<uint64_t> visited;
    while(node!=head){
        if(Clock::now()>deadline){frame.budgetExpired=true;break;}
        if(visited.size()>=4096||!visited.insert(node).second)throw std::runtime_error("Lista cíclica/inválida");++frame.nodes;
        auto next=m.ptr(node);
        try{
            auto component=m.ptr(node+0x18),entity=m.ptr(component+0x40);if(!(m.read<uint8_t>(entity+0x130)&0x10)){node=next;continue;}
            auto skeleton=m.ptr(component+0xe8),pose=m.ptr(skeleton+0xd8),first=m.ptr(pose+0x10),last=m.ptr(pose+0x18);
            if(last<first||(last-first)%0xa0||last-first>512*0xa0)throw std::runtime_error("Vetor de ossos inválido");
            auto transform=m.read<std::array<float,12>>(entity+0x58);for(float v:transform)if(!std::isfinite(v)||std::abs(v)>1e8)throw std::runtime_error("Transform inválido");
            auto origin=anchor(m,entity);std::vector<uint8_t> data(static_cast<size_t>(last-first));if(!data.empty())m.bytes(first,data.data(),data.size());
            Pose actor;actor.id=entity;int recognized=0;std::array<bool,Count> ambiguous{};
            for(size_t at=0;at<data.size();at+=0xa0)try{
                if(Clock::now()>deadline){frame.budgetExpired=true;throw std::runtime_error("Prazo de leitura excedido");}uint64_t descriptor{};std::memcpy(&descriptor,data.data()+at+0x88,8);
                int slot=-1;auto cached=names.find(descriptor);
                if(cached!=names.end())slot=cached->second;
                else{std::array<char,96> name{};auto string=m.ptr(m.ptr(descriptor+0x58));m.bytes(string,name.data(),name.size());auto end=std::find(name.begin(),name.end(),'\0');if(end==name.end())continue;
                    std::string label(name.begin(),end);slot=semantic(label);names.emplace(descriptor,slot);if(frame.sampledNames.size()<200)frame.sampledNames+=label+" ";}
                if(slot<0)continue;++frame.namedBones;
                if(ambiguous[slot])continue;if(actor.points[slot]){actor.points[slot].reset();ambiguous[slot]=true;--recognized;++frame.ambiguousBones;continue;}
                std::array<float,12> bone{};std::memcpy(bone.data(),data.data()+at+0x30,48);for(float v:bone)if(!std::isfinite(v))throw std::runtime_error("Matriz de osso não finita");
                V3 local{bone[9],bone[10],bone[11]};if(!finite(local))throw std::runtime_error("Osso não finito");
                V3 p{transform[0]*local.x+transform[3]*local.y+transform[6]*local.z+transform[9],transform[1]*local.x+transform[4]*local.y+transform[7]*local.z+transform[10],transform[2]*local.x+transform[5]*local.y+transform[8]*local.z+transform[11]};
                p.x+=float(double(origin[0])-frame.view.anchor[0]);p.y+=float(double(origin[1])-frame.view.anchor[1]);p.z+=float(double(origin[2])-frame.view.anchor[2]);
                if(!finite(p))throw std::runtime_error("Posição inválida");actor.points[slot]=p;++recognized;
            }catch(const std::exception& e){++frame.readErrors;if(frame.error.empty())frame.error=std::string("Osso: ")+e.what();if(frame.budgetExpired)break;}
            if(frame.budgetExpired){++frame.rejected;break;}
            if(anchor(m,entity)!=origin||m.ptr(component+0xe8)!=skeleton||m.ptr(skeleton+0xd8)!=pose)throw std::runtime_error("Identidade/origem da pose mudou");
            if(m.read<uint64_t>(pose+0x10)!=first||m.read<uint64_t>(pose+0x18)!=last||m.ptr(component+0x40)!=entity)throw std::runtime_error("Pose mudou durante a leitura");
            if(recognized>=5)result.push_back(std::move(actor));else ++frame.rejected;
        }catch(const std::exception& e){++frame.readErrors;++frame.rejected;if(frame.error.empty())frame.error=std::string("Entidade: ")+e.what();}
        node=next;
    }
    return unique(std::move(result));
}
inline HWND gameWindow(DWORD pid){struct Search{DWORD pid;HWND found;};Search s{pid,nullptr};EnumWindows([](HWND w,LPARAM v)->BOOL{auto& s=*reinterpret_cast<Search*>(v);DWORD id{};GetWindowThreadProcessId(w,&id);if(id==s.pid&&IsWindowVisible(w)&&GetWindow(w,GW_OWNER)==nullptr){s.found=w;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&s));return s.found;}
inline DWORD findGame(){Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0));PROCESSENTRY32W e{sizeof(e)};if(Process32FirstW(snapshot.h,&e))do{if(!_wcsicmp(e.szExeFile,L"BloodStrike.exe"))return e.th32ProcessID;}while(Process32NextW(snapshot.h,&e));return 0;}
class Runtime{
    std::jthread thread;std::mutex mutex;Settings settings;std::shared_ptr<const Frame> published=std::make_shared<Frame>(),lastFocused;
    HWND notify{};std::atomic<bool> pending{},calibrateRequested{};uint64_t serial{};
    void publish(Frame f){f.serial=++serial;if(f.focused)std::atomic_store(&lastFocused,std::make_shared<const Frame>(f));std::atomic_store(&published,std::make_shared<const Frame>(std::move(f)));if(notify&&!pending.exchange(true)&&!PostMessageW(notify,WM_APP+1,0,0))pending=false;}
public:
    std::atomic<uint64_t> sent{};std::atomic<double> maxCycleMs{};
    ~Runtime(){stop();}
    Settings config(){std::scoped_lock lock(mutex);return settings;}
    void config(Settings s){std::scoped_lock lock(mutex);settings=s;}
    std::shared_ptr<const Frame> frame(){return std::atomic_load(&published);}
    std::shared_ptr<const Frame> diagnosticFrame(){auto f=frame();auto prior=std::atomic_load(&lastFocused);return !f->focused&&prior&&prior->pid==f->pid?prior:f;}
    void requestCalibration(){std::scoped_lock lock(mutex);settings.calibrated=false;calibrateRequested=true;}
    void acknowledge(){pending=false;}
    void stop(){if(thread.joinable()){thread.request_stop();thread.join();}}
    void start(HWND window){notify=window;thread=std::jthread([this](std::stop_token stop){
        DWORD pid=0;uint64_t base=0;std::unique_ptr<Memory> memory;std::string hash;Clock::time_point retry{};Stabilizer stabilizer;Calibration calibration;std::string calibrationStatus="Sensibilidade não calibrada";
        Handle timer(CreateWaitableTimerExW(nullptr,nullptr,0x2,TIMER_ALL_ACCESS));if(!timer.h)timer.h=CreateWaitableTimerW(nullptr,FALSE,nullptr);
        while(!stop.stop_requested()){
            const auto began=Clock::now();Frame f;Settings s=config();if(calibrateRequested.exchange(false)){calibration.start(began);stabilizer.reset();calibrationStatus="Calibração: volte ao jogo em 3 segundos";}
            try{
                if(!memory){if(began<retry){std::this_thread::sleep_for(std::chrono::milliseconds(10));continue;}retry=began+std::chrono::seconds(2);pid=findGame();if(!pid)throw std::runtime_error("Abra o Blood Strike pela Steam");
                    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid));MODULEENTRY32W module{sizeof(module)};if(!Module32FirstW(snap.h,&module))throw std::runtime_error("Módulo indisponível: "+std::to_string(GetLastError()));
                    base=reinterpret_cast<uint64_t>(module.modBaseAddr);hash=fingerprint(module.szExePath);f.hash=hash;if(hash!=ExpectedHash)throw std::runtime_error("SHA256 diferente do layout auditado; leitura bloqueada");memory=std::make_unique<Memory>(pid);
                }
                f.pid=pid;f.moduleBase=base;f.hash=hash;auto w=gameWindow(pid);if(!w)throw std::runtime_error("Janela do jogo indisponível");RECT r{};GetClientRect(w,&r);POINT origin{};ClientToScreen(w,&origin);f.x=origin.x;f.y=origin.y;f.focused=GetForegroundWindow()==w;
                if(!f.focused){f.status="Conectado / jogo sem foco";f.aimState=aimState(f,s,false,Clock::now());}
                else{f.status="Leitura experimental";f.view=camera(*memory,base,r.right,r.bottom);if(s.rage&&!calibration.busy()){f.poses=sample(*memory,base,f);auto end=camera(*memory,base,r.right,r.bottom);if(end.anchor!=f.view.anchor)throw std::runtime_error("Origem da câmera mudou");f.view=end;}
                    f.stamp=began;
                    const bool held=(GetAsyncKeyState(s.key)&0x8000)!=0;f.aimState=aimState(f,s,held,Clock::now());
                    if(!calibration.busy()&&f.aimState=="Pronto para entrada"){
                        auto step=stabilizer.update(f,s,Clock::now());
                        if((step.x||step.y)&&GetForegroundWindow()==w&&!stop.stop_requested()){INPUT input{};input.type=INPUT_MOUSE;input.mi.dx=step.x;input.mi.dy=step.y;input.mi.dwFlags=MOUSEEVENTF_MOVE;SetLastError(0);auto accepted=SendInput(1,&input,sizeof(input));sent+=accepted;f.aimState=accepted?"Correção estabilizada aceita pelo Windows":"Windows não aceitou a entrada ("+std::to_string(GetLastError())+")";}
                        else f.aimState="Alvo estabilizado / aguardando câmera";
                    }else stabilizer.reset();
                }
                if(!f.focused)stabilizer.reset();
                if(calibration.busy()){
                    auto c=calibration.tick(f.view,f.focused,Clock::now());calibrationStatus=c.message;
                    if((c.move.x||c.move.y)&&GetForegroundWindow()==w){INPUT input{};input.type=INPUT_MOUSE;input.mi.dx=c.move.x;input.mi.dy=c.move.y;input.mi.dwFlags=MOUSEEVENTF_MOVE;if(!SendInput(1,&input,sizeof(input))){calibration.cancel();calibrationStatus="Calibração falhou: Windows recusou a entrada";}}
                    if(c.save){std::scoped_lock lock(mutex);settings.degreesX=c.x;settings.degreesY=c.y;settings.calibrationFov=c.fov;settings.calibrated=true;}
                    f.aimState="Calibração em andamento";
                }

            }catch(const std::exception& e){f.error=e.what();f.status="Leitura indisponível";f.poses.clear();stabilizer.reset();if(calibration.busy()){calibration.cancel();calibrationStatus="Calibração cancelada: "+f.error;}f.aimState="Leitura indisponível: "+f.error;if(memory&&!memory->alive())memory.reset();}
            f.calibrationState=calibrationStatus;f.readMs=std::chrono::duration<float,std::milli>(Clock::now()-began).count();maxCycleMs.store(std::max(maxCycleMs.load(),double(f.readMs)));publish(std::move(f));
            if(timer.h){LARGE_INTEGER due{};due.QuadPart=-10000;SetWaitableTimer(timer.h,&due,0,nullptr,nullptr,FALSE);WaitForSingleObject(timer.h,30);}else std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });}
};
}
