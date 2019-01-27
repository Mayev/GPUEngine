#pragma once
#include <string>
#include <iostream>
#include <sstream>

const std::string traversalSupportFunctions = R".(

uint getNumNodesInPreviousLevels(int level)
{
	const int l = level - 1;

	if (l < 0 || l> int(treeDepth))
		return 0;

	return levelSizesInclusiveSum[l];
}

int getNodeIdInLevel(uint nodeID, uint level)
{
	return int(nodeID) - int(getNumNodesInPreviousLevels(int(level)));
}

int getNodeParent(uint nodeID, uint nodeLevel)
{
	if (nodeID == 0)
		return -1;

	const int idInLevel = getNodeIdInLevel(nodeID, nodeLevel);
	const int parentRelativeID = idInLevel / 8;

	return parentRelativeID + int(getNumNodesInPreviousLevels(int(nodeLevel) - 1));
}

int getChildrenStartingId(uint nodeID, unsigned int nodeLevel)
{
	const int idInLevel = getNodeIdInLevel(nodeID, nodeLevel);
	
	return 8*idInLevel + int(levelSizesInclusiveSum[nodeLevel]);
}
).";

//In iterative manner
//Not expecting to have a lot of buffers
const std::string genGetNodeFromBuffer(unsigned int numBuffers)
{
	std::stringstream str;

	str << "uint getNodeFromBuffer(uint nodeIndex, uint bufferId)\n";
	str << "{\n";

	for(unsigned int i = 0; i<numBuffers; ++i)
	{
		str << "\tif(bufferId==" << i << ") return edges" << i << "[nodeIndex];\n";
	}
	str << "\treturn 0;\n";
	str << "}\n\n";

	return str.str();
}

std::string genBuffer(unsigned int i)
{
	std::stringstream str;
	str << "layout(std430, binding = " << i << ") readonly buffer _edges" << i << "{uint edges" << i << "[]; };\n";
	return str.str();
}

std::string genFindBufferFunc()
{
	std::stringstream str;
	str << "uint getNodeBufferIndex(uint nodeId)\n{\n";
	str << "\tuint index = 0;\n";
	str << "\twhile(nodeId>edgeBuffersMapping[index]) ++index;\n";
	str << "\treturn index;\n}\n";

	return str.str();
}

//Nie je uplne dokonala, neriesi ze "num" moze byt nula
const std::string findLsbFunction = R".(
int getLSB64(uint64_t num)
{
	const uvec2 v = unpackUint2x32(num);
	const int lowPos = findLSB(v.x);
	
	return (lowPos<0) ? 32 + findLSB(v.y) : lowPos;
}

).";

std::string getCompressionIdWithinParentFunction(unsigned int compressionLevel)
{
	const std::string c1 = R".(
uint getCompressionIdWithinParent(uint nodeId, uint compressionParentId, uint compressionParentLevel)
{
	return nodeId - uint(getChildrenStartingId(compressionParentId, compressionParentLevel));
}
).";

	const std::string c2 = R".(
uint getCompressionIdWithinParent(uint nodeId, uint compressionParentId, uint compressionParentLevel)
{
	const uint depth = treeDepth - compressionParentLevel;
		if(depth!=COMPRESSION_LEVEL)
		return 0;
		uint startingNode = compressionParentId;
	
	for(uint i = 0; i<depth; ++i)
		startingNode = getChildrenStartingId(startingNode, compressionParentLevel-i);
	
	return nodeId - uint(startingNode);
}
).";

	if (compressionLevel == 1)
		return c1;

	return c2;
}

std::string isCompressionIdPresentFunction(unsigned int compressionLevel)
{
	//Assumption is that id is 8-bit
	const std::string c1 = R".(
bool isCompressionIdPresent(uint nodeCompressionId, const uint startingIndexToCompressionId, uint currentLevel)
{
	return (currentLevel!=(treeDepth-COMPRESSION_LEVEL)) || ((nodeInfo[startingIndexToCompressionId + 0] & (1 << nodeCompressionId)) != 0);
}
).";

	//id is 64 bit and more
	const std::string c2 = R".(
bool isCompressionIdPresent(const uint nodeCompressionId, const uint startingIndexToCompressionId, uint currentLevel)
{
	if(currentLevel == (treeDepth-COMPRESSION_LEVEL))
	{
		const uint index = nodeCompressionId / 32;
		const uint compressionIndex = nodeCompressionId % 32;
	
		return (nodeInfo[startingIndexToCompressionId + index] & (1 << compressionIndex))!=0;
	}
	else
		return true;
}
).";

	if (compressionLevel == 1)
		return c1;

	return c2;
}

std::string genPrologue(unsigned int workgroupSize)
{
	std::stringstream str;
	str << "#version 450 core\n";
	str << "#extension GL_ARB_shader_ballot : enable\n";
	str << "#extension GL_ARB_gpu_shader_int64 : enable\n";
	str << "#extension GL_AMD_gpu_shader_int64 : enable\n\n";
	str << "layout(local_size_x=" << workgroupSize << ") in;\n\n";

	return str.str();
}

std::string genGetSubBufferId()
{
	const unsigned int tmp = BitmaskTypeSizeBits / 32;
	const unsigned int nofUints = tmp ? tmp : 1;

	std::stringstream str;

	str << "uint[NOF_UINTS_PER_COMPRESSION_ID] getBufferId(uint subBufferIndex)\n";
	str << "{\n";
	str << "	uint retval[NOF_UINTS_PER_COMPRESSION_ID];\n";

	for (unsigned int i = 0; i < nofUints; ++i)
	{
		str << "	retval[" << i << "] = nodeInfo[shm_startingIndexToNodeInfo + NOF_UINTS_PER_NOF_SUBBUFFERS + NOF_UINTS_PER_SUBBUFFER_INFO*(subBufferIndex) + " << i << "];\n";
	}

	str << "	return retval;\n";
	str << "}\n\n";

	return str.str();
}

std::string genStoreBufferID(unsigned int nofBuffers)
{
	std::stringstream str;

	str << "void storeBufferInfo(\n";
	str << "	uint64_t voteMask, \n";
	str << "	bool storePotential, \n";
	str << "    unsigned int startingBufferIndex, \n";
	if (nofBuffers>1)
		str << "	unsigned int bufferIndex,\n";
	str << "    unsigned int bufferSize)\n";
	str << "{\n";
	str << "	const uvec2 maskUnpack = unpackUint2x32(voteMask);\n";
	str << "	const uint numBuffers = bitCount(maskUnpack.x) + bitCount(maskUnpack.y);\n";
	str << "	const int firstSet = getLSB64(voteMask);\n";
	str << "	const uint64_t andMask = (uint64_t(1) << gl_SubGroupInvocationARB) - uint64_t(1);\n";
	str << "	const uvec2 maskIdxPacked = unpackUint2x32(voteMask & andMask);\n";
	str << "	const uint idxInWarp = bitCount(maskIdxPacked.x) + bitCount(maskIdxPacked.y);\n";
	str << "	unsigned int storeIndex;\n";
	str << "	\n";
	str << "	if (gl_SubGroupInvocationARB == firstSet)\n";
	str << "	{\n";
	str << "		if (storePotential)\n";
	str << "		{\n";
	str << "			storeIndex = atomicAdd(nofPotBuffers, numBuffers); \n";
	str << "		}\n";
	str << "	\n";
	str << "		if (!storePotential)\n";
	str << "		{\n";
	str << "			storeIndex = atomicAdd(nofSilBuffers, numBuffers);\n";
	str << "		}\n";
	str << "	}\n";
	str << "	\n";
	str << "	storeIndex = NOF_UINTS_BUFFER_INFO*(readInvocationARB(storeIndex, firstSet) + idxInWarp);\n";
	str << "	\n";
	str << "	if (storePotential)\n";
	str << "	{\n";
	str << "		potentialBuffers[storeIndex + 0] = bufferSize;\n";
	str << "		potentialBuffers[storeIndex + 1] = startingBufferIndex;\n";
	if (nofBuffers>1)
		str << "		potentialBuffers[storeIndex + 2] = bufferIndex;\n";
	str << "		atomicAdd(nofPotEdges, bufferSize);\n";
	str << "	}\n";
	str << "	else\n";
	str << "	{\n";
	str << "		silhouetteBuffers[storeIndex + 0] = bufferSize;\n";
	str << "		silhouetteBuffers[storeIndex + 1] = startingBufferIndex;\n";
	if (nofBuffers>1)
		str << "		silhouetteBuffers[storeIndex + 2] = bufferIndex;\n";
	str << "		atomicAdd(nofSilEdges, bufferSize);\n";
	str << "	}\n";
	str << "}\n";

	return str.str();
}

std::string genBufferPreprocessShader(const std::vector<uint32_t>& lastNodePerEdgeBuffer, std::shared_ptr<Octree> octree, unsigned int workgroupSize)
{
	const unsigned int numBuffers = unsigned int(lastNodePerEdgeBuffer.size());
	const unsigned int compressionLevel = unsigned int(octree->getCompressionLevel());

	std::stringstream str;

	str << genPrologue(workgroupSize);

	str << "\n";

	unsigned int currentIndex = 0;

	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _nofEdgesPrefixSum{ uint nofEdgesPrefixSum[]; };\n";

	str << "layout(std430, binding = " << currentIndex++ << ") buffer _potential{ uint potentialBuffers[]; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _silhouette{ uint silhouetteBuffers[]; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _nofPotBuffers{ uint nofPotBuffers; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _nofSilBuffers{ uint nofSilBuffers; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _nofPotEdges{ uint nofPotEdges; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _nofSilEdges{ uint nofSilEdges; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _nodeInfoBuffer {uint nodeInfo[]; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _nodeInfoBufferIndexing {uint nodeInfoIndexing[]; };\n";

	str << "uniform uint cellContainingLight;\n\n";

	str << "const uint treeDepth = " << octree->getDeepestLevel() << ";\n";
	str << "#define COMPRESSION_LEVEL " << compressionLevel << "\n";
	const unsigned int tmpNofUints = ipow(OCTREE_NUM_CHILDREN, compressionLevel) / 32;
	const unsigned int nofUintsPerCompressionId = tmpNofUints ? tmpNofUints : 1;
	const unsigned int nofUintsPerNodeInfo = nofUintsPerCompressionId + 1;
	str << "#define NOF_UINTS_PER_COMPRESSION_ID " << nofUintsPerCompressionId << "\n";
	str << "#define NOF_UINTS_PER_SUBBUFFER_INFO " << nofUintsPerNodeInfo << "\n";
	str << "#define NOF_UINTS_PER_NOF_SUBBUFFERS 2\n\n";

	str << "const uint levelSizesInclusiveSum[" << octree->getDeepestLevel() + 1 << "] = uint[" << octree->getDeepestLevel() + 1 << "](";
	const std::vector<unsigned int> ls = octree->getLevelSizeInclusiveSum();
	for (unsigned int i = 0; i < ls.size(); ++i)
	{
		str << ls[i];
		if (i != (ls.size() - 1))
			str << ", ";
	}
	str << ");\n";

	if (numBuffers > 1)
	{
		str << "const uint edgeBuffersMapping[" << numBuffers << "] = uint[" << numBuffers << "](";
		for (unsigned int i = 0; i < numBuffers; ++i)
		{
			str << lastNodePerEdgeBuffer[i];
			if (i != (numBuffers - 1))
				str << ", ";
		}
		str << ");\n";
	}

	if (numBuffers > 1)
	{
		str << genFindBufferFunc();
	}

	const unsigned int nofUintsPerSubbufferInfo = (numBuffers > 1) ? 4 : 2;
	str << "#define NOF_UINTS_BUFFER_INFO " << nofUintsPerSubbufferInfo << "\n";

	str << traversalSupportFunctions;

	str << findLsbFunction;

	str << genStoreBufferID(numBuffers);

	str << getCompressionIdWithinParentFunction(octree->getCompressionLevel());
	str << isCompressionIdPresentFunction(octree->getCompressionLevel());

	//Shared memory
	str << "shared uint shm_nofPotSilBuffers[2];\n";
	str << "shared uint shm_startingIndexToNodeInfo;\n\n";

	str << genGetSubBufferId();

	//main
	str << "void main()\n";
	str << "{\n";
	str << "	int currentNode = int(cellContainingLight);\n";
	str << "	for(int currentLevel = int(treeDepth); currentLevel >=0; --currentLevel)\n";
	str << "	{\n";
	str << "		//First in warp - get info\n";
	str << "		if(gl_LocalInvocationIndex==0)\n";
	str << "		{\n";
	str << "			shm_startingIndexToNodeInfo = nodeInfoIndexing[currentNode];\n";
	str << "			shm_nofPotSilBuffers[0] = nodeInfo[shm_startingIndexToNodeInfo + 0];\n";
	str << "			shm_nofPotSilBuffers[1] = nodeInfo[shm_startingIndexToNodeInfo + 1];\n";
	str << "		}\n";
	str << "\n";
	str << "		barrier();\n";
	str << "\n";
	str << "		//Check if pot/sil\n";
	str << "		const uint nofPot = shm_nofPotSilBuffers[0];\n";
	str << "		const uint nofSil = shm_nofPotSilBuffers[1];\n";
	str << "		const bool isActive = gl_GlobalInvocationID.x < (nofPot + nofSil);\n";
	str << "		const bool isPotential = gl_GlobalInvocationID.x < nofPot;\n";
	str << "\n";
	str << "		if(isActive)\n";
	str << "		{\n";
	str << "			const uint compressionId = getCompressionIdWithinParent(cellContainingLight, currentNode, currentLevel);\n";
	str << "\n";
	str << "			if (isCompressionIdPresent(compressionId, shm_startingIndexToNodeInfo + NOF_UINTS_PER_NOF_SUBBUFFERS + NOF_UINTS_PER_SUBBUFFER_INFO*(gl_GlobalInvocationID.x), currentLevel))\n";
	str << "			{\n";
	str << "				uint subbufferId[NOF_UINTS_PER_COMPRESSION_ID] = getBufferId(gl_GlobalInvocationID.x);\n";
	if (numBuffers > 1)
		str << "				const uint bufferNum = getNodeBufferIndex(currentNode);\n";
	str << "				const uint bufferStart = nodeInfo[shm_startingIndexToNodeInfo + NOF_UINTS_PER_NOF_SUBBUFFERS + NOF_UINTS_PER_SUBBUFFER_INFO*(gl_GlobalInvocationID.x) + NOF_UINTS_PER_COMPRESSION_ID];\n";
	str << "				const uint startCurrent = nofEdgesPrefixSum[bufferStart];\n";
	str << "				const uint endCurrent = nofEdgesPrefixSum[bufferStart + 1];\n";
	str << "				const uint numEdges = endCurrent - startCurrent;\n";
	str << "\n";
	str << "				//Store\n";
	str << "				const uint64_t votePotential = ballotARB(isPotential);\n";
	str << "				const uint64_t voteSilhouette = ballotARB(!isPotential);\n";
	str << "				const uint64_t voteRes = isPotential ? votePotential : voteSilhouette;\n";
	if (numBuffers > 1)
		str << "				storeBufferInfo(voteRes, isPotential, startCurrent, bufferNum, numEdges);\n";
	else
		str << "				storeBufferInfo(voteRes, isPotential, startCurrent, numEdges);\n";
	str << "			}\n";
	str << "		}\n";
	str << "		currentNode = getNodeParent(currentNode, currentLevel);\n";
	str << "		barrier();\n";
	str << "	}\n";
	str << "}\n";

	return str.str();
}

std::string genCopyShader3(const std::vector<uint32_t>& lastNodePerEdgeBuffer, std::shared_ptr<Octree> octree, unsigned int workgroupSize)
{
	std::stringstream str;
	const unsigned int numBuffers = unsigned int(lastNodePerEdgeBuffer.size());
	str << genPrologue(workgroupSize);

	for (unsigned int i = 0; i < numBuffers; ++i)
		str << genBuffer(i);

	str << "\n";

	unsigned int currentIndex = numBuffers;

	const unsigned int nofUintsPerSubbufferInfo = (numBuffers > 1) ? 4 : 2;

	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _inPot{ uint inPotential[]; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _inSil{ uint inSilhouette[]; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _inNofPotBuffers{ uint nofPotBuffers; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _inNofSilBuffers{ uint nofSilBuffers; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _nofPotential{ uint nofPotential; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") readonly buffer _nofSilhouette{ uint nofSilhouette; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _potential{ uint potentialEdges[]; };\n";
	str << "layout(std430, binding = " << currentIndex++ << ") buffer _silhouette{ uint silhouetteEdges[]; };\n";

	str << "\nshared uint shm[" << workgroupSize * nofUintsPerSubbufferInfo << "];\n\n";

	str << "#define NOF_UINTS_BUFFER_INFO " << nofUintsPerSubbufferInfo << "\n";

	str << findLsbFunction;

	if (numBuffers > 1)
	{
		str << "const uint edgeBuffersMapping[" << numBuffers << "] = uint[" << numBuffers << "](";
		for (unsigned int i = 0; i < numBuffers; ++i)
		{
			str << lastNodePerEdgeBuffer[i];
			if (i != (numBuffers - 1))
				str << ", ";
		}
		str << ");\n";
	}

	if (numBuffers > 1)
	{
		str << genFindBufferFunc();
		str << genGetNodeFromBuffer(numBuffers);
	}

	str << R".(

uint getPrefixSumValuePot(unsigned int index,  uint nofPotBufs)
{
	if(index>=nofPotBufs)
		return nofPotential;
	
	return inPotential[NOF_UINTS_BUFFER_INFO * index + 0];
}

uint getPrefixSumValueSil(unsigned int index, uint nofSilBufs)
{
	if(index>=nofSilBufs)
		return nofSilhouette;
	
	return inSilhouette[NOF_UINTS_BUFFER_INFO * index + 0];
}
).";

	str << "void processEdgesPot(uint nofBuffs)\n";
	str << "{\n";
	str << "	//Binary search\n";
	str << "	int L = 0;\n";
	str << "	int R = int(nofBuffs) - 1;\n";
	str << "	int copyIndex = -1;\n";
	str << "	int m = 0;\n";
	if (numBuffers > 1)
		str << "	uint bufferIndex = 0;\n";
	str << "\n";
	str << "	while (L <= R)\n";
	str << "	{\n";
	str << "		m = (L + R) >> 1;\n";
	str << "\n";
	str << "		const uint m_val = getPrefixSumValuePot(m, nofBuffs);\n";
	str << "\n";
	str << "		if (m_val>gl_GlobalInvocationID.x)\n";
	str << "		{\n";
	str << "			R = m - 1;\n";
	str << "		}\n";
	str << "		else\n";
	str << "		{\n";
	str << "			const uint m_val1 = getPrefixSumValuePot(m + 1, nofBuffs);\n";
	str << "\n";
	str << "			if (m_val1 <= gl_GlobalInvocationID.x)\n";
	str << "				L = m + 1;\n";
	str << "			else\n";
	str << "			{\n";
	str << "				copyIndex = int(gl_GlobalInvocationID.x) - int(m_val);\n";
	if (numBuffers > 1)
		str << "				bufferIndex = inPotential[NOF_UINTS_BUFFER_INFO * uint(m) + 2];\n";
	str << "				break;\n";
	str << "			}\n";
	str << "		}\n";
	str << "	}\n";
	str << "\n";
	str << "	if (copyIndex >= 0)\n";
	str << "	{\n";
	str << "		const uint bufferStart = inPotential[NOF_UINTS_BUFFER_INFO * uint(m) + 1];\n";
	str << "\n";
	if (numBuffers == 1)
		str << "		potentialEdges[gl_GlobalInvocationID.x] = edges0[bufferStart + copyIndex];\n";
	else
		str << "		potentialEdges[gl_GlobalInvocationID.x] = getNodeFromBuffer(bufferStart + copyIndex, bufferIndex);\n";
	str << "	}\n";
	str << "}\n";
	str << "\n";
	str << "void processEdgesSil(uint nofBuffs, uint silhouetteIndex)\n";
	str << "{\n";
	str << "	//Binary search\n";
	str << "	int L = 0;\n";
	str << "	int R = int(nofBuffs) - 1;\n";
	str << "	int copyIndex = -1;\n";
	str << "	int m = 0;\n";
	if (numBuffers > 1)
		str << "	uint bufferIndex = 0;\n";
	str << "\n";
	str << "	while (L <= R)\n";
	str << "	{\n";
	str << "		m = (L + R) >> 1;\n";
	str << "\n";
	str << "		const uint m_val = getPrefixSumValueSil(m, nofBuffs);\n";
	str << "\n";
	str << "		if (m_val>silhouetteIndex)\n";
	str << "		{\n";
	str << "			R = m - 1;\n";
	str << "		}\n";
	str << "		else\n";
	str << "		{\n";
	str << "			const uint m_val1 = getPrefixSumValueSil(m + 1, nofBuffs);\n";
	str << "\n";
	str << "			if (m_val1 <= silhouetteIndex)\n";
	str << "				L = m + 1;\n";
	str << "			else\n";
	str << "			{\n";
	str << "				copyIndex = int(silhouetteIndex) - int(m_val);\n";
	if (numBuffers > 1)
		str << "				bufferIndex = inSilhouette[NOF_UINTS_BUFFER_INFO * uint(m) + 2];\n";
	str << "				break;\n";
	str << "			}\n";
	str << "		}\n";
	str << "	}\n";
	str << "\n";
	str << "	if (copyIndex >= 0)\n";
	str << "	{\n";
	str << "		const uint bufferStart = inSilhouette[NOF_UINTS_BUFFER_INFO * uint(m) + 1];\n";
	str << "\n";
	if (numBuffers == 1)
		str << "		silhouetteEdges[silhouetteIndex] = edges0[bufferStart + copyIndex];\n";
	else
		str << "		silhouetteEdges[silhouetteIndex] = getNodeFromBuffer(bufferStart + copyIndex, bufferIndex);;\n";
	str << "	}\n";
	str << "}\n";

	str << R".(
void main()
{
	uint nofPotEdges = 0;
	uint nofSilEdges = 0;
	if (gl_SubGroupInvocationARB == 0)
	{
		nofPotEdges = nofPotential;
		nofSilEdges = nofSilhouette;
	}

	nofPotEdges = readFirstInvocationARB(nofPotEdges);
	nofSilEdges = readFirstInvocationARB(nofSilEdges);
	
	//We want to eliminate warp that would have to process both pot and sil edges
	//So we round up the nof pot edges to full warps
	const uint nofPotEdgesRoundUp = uint(ceil(float(nofPotEdges)/float(gl_SubGroupSizeARB))) * gl_SubGroupSizeARB;
	
	if(gl_GlobalInvocationID.x>=(nofPotEdgesRoundUp + nofSilEdges))
		return;
		
	if(gl_GlobalInvocationID.x >=nofPotEdges && gl_GlobalInvocationID.x < nofPotEdgesRoundUp)
		return;
		
	uint nofPotBufs = 0;
	uint nofSilBufs = 0;
	if(gl_SubGroupInvocationARB==0)
	{
		nofPotBufs = nofPotBuffers;
		nofSilBufs = nofSilBuffers;
	}

	nofPotBufs = readFirstInvocationARB(nofPotBufs);
	nofSilBufs = readFirstInvocationARB(nofSilBufs);

	const bool isPot = gl_GlobalInvocationID.x<nofPotEdgesRoundUp;

	if(isPot)
		processEdgesPot(nofPotBufs);

	if(!isPot)
		processEdgesSil(nofSilBufs, gl_GlobalInvocationID.x - nofPotEdgesRoundUp);
}
).";

	return str.str();
}

std::string genPrefixSumKernelSimple(unsigned int workgroupSize)
{
	std::stringstream str;

	str << R".(
#version 450 core
).";

	str << "layout (local_size_x = " << workgroupSize << ") in;\n";
	str << R".(
layout(std430, binding = 0) buffer  _inputData  { uint inputBuffer[]; };
layout(std430, binding = 1) buffer  _outputData { uint outputBuffer[]; };
layout(std430, binding = 2) readonly buffer  _nofElems   { uint nofElements; };

shared uint temp[2 * gl_WorkGroupSize.x];

uniform uvec4 stridesOffsets;

void main()
{
	//if(nofElements<2)
	if(nofElements==0)
		return;

	const int thid = int(gl_LocalInvocationID.x);
	const uint inputStride = stridesOffsets.x;
	const uint inputOffset = stridesOffsets.y;
	const uint outputStride = stridesOffsets.z;
	const uint outputOffset = stridesOffsets.w;
	
	int  n = 0;
	
	if(nofElements<(2*gl_WorkGroupSize.x))
	{
		n = int(2*gl_WorkGroupSize.x);
	}
	else
	{
		//Finding the nearest power of two to the input
		//Based on https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
		n = int(nofElements);
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;
	}
	
	int offset = 1;
	
	// load input into shared memory
	if((2*thid) < nofElements)
		temp[2*thid] = inputBuffer[2*thid*inputStride + inputOffset];
	else
		temp[2*thid] = 0;
		
	if((2*thid+1) < nofElements)	
		temp[2*thid+1] = inputBuffer[(2*thid+1)*inputStride + inputOffset];
	else
		temp[2*thid+1] = 0;

	barrier();
	
	for (int d = n>>1; d > 0; d >>= 1) // build sum in place up the tree
	{ 
		barrier();
		if (thid < d)
		{
			int ai = offset*(2*thid+1)-1;
			int bi = offset*(2*thid+2)-1;
			temp[bi] += temp[ai];
		}
		
		offset *= 2;
	}
	
	if (thid == 0) // clear the last element
	{ 
		temp[n - 1] = 0; 
	}
	
	for (int d = 1; d < n; d *= 2) // traverse down tree & build scan
	{
		offset >>= 1;
		barrier();
		if (thid < d)                     
		{
			int ai = offset*(2*thid+1)-1;
			int bi = offset*(2*thid+2)-1;
			uint t = temp[ai];
			temp[ai] = temp[bi];
			temp[bi] += t; 
		}
	}
	
	barrier();

    // write results to device memory
	if((2*thid) < nofElements)
		outputBuffer[2*thid*outputStride + outputOffset] = temp[2*thid];
	if((2*thid+1) < nofElements)
		outputBuffer[(2*thid+1)*outputStride + outputOffset] = temp[2*thid+1];
}
).";

	return str.str();
}

std::string genPrefixSumKernelTwoLevel(unsigned int workgroupSize)
{
	std::stringstream str;

	str << R".(
#version 450 core

).";

	str << "layout (local_size_x = " << workgroupSize << ") in;\n";
	str << R".(
layout(std430, binding = 0)	 buffer _inputData  { uint inputBuffer[]; };
layout(std430, binding = 1)  buffer _outputData { uint outputBuffer[]; };
layout(std430, binding = 2) readonly  buffer _nofElems   { uint nofElements; };
layout(std430, binding = 3) writeonly buffer _tmpBufer   { uint tmpBuffer[]; };
layout(std430, binding = 4) writeonly buffer _tmpSum     { uint nofTmp; };

shared uint temp[2 * gl_WorkGroupSize.x];

uniform uvec4 stridesOffsets;

void main()
{
	//if(nofElements<2)
	if(nofElements==0)
		return;

	//starting index
	const uint wgId = gl_GlobalInvocationID.x/gl_WorkGroupSize.x;
	const unsigned int startingIndex = 2*gl_WorkGroupSize.x*wgId;
	
	if(startingIndex >= nofElements)
		return;

	const uint nofWorkingElements = min(2*gl_WorkGroupSize.x, nofElements - startingIndex);
	const int thid = int(gl_LocalInvocationID.x);
	const uint inputStride = stridesOffsets.x;
	const uint inputOffset = stridesOffsets.y;
	const uint outputStride = stridesOffsets.z;
	const uint outputOffset = stridesOffsets.w;
	
	int  n = 0;

	if(nofWorkingElements<(2*gl_WorkGroupSize.x))
	{
		n = int(2*gl_WorkGroupSize.x);
	}
	else
	{
		//Finding the nearest power of two to the input
		//Based on https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
		n = int(nofWorkingElements);
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;
	}

	int offset = 1;
	
	//In case in and out buffers are identical
	const uint lastItem = inputBuffer[(nofWorkingElements+startingIndex-1)*inputStride + inputOffset];

	// load input into shared memory
	if((2*thid) < nofWorkingElements)
		temp[2*thid] = inputBuffer[(2*thid + startingIndex)*inputStride + inputOffset];
	else
		temp[2*thid] = 0;

	if((2*thid+1) < nofWorkingElements)	
		temp[2*thid+1] = inputBuffer[(2*thid+1+startingIndex)*inputStride + inputOffset];
	else
		temp[2*thid+1] = 0;

	barrier();
	
	for (int d = n>>1; d > 0; d >>= 1) // build sum in place up the tree
	{ 
		barrier();
		if (thid < d)
		{
			int ai = offset*(2*thid+1)-1;
			int bi = offset*(2*thid+2)-1;
			temp[bi] += temp[ai];
		}
		
		offset *= 2;
	}
	
	if (thid == 0) // clear the last element
	{ 
		temp[n - 1] = 0; 
	}
	
	for (int d = 1; d < n; d *= 2) // traverse down tree & build scan
	{
		offset >>= 1;
		barrier();
		if (thid < d)                     
		{
			int ai = offset*(2*thid+1)-1;
			int bi = offset*(2*thid+2)-1;
			uint t = temp[ai];
			temp[ai] = temp[bi];
			temp[bi] += t; 
		}
	}
	
	barrier();

    // write results to device memory
	if((2*thid) < nofWorkingElements)
		outputBuffer[(2*thid+startingIndex)*outputStride + outputOffset] = temp[2*thid]; // write results to device memory

	if((2*thid+1) < nofWorkingElements)
		outputBuffer[(2*thid+1+startingIndex)*outputStride + outputOffset] = temp[2*thid+1];

	if(thid == 0)
	{
		tmpBuffer[wgId] = temp[nofWorkingElements-1] + lastItem;
		atomicAdd(nofTmp, 1);
	}
}
).";

	return str.str();
}

std::string generateSummationKernel(unsigned int workgroupSize)
{
	std::stringstream str;
	str << "#version 450 core\n";
	str << "layout (local_size_x = " << workgroupSize << ") in;\n";
	str << R".(
layout(std430, binding = 0) buffer _inputData  { uint inputBuffer[]; };
layout(std430, binding = 1) readonly buffer _sumData { uint perWgSum[]; };
layout(std430, binding = 2) readonly buffer _dataSize { uint dataSize; };

uniform uint previousWgSize;

uniform uvec2 outputStrideOffset;

void main()
{
	if(gl_GlobalInvocationID.x < int(dataSize))
	{
		const uint index = gl_GlobalInvocationID.x/(2*previousWgSize);
		inputBuffer[outputStrideOffset.x *gl_GlobalInvocationID.x + outputStrideOffset.y] += perWgSum[index];
	}
}
).";
	return str.str();
}