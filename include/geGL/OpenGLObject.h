#pragma once

#include<geGL/OpenGL.h>

namespace ge{
  namespace gl{
    class GEGL_EXPORT OpenGLObject{
      protected:
        GLuint _id = 0u;///<object id
      public:
        OpenGLObject(GLuint id = 0u);
        virtual ~OpenGLObject();
        GLuint getId()const;
    };
  }//gl
}//ge

