#include<geGL/Renderbuffer.h>

using namespace ge::gl;

/**
 * @brief Construct new renderbuffer (reserve its id)
 */
Renderbuffer::Renderbuffer(){
  this->_gl.glCreateRenderbuffers(1,&this->_id);
}

/**
 * @brief Construct new renderbuffer
 *
 * @param table opengl function table
 * @param internalFormat internal format of data in renderbuffer
 * @param width width of renderbuffer
 * @param height height of renderbuffer
 * @param samples number of samples per pixel of renderbuffer (multisampling)
 */
Renderbuffer::Renderbuffer(
    FunctionTablePointer const&table,
    GLenum                     internalFormat,
    GLsizei                    width,
    GLsizei                    height,
    GLsizei                    samples):OpenGLObject(table){
  assert(this!=nullptr);
  this->_gl.glCreateRenderbuffers(1,&this->_id);
  this->setStorage(internalFormat,width,height,samples);
}

/**
 * @brief Construct new renderbuffer with default opengl function table
 *
 * @param internalFormat internal format of data in renderbuffer
 * @param width width of renderbuffer
 * @param height height of renderbuffer
 * @param samples number of samples per pixel of renderbuffer (more than one means multisampling)
 */
Renderbuffer::Renderbuffer( 
    GLenum  internalFormat,
    GLsizei width,
    GLsizei height,
    GLsizei samples):Renderbuffer(nullptr,internalFormat,width,height,samples){
}

/**
 * @brief Reallocate storage of renderbuffer (useful when resizing framebuffer)
 *
 * @param internalFormat internal format of data in renderbuffer
 * @param width width of renderbuffer
 * @param height height of renderbuffer
 * @param samples number of samples per pixel of renderbuffer (more than one means multisampling)
 */
void Renderbuffer::setStorage(
    GLenum  internalFormat,
    GLsizei width,
    GLsizei height,
    GLsizei samples)const{
  assert(this!=nullptr);
  this->_gl.glNamedRenderbufferStorageMultisample(this->_id,internalFormat,samples,width,height);
}


/**
 * @brief Destroyes renderbuffer
 */
Renderbuffer::~Renderbuffer(){
  assert(this!=nullptr);
  this->_gl.glDeleteRenderbuffers(1,&this->_id);
}

/**
 * @brief Bind renderbuffer
 */
void   Renderbuffer::bind()const{
  assert(this!=nullptr);
  this->_gl.glBindRenderbuffer(GL_RENDERBUFFER,this->_id);
}

/**
 * @brief Unbind renderbuffer
 */
void Renderbuffer::unbind()const{
  assert(this!=nullptr);
  this->_gl.glBindRenderbuffer(GL_RENDERBUFFER,0);
}

/**
 * @brief get renderbuffer width
 *
 * @return width of renderbuffer in pixels
 */
GLint Renderbuffer::getWidth()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_WIDTH);
}

/**
 * @brief get renderbuffer height
 *
 * @return height of renderbuffer in pixels
 */
GLint Renderbuffer::getHeight()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_HEIGHT);
}

/**
 * @brief get renderbuffer internal format
 *
 * @return internal format of renderbuffer
 */
GLenum Renderbuffer::getInternalFormat()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_INTERNAL_FORMAT);
}

/**
 * @brief get number of samples per pixel of renderbuffer (more than1 means multisampling)
 *
 * @return number of samples per pixel
 */
GLint Renderbuffer::getSamples()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_SAMPLES);
}

/**
 * @brief get size of red channel 
 *
 * @return size of red channel in bytes
 */
GLint Renderbuffer::getRedSize()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_RED_SIZE);
}

/**
 * @brief get size of green channel 
 *
 * @return size of green channel in bytes
 */
GLint Renderbuffer::getGreenSize()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_GREEN_SIZE);
}

/**
 * @brief get size of blue channel 
 *
 * @return size of blue channel in bytes
 */
GLint Renderbuffer::getBlueSize()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_BLUE_SIZE);
}

/**
 * @brief get size of alpha channel 
 *
 * @return size of alpha channel in bytes
 */
GLint Renderbuffer::getAlphaSize()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_ALPHA_SIZE);
}

/**
 * @brief get size of depth channel 
 *
 * @return size of depth channel in bytes
 */
GLint Renderbuffer::getDepthSize()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_DEPTH_SIZE);
}

/**
 * @brief get size of stencil channel 
 *
 * @return size of stencil channel in bytes
 */
GLint Renderbuffer::getStencilSize()const{
  assert(this!=nullptr);
  return this->_getParamateri(GL_RENDERBUFFER_STENCIL_SIZE);
}

GLint Renderbuffer::_getParamateri(GLenum pname)const{
  assert(this!=nullptr);
  GLint param;
  this->_gl.glGetNamedRenderbufferParameteriv(this->_id,pname,&param);
  return param;
}


