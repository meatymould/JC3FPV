#pragma once
#include "flight.h"
namespace fpv {
inline double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
struct RayHit {double fraction=1;V normal;};
struct CollisionSettings {double radius=.18,skin=.015,restitution=.08,friction=.18;bool enabled=true;};
// A continuous ray fan approximates a small spherical collision volume. This is
// conservative around edges, not a native Havok convex shape sweep.
template<class Ray> bool collideMove(V start,V&end,V&velocity,const CollisionSettings&c,Ray&&ray){
 bool contact=false;V remaining=end-start;end=start;
 for(int iteration=0;iteration<4;iteration++){
  double length=remaining.length();if(length<1e-9)break;V dir=remaining*(1/length);
  V side=cross(dir,std::abs(dir.y)<.8?V{0,1,0}:V{1,0,0});side=side*(1/side.length());V up=cross(side,dir);
  V offsets[5]={{},side*(c.radius*.8),side*(-c.radius*.8),up*(c.radius*.8),up*(-c.radius*.8)};
  double travel=length;V normal{};bool hit=false;double castLength=length+2*c.radius+c.skin;
  for(V offset:offsets){RayHit h;if(!ray(end+offset,end+offset+dir*castLength,h))continue;
   double nlen=h.normal.length();if(nlen<.5||!std::isfinite(h.fraction)||h.fraction<0||h.fraction>1)continue;
   V n=h.normal*(1/nlen);if(dot(n,dir)>0)n=n*(-1);double approach=-dot(dir,n);if(approach<1e-6)continue;
   V point=end+offset+dir*(castLength*h.fraction);
   double allowed=(dot(end-point,n)-c.radius-c.skin)/approach;
   if(allowed<travel){travel=std::max(0.,allowed);normal=n;hit=true;}
  }
  end=end+dir*travel;if(!hit)break;contact=true;
  double vn=dot(velocity,normal);if(vn<0){V tangent=velocity-normal*vn;double ts=tangent.length();double friction=ts>1e-9?std::max(0.,1-c.friction*(1+c.restitution)*(-vn)/ts):0;velocity=tangent*friction-normal*(vn*c.restitution);}
  // Consume the normal displacement, retain tangential travel for sliding.
  remaining=dir*(length-travel);double into=dot(remaining,normal);if(into<0)remaining=remaining-normal*into;
 }
 return contact;
}
template<class Ray> bool advanceCollision(Flight&f,Input in,double dt,const CollisionSettings&c,Ray&&ray){
 if(!c.enabled){f.advance(in,dt);return false;}bool hit=false;double left=std::clamp(dt,0.,.1);
 while(left>1e-9){double h=std::min(left,.005);V start=f.p;f.advance(in,h);hit=collideMove(start,f.p,f.v,c,ray)||hit;left-=h;}return hit;
}
}
