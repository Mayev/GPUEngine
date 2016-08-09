#include<geGL/Sampler.h>

using namespace ge::gl;

Sampler::Sampler(){
  assert(this!=nullptr);
  this->_gl.glCreateSamplers(1,&this->_id);
}

Sampler::Sampler(Sampler*sampler){
  assert(this!=nullptr);
  this->_gl.glCreateSamplers(1,&this->_id);
  GLfloat borderColor[4];
  sampler->getBorderColor(borderColor);
  this->setBorderColor(borderColor);

  GLenum compareFunc = sampler->getCompareFunc();
  this->setCompareFunc(compareFunc);

  GLenum compareMode = sampler->getCompareMode();
  this->setCompareMode(compareMode);

  GLfloat lodBias = sampler->getLodBias();
  this->setLodBias(lodBias);

  GLfloat minLod = sampler->getMinLod();
  this->setMinLod(minLod);

  GLfloat maxLod = sampler->getMaxLod();
  this->setMinLod(maxLod);

  GLenum minFilter =  sampler->getMinFilter();
  this->setMinFilter(minFilter);

  GLenum magFilter = sampler->getMagFilter();
  this->setMagFilter(magFilter);

  GLenum wrapS = sampler->getWrapS();
  this->setWrapS(wrapS);

  GLenum wrapT = sampler->getWrapT();
  this->setWrapS(wrapT);

  GLenum wrapR = sampler->getWrapR();
  this->setWrapS(wrapR);
}

Sampler::Sampler (
    FunctionTablePointer const&table):OpenGLObject(table){
  assert(this!=nullptr);
  this->_gl.glCreateSamplers(1,&this->_id);
}

Sampler::Sampler(
    FunctionTablePointer const&table,
    Sampler*sampler):OpenGLObject(table){
  assert(this!=nullptr);
  this->_gl.glCreateSamplers(1,&this->_id);
  GLfloat borderColor[4];
  sampler->getBorderColor(borderColor);
  this->setBorderColor(borderColor);

  GLenum compareFunc = sampler->getCompareFunc();
  this->setCompareFunc(compareFunc);

  GLenum compareMode = sampler->getCompareMode();
  this->setCompareMode(compareMode);

  GLfloat lodBias = sampler->getLodBias();
  this->setLodBias(lodBias);

  GLfloat minLod = sampler->getMinLod();
  this->setMinLod(minLod);

  GLfloat maxLod = sampler->getMaxLod();
  this->setMinLod(maxLod);

  GLenum minFilter =  sampler->getMinFilter();
  this->setMinFilter(minFilter);

  GLenum magFilter = sampler->getMagFilter();
  this->setMagFilter(magFilter);

  GLenum wrapS = sampler->getWrapS();
  this->setWrapS(wrapS);

  GLenum wrapT = sampler->getWrapT();
  this->setWrapS(wrapT);

  GLenum wrapR = sampler->getWrapR();
  this->setWrapS(wrapR);

}

Sampler::~Sampler(){
  assert(this!=nullptr);
  this->_gl.glDeleteSamplers(1,&this->_id);
}

void Sampler::setBorderColor(GLfloat*color    )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameterfv(this->_id,GL_TEXTURE_BORDER_COLOR,color);
}

void Sampler::setCompareFunc(GLenum  func     )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_COMPARE_FUNC,func);
}

void Sampler::setCompareMode(GLenum  mode     )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_COMPARE_MODE,mode);
}

void Sampler::setLodBias    (GLfloat lodBias  )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameterf(this->_id,GL_TEXTURE_LOD_BIAS,lodBias);
}

void Sampler::setMinLod     (GLfloat minLod   )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameterf(this->_id,GL_TEXTURE_MIN_LOD,minLod);
}

void Sampler::setMaxLod     (GLfloat maxLod   )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameterf(this->_id,GL_TEXTURE_MAX_LOD,maxLod);
}

void Sampler::setMinFilter  (GLenum  minFilter)const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_MIN_FILTER,minFilter);
}

void Sampler::setMagFilter  (GLenum  magFilter)const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_MAG_FILTER,magFilter);
}

void Sampler::setWrapS      (GLenum  wrapS    )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_WRAP_S,wrapS);
}

void Sampler::setWrapT      (GLenum  wrapT    )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_WRAP_T,wrapT);
}

void Sampler::setWrapR      (GLenum  wrapR    )const{
  assert(this!=nullptr);
  this->_gl.glSamplerParameteri(this->_id,GL_TEXTURE_WRAP_R,wrapR);
}

void Sampler::getBorderColor(GLfloat*color    )const{
  assert(this!=nullptr);
  this->_gl.glGetSamplerParameterfv(this->_id,GL_TEXTURE_BORDER_COLOR,color);
}

GLenum Sampler::getCompareFunc()const{
  assert(this!=nullptr);
  GLenum func;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_COMPARE_FUNC,(GLint*)&func);
  return func;
}

GLenum Sampler::getCompareMode()const{
  assert(this!=nullptr);
  GLenum mode;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_COMPARE_MODE,(GLint*)&mode);
  return mode;
}

GLfloat Sampler::getLodBias()const{
  assert(this!=nullptr);
  GLfloat lodBias;
  this->_gl.glGetSamplerParameterfv(this->_id,GL_TEXTURE_LOD_BIAS,&lodBias);
  return lodBias;
}

GLfloat Sampler::getMinLod()const{
  assert(this!=nullptr);
  GLfloat minLod;
  this->_gl.glGetSamplerParameterfv(this->_id,GL_TEXTURE_MIN_LOD,&minLod);
  return minLod;
}

GLfloat Sampler::getMaxLod()const{
  assert(this!=nullptr);
  GLfloat maxLod;
  this->_gl.glGetSamplerParameterfv(this->_id,GL_TEXTURE_MAX_LOD,&maxLod);
  return maxLod;
}

GLenum Sampler::getMinFilter()const{
  assert(this!=nullptr);
  GLenum minFilter;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_MIN_FILTER,(GLint*)&minFilter);
  return minFilter;
}

GLenum Sampler::getMagFilter()const{
  assert(this!=nullptr);
  GLenum magFilter;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_MAG_FILTER,(GLint*)&magFilter);
  return magFilter;
}

GLenum Sampler::getWrapS()const{
  assert(this!=nullptr);
  GLenum wrapS;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_WRAP_S,(GLint*)&wrapS);
  return wrapS;
}

GLenum Sampler::getWrapT()const{
  assert(this!=nullptr);
  GLenum wrapT;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_WRAP_T,(GLint*)&wrapT);
  return wrapT;
}

GLenum Sampler::getWrapR()const{
  assert(this!=nullptr);
  GLenum wrapR;
  this->_gl.glGetSamplerParameteriv(this->_id,GL_TEXTURE_WRAP_R,(GLint*)&wrapR);
  return wrapR;
}

void Sampler::bind(GLuint unit)const{
  assert(this!=nullptr);
  this->_gl.glBindSampler(unit,this->_id);
}

void Sampler::unbind(GLuint unit)const{
  assert(this!=nullptr);
  this->_gl.glBindSampler(unit,0);
}
