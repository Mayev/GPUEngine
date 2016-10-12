#pragma once

#include<geGL/geGL.h>
#include"ShadowMethod.h"
#include"Model.h"
#include"FastAdjacency.h"
#include"TimeStamp.h"


class RSSV: public ShadowMethod{
  public:
    RSSV(
        glm::uvec2                      const&windowSize   ,
        std::shared_ptr<ge::gl::Texture>const&shadowMask   ,
        std::shared_ptr<ge::gl::Texture>const&depthTexture ,
        std::shared_ptr<Model>          const&model        ,
        size_t                          const&wavefrontSize
        );
    virtual ~RSSV();
    virtual void create(
        glm::vec4 const&lightPosition,
        glm::mat4 const&view         ,
        glm::mat4 const&projection   )override;
  protected:
    glm::uvec2                      _windowSize          = glm::uvec2(512,512);
    std::shared_ptr<ge::gl::Texture>_shadowMask          = nullptr            ;
    std::shared_ptr<ge::gl::Texture>_depthTexture        = nullptr            ;
    size_t                          _wavefrontSize       = 64                 ;
    std::shared_ptr<ge::gl::Buffer> _triangles           = nullptr            ;
    std::shared_ptr<ge::gl::Buffer> _adjacency           = nullptr            ;
    std::shared_ptr<ge::gl::Buffer> _silhouettes         = nullptr            ;
    size_t                          _nofEdges            = 0                  ;
    size_t                          _nofTriangles        = 0                  ;
    std::shared_ptr<ge::gl::Program>_computeSilhouettes  = nullptr            ;
    std::shared_ptr<ge::gl::Buffer> _dib                 = nullptr            ;
    std::shared_ptr<ge::gl::Program>_generateHDT0Program = nullptr            ;
    std::shared_ptr<ge::gl::Program>_generateHDT1Program = nullptr            ;
    std::shared_ptr<ge::gl::Program>_generateHDTProgram  = nullptr            ;
    glm::uvec2                      _generateHDT0WGS     = glm::uvec2(8,8)    ;
    size_t                          _nofLevels           = 1                  ;
    std::vector<std::shared_ptr<ge::gl::Texture>>_HDT;
    void _generateHDT();
};
