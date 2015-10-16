#include<geUtil/copyArgumentManager2Namespace.h>
#include<geCore/TypeRegister.h>

void ArgData2TypeDescriptor(std::vector<unsigned>&descriptor,ge::util::ArgData*data){
  ge::util::ArgData::Type type=data->getType();
  switch(type){
    case ge::util::ArgData::BOOL:
      descriptor.push_back(ge::core::TypeRegister::BOOL);
      return;
    case ge::util::ArgData::I8:
      descriptor.push_back(ge::core::TypeRegister::I8);
      return;
    case ge::util::ArgData::I16:
      descriptor.push_back(ge::core::TypeRegister::I16);
      return;
    case ge::util::ArgData::I32:
      descriptor.push_back(ge::core::TypeRegister::I32);
      return;
    case ge::util::ArgData::I64:
      descriptor.push_back(ge::core::TypeRegister::I64);
      return;
    case ge::util::ArgData::U8:
      descriptor.push_back(ge::core::TypeRegister::U8);
      return;
    case ge::util::ArgData::U16:
      descriptor.push_back(ge::core::TypeRegister::U16);
      return;
    case ge::util::ArgData::U32:
      descriptor.push_back(ge::core::TypeRegister::U32);
      return;
    case ge::util::ArgData::U64:
      descriptor.push_back(ge::core::TypeRegister::U64);
      return;
    case ge::util::ArgData::F32:
      descriptor.push_back(ge::core::TypeRegister::F32);
      return;
    case ge::util::ArgData::F64:
      descriptor.push_back(ge::core::TypeRegister::F64);
      return;
    case ge::util::ArgData::STRING:
      descriptor.push_back(ge::core::TypeRegister::STRING);
      return;
    case ge::util::ArgData::ARRAY:
      descriptor.push_back(ge::core::TypeRegister::ARRAY);
      descriptor.push_back(((ge::util::ArrayData*)data)->size());
      ArgData2TypeDescriptor(descriptor,(*((ge::util::ArrayData*)data))[0]);
      return;
    case ge::util::ArgData::STRUCT:
      descriptor.push_back(ge::core::TypeRegister::STRUCT);
      descriptor.push_back(((ge::util::StructData*)data)->size());
      for(unsigned i=0;i<((ge::util::StructData*)data)->size();++i)
        ArgData2TypeDescriptor(descriptor,(*((ge::util::StructData*)data))[i]);
      return;
  }
}

void ArgData2Accessor(ge::core::Accessor ac,ge::util::ArgData*data){
  ge::util::ArgData::Type type=data->getType();
  switch(type){
    case ge::util::ArgData::BOOL:
      ac=((ge::util::BaseData<bool>*)data)->get();
      return;
    case ge::util::ArgData::I8:
      ac=((ge::util::BaseData<char>*)data)->get();
      return;
    case ge::util::ArgData::I16:
      ac=((ge::util::BaseData<short>*)data)->get();
      return;
    case ge::util::ArgData::I32:
      ac=((ge::util::BaseData<int>*)data)->get();
      return;
    case ge::util::ArgData::I64:
      ac=((ge::util::BaseData<int long long>*)data)->get();
      return;
    case ge::util::ArgData::U8:
      ac=((ge::util::BaseData<unsigned char>*)data)->get();
      return;
    case ge::util::ArgData::U16:
      ac=((ge::util::BaseData<unsigned short>*)data)->get();
      return;
    case ge::util::ArgData::U32:
      ac=((ge::util::BaseData<unsigned int>*)data)->get();
      return;
    case ge::util::ArgData::U64:
      ac=((ge::util::BaseData<unsigned long long>*)data)->get();
      return;
    case ge::util::ArgData::F32:
      ac=((ge::util::BaseData<float>*)data)->get();
      return;
    case ge::util::ArgData::F64:
      ac=((ge::util::BaseData<double>*)data)->get();
      return;
    case ge::util::ArgData::STRING:
      ac=((ge::util::BaseData<std::string>*)data)->get();
      return;
    case ge::util::ArgData::ARRAY:
      for(unsigned i=0;i<((ge::util::ArrayData*)data)->size();++i)
        ArgData2Accessor(ac[i],(*((ge::util::ArrayData*)data))[i]);
      return;
    case ge::util::ArgData::STRUCT:
      for(unsigned i=0;i<((ge::util::StructData*)data)->size();++i)
        ArgData2Accessor(ac[i],(*((ge::util::StructData*)data))[i]);
      return;
  }
}

void ge::util::sim::copyArgumentManager2Namespace(std::shared_ptr<ge::util::sim::Namespace>const&ns,ge::util::ArgumentManager*argm,std::shared_ptr<ge::core::TypeRegister>&typeRegister){
  for(auto x:*argm){
    std::vector<unsigned>typeDescriptor;
    ArgData2TypeDescriptor(typeDescriptor,x.second);
    //std::shared_ptr<ge::core::Accessor>sac=std::shared_ptr<ge::core::Accessor>(new ge::core::Accessor(typeRegister->allocAccessor(typeRegister->addType(/*ss.str().c_str()*/"",typeDescriptor))),[](ge::core::Accessor*ac){ac->free();delete ac;});
//    std::shared_ptr<ge::core::Accessor>sac=std::make_shared<ge::core::Accessor>(typeRegister->allocAccessor(typeRegister->addType("",typeDescriptor)));
    std::shared_ptr<ge::core::Accessor>sac=typeRegister->sharedAccessor(typeRegister->addType("",typeDescriptor));
    ArgData2Accessor(*sac,x.second);
    ns->insert(x.first,sac);
  }
}
