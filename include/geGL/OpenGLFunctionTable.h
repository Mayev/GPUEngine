#pragma once

#include<geGL/OpenGL.h>

namespace ge{
  namespace gl{
    namespace opengl{
      class FunctionTable{
        public:
          FunctionTable(){}
          virtual ~FunctionTable(){}
          bool construct(){return this->m_init();}
          virtual bool m_init(){return true;}
#include<geGL/Generated/FunctionTableCalls.h>
#include<geGL/Generated/OpenGLPFN.h>
#include<geGL/Generated/OpenGLFunctions.h>
#include<geGL/Generated/MemberOpenGLPFN.h>
#include<geGL/Generated/MemberOpenGLFunctions.h>
      };
    }
  }
}

