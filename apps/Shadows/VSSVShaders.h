#pragma once

#include<iostream>
const std::string _drawSidesVertexShaderSrc = R".(
#version 450

#ifndef MAX_MULTIPLICITY
#define MAX_MULTIPLICITY 2
#endif//MAX_MULTIPLICITY

layout(location=0)in vec3  edgeVertexA        ;
layout(location=1)in vec3  edgeVertexB        ;
layout(location=2)in float nofOppositeVertices;
layout(location=3)in vec3  oppositeVertices[MAX_MULTIPLICITY];

uniform vec4 lightPosition    = vec4(10,10,10,1);
uniform mat4 modelMatrix      = mat4(1.f);
uniform mat4 viewMatrix       = mat4(1.f);
uniform mat4 projectionMatrix = mat4(1.f);

int greaterVec(vec3 a,vec3 b){
  return int(dot(sign(a-b),vec3(4.0,2.0,1.0)));
}

int computeMult(vec3 A,vec3 B,vec3 C,vec4 L){
  vec3 n=cross(C-A,L.xyz-A*L.w);
  return int(sign(dot(n,B-A)));
}

int currentMultiplicity(vec3 A,vec3 B,vec3 O,vec4 L){
  if(greaterVec(A,O)>0)
    return computeMult(O,A,B,L);
  else if(greaterVec(B,O)>0)
    return -computeMult(A,O,B,L);
  else
    return computeMult(A,B,O,L);
}

void main(){
  mat4 mvp = projectionMatrix*viewMatrix*modelMatrix;
  //int vertexIDCCW = int(gl_VertexID>2?gl_VertexID-2:2-gl_VertexID);
  //int vertexIDCW  = int(gl_VertexID>2?6-gl_VertexID:gl_VertexID);
  int vertexIDCCW = gl_VertexID;
  int vertexIDCW  = 4-gl_VertexID;
  int sideID      = gl_InstanceID%MAX_MULTIPLICITY;
  vec4 P[4];
  P[0]=vec4(edgeVertexA.xyz,1);
  P[1]=vec4(edgeVertexB.xyz,1);
  P[2]=vec4(P[0].xyz*lightPosition.w-lightPosition.xyz,0);
  P[3]=vec4(P[1].xyz*lightPosition.w-lightPosition.xyz,0);
  int multiplicity = 0;
  for(uint m=0;m<uint(nofOppositeVertices) && m<2;++m)
    multiplicity += currentMultiplicity(P[0].xyz,P[1].xyz,oppositeVertices[m].xyz,lightPosition);
  if(sideID<int(abs(float(multiplicity)))){
    if(multiplicity>0)
      gl_Position=mvp*P[vertexIDCCW];
    if(multiplicity<0)
      gl_Position=mvp*P[vertexIDCW];
  }else
    gl_Position=vec4(0,0,0,0);
}
).";


const std::string _drawSidesFragmentShaderSrc = R".(
#version 450
void main(){
}
).";
