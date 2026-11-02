#include "runtime.hpp"
#include <iostream>
int main(){try{
 sangue::Memory self(GetCurrentProcessId());uint64_t sample=0xabc123;
 if(self.read<uint64_t>(reinterpret_cast<uint64_t>(&sample))!=sample)return 1;
 auto path=std::filesystem::temp_directory_path()/L"sangue-sha-fixture.txt";{std::ofstream f(path);f<<"abc";}
 auto hash=sangue::fingerprint(path.wstring());std::filesystem::remove(path);
 if(hash!="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")return 2;
 if(sangue::semantic("Bip01 Head")!=sangue::Head||sangue::semantic("upperarm_l")!=sangue::UpperL||sangue::semantic("unknown")!=-1)return 3;
 std::cout<<"Own-process memory, SHA256 and semantic mapping passed\n";
 }catch(const std::exception& e){std::cerr<<e.what();return 4;}return 0;}
