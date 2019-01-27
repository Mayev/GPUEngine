#pragma once

#include <geGL/Program.h>
#include <geGL/Buffer.h>
#include <geGL/VertexArray.h>

#include "OctreeVisitor.hpp"
#include "GpuEdges.hpp"
#include "../TimeStamp.h"

enum class DrawingMethod : unsigned char
{
	GS = 0,
	TS = 1,
	CS = 2
};

class OctreeSidesDrawer
{
public:
	OctreeSidesDrawer(std::shared_ptr<OctreeVisitor> octreeVisitor, unsigned int subgroupSize, DrawingMethod potential, DrawingMethod silhouette);

	void init(std::shared_ptr<GpuEdges> gpuEdges);

	void drawSides(const glm::mat4& mvp,const glm::vec4& light);

	void setStamper(std::shared_ptr<TimeStamp> stamper);

protected:
	void _loadOctreeToGpu();

	void _drawSidesFromSilhouetteEdgesTS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);
	void _drawSidesFromSilhouetteEdgesGS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);
	void _drawSidesFromSilhouetteEdgesCS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);

	void _drawSidesFromPotentialEdgesGS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);
	void _drawSidesFromPotentialEdgesTS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);
	void _drawSidesFromPotentialEdgesCS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);

	void _drawSidesCS(const glm::mat4& mvp, const glm::vec4& lightPos, unsigned int cellContainingLightId);

	void _generateSidesFromPotentialCS(const glm::vec4& lightPos, unsigned cellContainingLightId);
	void _generateSidesFromSilhouetteCS(const glm::vec4& lightPos, unsigned cellContainingLightId);

	void _initShaders();
	void _initBuffers();
		void _getMaxPossibleEdgeCountInTraversal(size_t& potentialPath, size_t& silhouettePath, size_t& maxInVoxel, size_t& maxPath) const;
		void _getMaximumNofEdgesInLevel(unsigned int level, size_t& potential, size_t& silhouette) const;
			void _getNofEdgesInNode(unsigned int nodeId, size_t& potential, size_t& silhouette) const;

	bool _generateLoadGpuTraversalShader3();
	void _getPotentialSilhouetteEdgesGpu3(unsigned int lowestNodeContainingLight);

	void _processSubBuffer(
		const std::unordered_map<BitmaskType, std::vector<uint32_t>>::value_type& subBuffer, 
		std::vector<uint32_t>& compressedNodesInfo,
		std::vector<uint32_t>& nofEdgesPrefixSums,
		uint32_t* gpuMappedBuffer
		);

	void _breakCompressionIdToUintsAndPush(const BitmaskType& id, std::vector<uint32_t>& vectorToStore) const;

	void _allocSubBuffersDataBuffers();

	void _calcPrefixSums();
		void _calcPrefixSum(std::shared_ptr<ge::gl::Buffer> input, std::shared_ptr<ge::gl::Buffer> output, std::shared_ptr<ge::gl::Buffer> count, unsigned int worstCase, unsigned int tmpIndex);

	void _calcSingleTwoLevelPrefixSumWgSizes(unsigned int maxNofPotSubBuffs, unsigned int maxNofSilSubBuffs);
		unsigned int _getNearstLowerPow2(unsigned int x) const;

private:	
	int _lastFrameCellIndex = -1;

	std::shared_ptr<ge::gl::Program> _potentialGsProgram = nullptr;
	std::shared_ptr<ge::gl::Program> _potentialTsProgram = nullptr;
	std::shared_ptr<ge::gl::Program> _silhouetteSidesTsProgram = nullptr;
	std::shared_ptr<ge::gl::Program> _silhouetteSidesGsProgram = nullptr;
	std::shared_ptr<ge::gl::Program> _drawSidesProgram = nullptr;
	std::shared_ptr<ge::gl::Program> _testAndGenerateSidesProgram = nullptr;
	std::shared_ptr<ge::gl::Program> _generateSidesProgram = nullptr;

	std::shared_ptr<ge::gl::VertexArray>_dummyVAO = nullptr;
	std::shared_ptr<ge::gl::VertexArray>_potentialSidesCsVAO = nullptr;
	std::shared_ptr<ge::gl::VertexArray>_silhouetteSidesCsVAO = nullptr;

	std::shared_ptr<ge::gl::Buffer> _edgesIdsToGenerate = nullptr;
	std::shared_ptr<ge::gl::Buffer> _edgesIdsToTestAndGenerate = nullptr;

	std::shared_ptr<ge::gl::Buffer> _potentialEdgeCsVBO = nullptr;
	std::shared_ptr<ge::gl::Buffer> _silhouetteEdgeCsVBO = nullptr;

	std::shared_ptr<ge::gl::Buffer> _indirectDrawBufferPotentialCS = nullptr;
	std::shared_ptr<ge::gl::Buffer> _indirectDrawBufferSilhouetteCS = nullptr;

	std::shared_ptr<ge::gl::Buffer> _voxelEdgeIndices = nullptr;
	std::shared_ptr<ge::gl::Buffer> _voxelNofPotentialSilhouetteEdgesPrefixSum = nullptr;

	std::shared_ptr<ge::gl::Buffer> _indirectDispatchCsPotential = nullptr;
	std::shared_ptr<ge::gl::Buffer> _indirectDispatchCsSilhouette = nullptr;

	std::shared_ptr<ge::gl::Buffer> _compressedNodesInfoBuffer = nullptr;
	std::shared_ptr<ge::gl::Buffer> _compressedNodesInfoIndexingBuffer = nullptr;

	std::shared_ptr<GpuEdges> _gpuEdges;

	std::shared_ptr<OctreeVisitor> _octreeVisitor = nullptr;

	std::vector<std::shared_ptr<ge::gl::Buffer>> _gpuOctreeBuffers;
	std::vector<uint32_t> _lastNodePerEdgeBuffer;

	uint32_t _nofPotentialEdgesToDraw = 0;
	uint32_t _nofSilhouetteEdgesToDraw = 0;

	uint32_t _maxNofEdgesInVoxel;
	uint32_t _maxNofEdgesPath;

	uint32_t _workgroupSize = 0;

	DrawingMethod _potentialDrawingMethod;
	DrawingMethod _silhouetteDrawingMethod;

	std::shared_ptr<TimeStamp> _timer;

	std::shared_ptr<ge::gl::Program> m_subBuffersPreprocessShader = nullptr;
	std::shared_ptr<ge::gl::Program> m_getDataFromPrecomputedBuffersShader2 = nullptr;

	std::shared_ptr<ge::gl::Buffer> m_potSuBuffers = nullptr;
	std::shared_ptr<ge::gl::Buffer> m_silSuBuffers = nullptr;
	std::shared_ptr<ge::gl::Buffer> m_nofPotBuffers = nullptr;
	std::shared_ptr<ge::gl::Buffer> m_nofSilBuffers = nullptr;

	//Prefix sum
	std::shared_ptr<ge::gl::Buffer> m_prefixSumTmp[2] = { nullptr, nullptr };
	std::shared_ptr<ge::gl::Buffer> m_prefixSumTmpAtomic[2] = { nullptr, nullptr };

	std::shared_ptr<ge::gl::Program> m_prefixSumShaderSimple = nullptr;
	std::shared_ptr<ge::gl::Program> m_prefixSumShaderTwoLevel = nullptr;
	std::shared_ptr<ge::gl::Program> m_prefixSumShaderAdd = nullptr;

	unsigned int m_dataStride = 0;
	unsigned int m_maxNofPotBuffers = 0;
	unsigned int m_maxNofSilBuffers = 0;
	unsigned int m_prefixSumWorkgroupSizeSingle = 0;
	unsigned int m_prefixSumWorkgroupSizeTwoLevel = 0;
	unsigned int m_prefixSumWorkgroupSizeSummation = 256;

	unsigned int m_maxNofSubBuffersPath = 0;
};