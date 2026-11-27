#include "flight.h"
#include "collision.h"
#include <cstdio>
#include <cstdlib>
using namespace fpv;
void check(bool b,const char*s){printf("%s %s\n",b?"PASS":"FAIL",s);if(!b)exit(1);}
int main(){
 Flight f;f.reset({0,100,0},0);f.advance({},.1);check(f.v.y<-.97&&f.v.y>-.99,"zero throttle falls under gravity");
 f.reset({0,100,0},0);f.q=Q::axis({0,0,1},pi/2);f.advance({0,0,0,1},.1);check(f.v.x< -2&&f.v.y<-.9,"tilted thrust accelerates sideways with gravity");
 f.reset({},0);f.q=Q::axis({0,0,1},pi/3);for(int i=0;i<1000;i++)f.step({},.001);check(std::abs(f.q.z-.5)<1e-10,"centered sticks do not self-level");
 Flight a,b;a.reset({},0);b.reset({},0);Input in{.35,.2,.1,.6};for(int i=0;i<300;i++)a.advance(in,1./30);for(int i=0;i<1440;i++)b.advance(in,1./144);check((a.p-b.p).length()<.25&&std::abs(a.q.w-b.q.w)<.02,"30 and 144 FPS fixed-step agreement");
 f.reset({},0);for(int i=0;i<10000;i++)f.step({1,1,1,.5},.001);check(std::isfinite(f.p.x)&&std::abs(f.q.w*f.q.w+f.q.x*f.q.x+f.q.y*f.q.y+f.q.z*f.q.z-1)<1e-9,"unrestricted rotations preserve unit quaternion");
 check(std::abs(rate(1,100,670,.3)-670)<1e-10&&rate(.01,100,670,.3)<1.05,"rate endpoints and soft center");
 f.reset({},0);f.v={30,0,0};f.advance({},.1);check(f.v.x<30&&f.v.x>25,"drag slows momentum without immediate stop");
 CollisionSettings c;auto floor=[](V a,V b,RayHit&hit){if(a.y>=0&&b.y<0){hit.fraction=a.y/(a.y-b.y);hit.normal={0,1,0};return true;}return false;};
 V end{0,-50,0},vel{0,-120,0};check(collideMove({0,2,0},end,vel,c,floor)&&end.y>=c.radius+c.skin-1e-8&&vel.y>=0,"continuous high-speed floor impact preserves clearance");
 auto wall=[](V a,V b,RayHit&h){if(a.x<2&&b.x>=2){h.fraction=(2-a.x)/(b.x-a.x);h.normal={-1,0,0};return true;}return false;};
 end={30,0,10};vel={120,0,40};check(collideMove({},end,vel,c,wall)&&end.x<2-c.radius&&end.z>1&&vel.x<=0,"thin wall prevents tunneling while retaining slide");
 auto ceiling=[](V a,V b,RayHit&h){if(a.y<2&&b.y>=2){h.fraction=(2-a.y)/(b.y-a.y);h.normal={0,-1,0};return true;}return false;};
 end={0,5,0};vel={0,20,0};check(collideMove({},end,vel,c,ceiling)&&end.y<2-c.radius&&vel.y<=0,"ceiling collision works in world space");
 f.reset({0,1,0},0);for(int i=0;i<600;i++)advanceCollision(f,{},1./60,c,floor);check(f.p.y>=c.radius+c.skin-1e-8&&f.p.y<.25&&f.v.length()<.1,"resting on floor stays stable for ten seconds");
 end={1,2,3};vel={10,20,30};auto empty=[](V,V,RayHit&){return false;};check(!collideMove({},end,vel,c,empty)&&(end-V{1,2,3}).length()<1e-9,"clear space leaves trajectory unchanged");
 a.reset({0,20,0},0);b.reset({0,20,0},0);a.v={0,-100,0};b.v=a.v;for(int i=0;i<60;i++)advanceCollision(a,{},1./30,c,floor);for(int i=0;i<288;i++)advanceCollision(b,{},1./144,c,floor);check(a.p.y>=c.radius+c.skin-1e-8&&b.p.y>=c.radius+c.skin-1e-8&&std::abs(a.p.y-b.p.y)<.005,"30 and 144 FPS high-speed landing agreement");
 vel={10,-.1,0};end={.1,-.1,0};collideMove({0,.2,0},end,vel,c,floor);check(vel.x>9.9,"light contact friction does not erase sideways momentum");
 f.c=Config{};f.c.gravity=0;f.reset({},0);f.advance({},.1);check(f.v.length()<1e-12,"zero-gravity setting removes gravity without adding lift");
 f.c.gravity=30;f.advance({},.1);check(f.v.y<-2.9,"custom gravity controls acceleration");
 f.c=Config{};f.c.speedLimit=5;f.reset({},0);f.v={100,100,100};f.advance({},.1);check(f.v.length()<=5.000001,"optional speed limit caps total velocity");
 f.c=Config{};f.c.gravity=0;f.c.dragScale=0;f.reset({},0);f.v={30,0,0};f.advance({},.1);check(std::abs(f.v.x-30)<1e-9,"zero-drag setting preserves coasting velocity");
 puts("Flight, collision and settings tests complete");
}
