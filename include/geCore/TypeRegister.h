#pragma once

#include<geCore/Export.h>
#include<geCore/dtemplates.h>
#include<iostream>
#include<vector>
#include<map>
#include<set>
#include<typeinfo>
#include<functional>

namespace ge{
  namespace core{
    class Accessor;
    class GECORE_EXPORT TypeRegister: public std::enable_shared_from_this<TypeRegister>{
      public:
        DEF_ENUM(Type,UNREGISTERED,VOID,BOOL,I8,I16,I32,I64,U8,U16,U32,U64,F32,F64,STRING,ARRAY,STRUCT,PTR,FCE,OBJ,TYPEID);
        typedef unsigned TypeID;
        typedef void(OBJConstructor)(signed   char*);
        typedef void(OBJDestructor )(unsigned char*);
      protected:
        struct POSITION{
          static const unsigned TYPE = 0u;
          struct ARRAY{
            static const unsigned SIZE       = 1u;
            static const unsigned INNER_TYPE = 2u;
          };
          struct STRUCT{
            static const unsigned NOF_ELEMENTS      = 1u;
            static const unsigned INNER_TYPES_START = 2u;
          };
          struct PTR{
            static const unsigned INNER_TYPE = 1u;
          };
          struct FCE{
            static const unsigned RETURN_TYPE     = 1u;
            static const unsigned NOF_ARGUMENTS   = 2u;
            static const unsigned ARGUMENTS_START = 3u;
          };
          struct OBJ{
            static const unsigned SIZE = 1u;
          };
        };

        std::vector<unsigned>       _typeStart;
        std::vector<unsigned>       _types    ;
        std::map<std::string,TypeID>_name2Id  ;
        std::map<TypeID,std::string>_id2name  ;
        std::map<TypeID,std::set<std::string>>_id2Synonyms;
        std::map<TypeID,std::function<OBJConstructor>>_id2Constructor;
        std::map<TypeID,std::function<OBJDestructor>>_id2Destructor;
        bool   _isNewTypeEqualTo(TypeID et,std::vector<unsigned>const&type,unsigned&start);
        bool   _typeExists      (TypeID*et,std::vector<unsigned>const&type,unsigned&start);
        TypeID _typeAdd         (          std::vector<unsigned>const&type,unsigned&start);
        bool   _incrCheck(unsigned size,unsigned&start);
        void   _bindTypeIdName(TypeID id,const char*name);
        void   _callConstructors(char*ptr,TypeID id)const;

        template<typename TO>
        TO _argsTo()const{
          return nullptr;
        }
        template<typename TO,typename... ARGS>
        TO _argsTo(TO to,ARGS... args)const{
          return to;
        }
        template<typename TO,typename NOTTO,typename... ARGS>
        typename std::enable_if<!std::is_same<TO,NOTTO>::value,TO>::type _argsTo(NOTTO,ARGS... args)const{
          return this->_argsTo<TO>(args...);
        }


        void _argsToVector(std::vector<unsigned>&){}
        template<typename... Args>
          void _argsToVector(std::vector<unsigned>&typeConfig,unsigned t,Args... args){
            typeConfig.push_back(t);
            this->_argsToVector(typeConfig,args...);
          }
        template<typename... Args>
          void _argsToVector(std::vector<unsigned>&typeConfig,const char*t,Args... args){
            typeConfig.push_back(this->getTypeId(t));
            this->_argsToVector(typeConfig,args...);
          }
        template<typename... Args>
          void _argsToVector(std::vector<unsigned>&typeConfig,std::function<OBJConstructor>,Args... args){
            this->_argsToVector(typeConfig,args...);
          }
        template<typename... Args>
          void _argsToVector(std::vector<unsigned>&typeConfig,std::function<OBJDestructor>,Args... args){
            this->_argsToVector(typeConfig,args...);
          }

      public:
        template<typename TYPE>
        static const char* getTypeKeyword();
        template<typename TYPE>
        static std::function<OBJConstructor> getConstructor(){
          return std::function<OBJConstructor>([](signed char*ptr){new(ptr)TYPE();});
        }
        template<typename TYPE>
        static std::function<OBJDestructor> getDestructor(){
          return std::function<OBJDestructor>([](unsigned char*ptr){((TYPE*)ptr)->~TYPE();});
        }

        TypeRegister();
        ~TypeRegister();
        unsigned   getNofTypes()const;
        TypeID     getTypeId               (unsigned index)const;
        unsigned   getIndex                (TypeID id)const;
        unsigned   getTypeDescriptionLength(TypeID id)const;
        unsigned   getTypeDescriptionElem  (TypeID id,unsigned i)const;
        Type       getTypeIdType           (TypeID id)const;
        Type       getElementType          (unsigned element)const;
        unsigned   getNofStructElements    (TypeID id)const;
        TypeID     getStructElementTypeId  (TypeID id,unsigned element)const;
        unsigned   getArraySize            (TypeID id)const;
        TypeID     getArrayInnerTypeId     (TypeID id)const;
        TypeID     getPtrInnerTypeId       (TypeID id)const;
        TypeID     getFceReturnTypeId      (TypeID id)const;
        unsigned   getNofFceArgs           (TypeID id)const;
        TypeID     getFceArgTypeId         (TypeID id,unsigned element)const;
        unsigned   getObjSize              (TypeID id)const;
        TypeID     getTypeId               (const char*name)const;
        std::string getTypeIdName          (TypeID id)const;
        std::set<std::string>const &getTypeIdSynonyms(TypeID id)const;
        bool                  hasSynonyms      (TypeID id)const;
        bool                  areSynonyms      (const char*name0,const char*name1)const;
        unsigned   computeTypeIdSize       (TypeID id)const;

        void*alloc(TypeID id)const;
        Accessor allocAccessor(TypeID id)const;
        Accessor allocAccessor(const char*name)const;
        std::shared_ptr<Accessor>sharedAccessor(TypeID id)const;
        std::shared_ptr<Accessor>sharedAccessor(const char*name)const;

        Accessor emptyAccessor(TypeID id)const;
        Accessor emptyAccessor(const char*name)const;
        std::shared_ptr<Accessor>sharedEmptyAccessor(TypeID id      ,std::function<OBJDestructor> destructor  = nullptr)const;
        std::shared_ptr<Accessor>sharedEmptyAccessor(const char*name,std::function<OBJDestructor> destructor  = nullptr)const;

        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessor(TypeID id,ARGS... args)const;
        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessor(const char*name,ARGS... args)const;
        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessor(ARGS... args)const;

        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessorAddCD(const char*name,ARGS... args);
        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessorAddC(const char*name,ARGS... args);
        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessorAddD(const char*name,ARGS... args);
        template<typename CLASS,typename... ARGS>
        inline std::shared_ptr<Accessor>sharedAccessorAdd(const char*name,ARGS... args);

        void destroyUsingCustomDestroyer    (unsigned char*ptr,TypeID id)const;
        void constructUsingCustomConstructor(  signed char*ptr,TypeID id)const;

        template<typename... Args>
          unsigned addType(const char* name,Args... args){
            std::vector<unsigned>typeConfig;
            this->_argsToVector(typeConfig,args...);
            return this->addType(name, typeConfig,this->_argsTo<std::function<OBJConstructor>>(args...),this->_argsTo<std::function<OBJDestructor>>(args...));
          }

        TypeID addType(
            const char*                   name                 ,
            std::vector<unsigned>const&   type                 ,
            std::function<OBJConstructor> constructor = nullptr,
            std::function<OBJDestructor > destructor  = nullptr);


        template<typename CLASS>
        TypeID addClassCD(const char*name){
          return this->addType(name,TypeRegister::OBJ,sizeof(CLASS),TypeRegister::getConstructor<CLASS>(),TypeRegister::getDestructor<CLASS>());
        }
        template<typename CLASS>
        TypeID addClassD(const char*name){
          return this->addType(name,TypeRegister::OBJ,sizeof(CLASS),TypeRegister::getDestructor<CLASS>());
        }
        template<typename CLASS>
        TypeID addClassC(const char*name){
          return this->addType(name,TypeRegister::OBJ,sizeof(CLASS),TypeRegister::getConstructor<CLASS>());
        }
        template<typename CLASS>
        TypeID addClass(const char*name){
          return this->addType(name,TypeRegister::OBJ,sizeof(CLASS));
        }

        void addConstructor(TypeID id,std::function<OBJConstructor> constructor = nullptr);
        void addDestructor (TypeID id,std::function<OBJDestructor > destructor  = nullptr);

        std::string toStr(TypeID id)const;
        std::string toStr()const;

        unsigned getNofDescriptionUints()const;
        unsigned getDescription(unsigned i)const;
    };

    class GECORE_EXPORT Accessor{
      protected:
        std::shared_ptr<const TypeRegister>_manager       = nullptr                   ;
        std::shared_ptr<char>              _data          = nullptr                   ;
        unsigned                           _offset        = 0u                        ;
        TypeRegister::TypeID               _id            = TypeRegister::UNREGISTERED;
        static void _callDestructors(char*ptr,TypeRegister::TypeID id,std::shared_ptr<const TypeRegister>const&manager);
      public:
        Accessor(Accessor const& ac);
        Accessor(
            std::shared_ptr<const TypeRegister>const&manager = nullptr                   ,
            const void*                              data    = nullptr                   ,
            TypeRegister::TypeID                     id      = TypeRegister::UNREGISTERED,
            unsigned                                 offset  = 0                         );
        Accessor(
            std::shared_ptr<const TypeRegister>const&manager                             ,
            std::shared_ptr<char>const&              data                                ,
            TypeRegister::TypeID                     id      = TypeRegister::UNREGISTERED,
            unsigned                                 offset  = 0                         );
        Accessor(
            std::shared_ptr<const TypeRegister>const&manager,
            TypeRegister::TypeID                     id     );
        template<typename TYPE>
          Accessor(TYPE val,std::shared_ptr<const TypeRegister>const&manager){
            this->_id      = manager->getTypeId(TypeRegister::getTypeKeyword<TYPE>());
            this->_manager = manager;
            ge::core::TypeRegister::TypeID id=this->_id;
            this->_data    = std::shared_ptr<char>((char*)manager->alloc(this->_id),[id,manager](char*ptr){Accessor::_callDestructors(ptr,id,manager);delete[]ptr;});
            this->_offset  = 0;
            (*this)=val;
          }
        ~Accessor();
        std::shared_ptr<const TypeRegister>getManager()const;
        void*getData()const;
        TypeRegister::TypeID getId()const;
        Accessor operator[](unsigned elem)const;
        unsigned getNofElements()const;
        const void*getPointer()const;
        std::string data2Str()const;
        template<typename T>
          Accessor& operator=(const T&data){
            *((T*)this->getData()) = data;
            return *this;
          }
        template<typename T>
          operator T&()const{
            return *((T*)this->getData());
          }
        template<typename T>
          operator T*()const{
            return (T*)this->getData();
          }
        template<typename T>
          operator T**()const{
            return (T**)&this->_data;
          }
        template<typename CLASS,typename... ARGS>
          void callConstructor(ARGS... args){
            new(this->getData())CLASS(args...);
          }
    };


    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessor(TypeID id,ARGS... args)const{
        //std::shared_ptr<ge::core::Accessor>ptr=std::shared_ptr<Accessor>(new Accessor(this->shared_from_this(),this->alloc(id),id),[](Accessor*ac){ac->free();delete ac;});
        std::shared_ptr<ge::core::Accessor>ptr=std::make_shared<Accessor>(this->shared_from_this(),this->alloc(id),id);
        ptr->callConstructor<CLASS>(args...);
        return ptr;
      }
    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessor(const char*name,ARGS... args)const{
        TypeID id=this->getTypeId(name);
        return this->sharedAccessor<CLASS>(id,args...);
      }
    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessor(ARGS... args)const{
        return this->sharedAccessor<CLASS>(this->getTypeKeyword<CLASS>(),args...);
      }

    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessorAddCD(const char*name,ARGS... args){
        this->addClassCD<CLASS>(name);
        return this->sharedAccessor<CLASS>(name,args...);
      }
    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessorAddC(const char*name,ARGS... args){
        this->addClassC<CLASS>(name);
        return this->sharedAccessor<CLASS>(name,args...);
      }
    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessorAddD(const char*name,ARGS... args){
        this->addClassD<CLASS>(name);
        return this->sharedAccessor<CLASS>(name,args...);
      }
    template<typename CLASS,typename... ARGS>
      inline std::shared_ptr<Accessor>TypeRegister::sharedAccessorAdd(const char*name,ARGS... args){
        this->addClass<CLASS>(name);
        return this->sharedAccessor<CLASS>(name,args...);
      }

  }
}

namespace ge{
  namespace core{
    template<typename TYPE>
      inline TYPE convertTo(std::shared_ptr<ge::core::Accessor> const&sharedAccessor){
        return (TYPE)*sharedAccessor;
      }

    template<typename TYPE>
      inline TYPE convertTo(ge::core::Accessor const&accessor){
        return (TYPE)accessor;
      }
    template<typename TYPE>
      inline TYPE convertTo(ge::core::Accessor* const&ptrAccessor){
        return (TYPE)*ptrAccessor;
      }

    template<typename TYPE>
      inline void convertFrom(std::shared_ptr<ge::core::Accessor>&output,TYPE const&input){
        *output=input;
      }

    template<typename TYPE>
      inline void convertFrom(ge::core::Accessor*&output,TYPE const&input){
        (TYPE&)(*output)=input;
      }

    template<typename TYPE>
      inline void convertFrom(ge::core::Accessor &output,TYPE const&input){
        ((TYPE&)output) = input;
      }

  }
}