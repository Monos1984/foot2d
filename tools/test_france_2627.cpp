#include "data.h"
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static bool checkUnique(const FrClubDef* a,int n,std::set<std::string>& all,const char* label){
  bool ok=true;
  for(int i=0;i<n;i++){
    if(!a[i].name || !*a[i].name){ std::printf("EMPTY %s %d\n",label,i); ok=false; continue; }
    if(!all.insert(a[i].name).second){ std::printf("DUP %s: %s\n",label,a[i].name); ok=false; }
  }
  return ok;
}
int main(){
  bool ok=true;
  std::printf("L1=%d L2=%d L3=%d N1=%d N2=%d RES_R1=%d\n",NUM_FR_L1,NUM_FR_L2,NUM_FR_L3,NUM_FR_N1,NUM_FR_N2,NUM_FR_RESERVES_R1);
  if(NUM_FR_L1!=18||NUM_FR_L2!=18||NUM_FR_L3!=18||NUM_FR_N1!=49||NUM_FR_N2!=111) ok=false;
  std::set<std::string> first;
  ok &= checkUnique(FR_L1,NUM_FR_L1,first,"L1");
  ok &= checkUnique(FR_L2,NUM_FR_L2,first,"L2");
  ok &= checkUnique(FR_L3,NUM_FR_L3,first,"L3");
  ok &= checkUnique(FR_N1,NUM_FR_N1,first,"N1");
  ok &= checkUnique(FR_N2,NUM_FR_N2,first,"N2");
  std::set<std::string> all=first;
  ok &= checkUnique(FR_RESERVES_R1,NUM_FR_RESERVES_R1,all,"RES_R1");
  auto checkParents=[&](const FrClubDef* a,int n,const char* label){
    for(int i=0;i<n;i++) if(a[i].parent && *a[i].parent && !first.count(a[i].parent)){
      std::printf("BAD_PARENT %s: %s -> %s\n",label,a[i].name,a[i].parent); ok=false;
    }
  };
  checkParents(FR_N1,NUM_FR_N1,"N1");
  checkParents(FR_N2,NUM_FR_N2,"N2");
  checkParents(FR_RESERVES_R1,NUM_FR_RESERVES_R1,"RES_R1");
  std::puts(ok?"PASS":"FAIL");
  return ok?0:1;
}
