#include<geCore/ResourceFactory.h>
#include<geCore/functionRegister.h>

using namespace ge::core;

std::shared_ptr<Accessor>ResourceFactory::operator()(
    std::shared_ptr<FunctionRegister>const&fr){
  this->_first=this->_uses==0;
  std::shared_ptr<Accessor>res;
  if(!this->_result)
    this->_result = fr->getTypeRegister()->sharedAccessor(this->_type);
  res=this->_result;
  this->_uses++;
  if(this->_uses==this->_maxUses){
    this->_uses   = 0      ;
    this->_result = nullptr;
  }
  return res;
}


