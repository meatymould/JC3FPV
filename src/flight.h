#pragma once
#include <cmath>
#include <algorithm>
namespace fpv {
constexpr double pi=3.141592653589793, rad=pi/180.;
struct V { double x=0,y=0,z=0; V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};} V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};} V operator*(double s)const{return{x*s,y*s,z*s};} double length()const{return std::sqrt(x*x+y*y+z*z);} };
struct Q { double w=1,x=0,y=0,z=0;
 Q operator*(Q b)const{return{w*b.w-x*b.x-y*b.y-z*b.z,w*b.x+x*b.w+y*b.z-z*b.y,w*b.y-x*b.z+y*b.w+z*b.x,w*b.z+x*b.y-y*b.x+z*b.w};}
 Q unit()const{double n=std::sqrt(w*w+x*x+y*y+z*z);return{w/n,x/n,y/n,z/n};}
 V rotate(V v)const{Q r=(*this)*Q{0,v.x,v.y,v.z}*Q{w,-x,-y,-z};return{r.x,r.y,r.z};}
 static Q axis(V a,double r){double s=std::sin(r/2);return{std::cos(r/2),a.x*s,a.y*s,a.z*s};}
};
struct Config {
 double mass=.65,maxThrust=26,dragArea=.018,verticalDragArea=.035,linearDrag=.12;
 double maxRate=670,centerRate=100,expo=.3,yawMaxRate=500;
 double rateTime=.035,maxAccel=18000,motorTime=.025,thrustExponent=2;
 double tilt=25,fov=110,deadzone=.05,throttleExpo=0;
 double gravity=9.80665,speedLimit=0,dragScale=1;
 int controller=0,throttleMode=0; // 0: true Mode 2 left Y; 1: right trigger gamepad convenience
};
struct Input {double roll=0,pitch=0,yaw=0,throttle=0;};
inline double rate(double x,double center,double maximum,double expo){
 // Betaflight Actual-rate equation: independent center slope and full-stick rate.
 double a=std::abs(x);return center*x+(maximum-center)*a*(expo*x*x*x*x*x+(1-expo)*x);
}
struct Flight {
 Config c; V p,v,omega; Q q; double thrust=0,accumulator=0; unsigned long long steps=0;
 void reset(V origin,double yaw){p=origin;v={};omega={};q=Q::axis({0,1,0},yaw);thrust=0;accumulator=0;}
 void step(Input in,double h){
  V target{-rate(in.pitch,c.centerRate,c.maxRate,c.expo)*rad,-rate(in.yaw,c.centerRate,c.yawMaxRate,c.expo)*rad,-rate(in.roll,c.centerRate,c.maxRate,c.expo)*rad};
  double limit=c.maxAccel*rad;
  V alpha{std::clamp((target.x-omega.x)/c.rateTime,-limit,limit),std::clamp((target.y-omega.y)/c.rateTime,-limit,limit),std::clamp((target.z-omega.z)/c.rateTime,-limit,limit)};
  omega=omega+alpha*h;
  double speed=omega.length();if(speed>1e-9)q=(q*Q::axis(omega*(1/speed),speed*h)).unit();
  double t=std::clamp(in.throttle,0.,1.);t=(1-c.throttleExpo)*t+c.throttleExpo*t*t*(3-2*t);
  double desired=c.maxThrust*std::pow(t,c.thrustExponent);
  thrust+=(desired-thrust)*(1-std::exp(-h/c.motorTime));
  // SI units, world Y up. Drag uses body-frame projected areas; thrust always body +Y.
  V body=Q{q.w,-q.x,-q.y,-q.z}.rotate(v);
  V d{-0.5*1.225*c.dragArea*body.x*std::abs(body.x),-0.5*1.225*c.verticalDragArea*body.y*std::abs(body.y),-0.5*1.225*c.dragArea*body.z*std::abs(body.z)};
  V a=q.rotate(V{0,thrust,0}+d*c.dragScale)*(1/c.mass)+V{0,-c.gravity,0}-v*(c.linearDrag*c.dragScale);
  v=v+a*h;double airspeed=v.length();if(c.speedLimit>0&&airspeed>c.speedLimit)v=v*(c.speedLimit/airspeed);p=p+v*h;++steps;
 }
 void advance(Input in,double dt){accumulator+=std::clamp(dt,0.,.1);while(accumulator+1e-12>=.001){step(in,.001);accumulator-=.001;}}
 // JC3 camera uses right/up/back basis columns followed by world position.
 void matrix(float* m)const {Q camera=q*Q::axis({1,0,0},c.tilt*rad);V r=camera.rotate({1,0,0}),u=camera.rotate({0,1,0}),b=camera.rotate({0,0,1});V a[4]={r,u,b,p};for(int i=0;i<4;i++){m[i*4]=float(a[i].x);m[i*4+1]=float(a[i].y);m[i*4+2]=float(a[i].z);m[i*4+3]=i==3?1.f:0.f;}}
};
}
