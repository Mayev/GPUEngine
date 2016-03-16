#ifndef GE_RG_MODEL_H
#define GE_RG_MODEL_H

#include <list>
#include <memory>
#include <geCore/idof.h>
#include <geRG/Export.h>
#include <geRG/Basics.h>

namespace ge
{
   namespace rg
   {
      class Mesh;
      class Transformation;


      class GERG_EXPORT Model {
      public:

         typedef std::list<std::shared_ptr<Model>> ModelList;
         typedef std::list<std::shared_ptr<Mesh>> MeshList;
         typedef std::map<idof_t,std::shared_ptr<Transformation>> TransformationRootList;

      protected:

         ModelList _submodelList;
         MeshList _meshList;
         TransformationRootList _transformationRootList;

      public:

         inline MeshList& meshList();
         inline const MeshList& meshList() const;
         inline MeshList::iterator addMesh(const std::shared_ptr<Mesh>& mesh);
         inline void removeMesh(MeshList::iterator it);

         inline const std::shared_ptr<Transformation>& transformationRoot(
               idof_t name=idof(scene,TransformationRoot)) const;
         inline TransformationRootList& transformationRootList();
         inline const TransformationRootList& transformationRootList() const;
         inline TransformationRootList::iterator addTransformationRoot(idof_t name,const std::shared_ptr<Transformation>& t);
         inline void removeTransformationRoot(TransformationRootList::iterator it);
         inline void removeTransformationRoot(idof_t name);

      };

   }
}



// inline methods

namespace ge
{
   namespace rg
   {
      inline const Model::MeshList& Model::meshList() const  { return _meshList; }
      inline Model::MeshList& Model::meshList()  { return _meshList; }
      inline Model::MeshList::iterator Model::addMesh(const std::shared_ptr<Mesh>& mesh)  { return _meshList.emplace(_meshList.end(),mesh); }
      inline void Model::removeMesh(Model::MeshList::iterator it)  { _meshList.erase(it); }

      inline const std::shared_ptr<Transformation>& Model::transformationRoot(idof_t name) const  { auto it=_transformationRootList.find(name); return (it!=_transformationRootList.end())?it->second:nullSharedPtr<Transformation>(); }
      inline Model::TransformationRootList& Model::transformationRootList()  { return _transformationRootList; }
      inline const Model::TransformationRootList& Model::transformationRootList() const  { return _transformationRootList; }
      inline Model::TransformationRootList::iterator Model::addTransformationRoot(idof_t name,const std::shared_ptr<Transformation>& t)  { auto r=_transformationRootList.emplace(std::make_pair(name,t)); if(!r.second) (r.first->second)=t; return r.first; }
      inline void Model::removeTransformationRoot(Model::TransformationRootList::iterator it)  { _transformationRootList.erase(it); }
      inline void Model::removeTransformationRoot(idof_t name)  { _transformationRootList.erase(name); }
   }
}

#endif /* GE_RG_MODEL_H */