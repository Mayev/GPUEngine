#include "OctreeCompressor.hpp"

#include <set>
#include <iterator>

void OctreeCompressor::compressOctree(std::shared_ptr<OctreeVisitor> visitor, unsigned int compressLevelHeight_1or2)
{
	if (!visitor)
		return;

	_visitor = visitor;

	const int deepestLevel = _visitor->getOctree()->getDeepestLevel();
	const int levelSize = ipow(OCTREE_NUM_CHILDREN, deepestLevel);

	const int startingIndex = _visitor->getOctree()->getNumNodesInPreviousLevels(deepestLevel);
	const int stopIndex = _visitor->getOctree()->getTotalNumNodes();

#pragma omp parallel for
	for (int index = startingIndex; index < stopIndex; index += OCTREE_NUM_CHILDREN)
	{
		_compressSyblings(index, true);
		_compressSyblings(index, false);
	}
}

std::bitset<BitmaskTypeSize> OctreeCompressor::checkEdgePresence(unsigned int edge, unsigned int startingId, bool checkPotential) const
{
	std::bitset<OCTREE_NUM_CHILDREN> retval(0);
	auto octree = _visitor->getOctree();

	for (unsigned int i = 0; i<OCTREE_NUM_CHILDREN; ++i)
	{
		const auto node = octree->getNode(i + startingId);
		const auto& buffer = checkPotential ? node->edgesMayCastMap[255] : node->edgesAlwaysCastMap[255];
		if (std::binary_search(buffer.begin(), buffer.end(), edge))
			retval[i] = true;
	}

	return retval;
}

void OctreeCompressor::_compressSyblings(unsigned int startingID, bool processPotential)
{
	std::set<unsigned int> allEdgesSet;
	for (unsigned int i = 0; i < OCTREE_NUM_CHILDREN; ++i)
	{
		const auto node = _visitor->getOctree()->getNode(startingID + i);
		auto& buffer = processPotential ? node->edgesMayCastMap[255] : node->edgesAlwaysCastMap[255];
		std::copy(buffer.begin(), buffer.end(), std::inserter(allEdgesSet, allEdgesSet.end()));
	}

	for (const auto e : allEdgesSet)
	{
		const auto bitmask = checkEdgePresence(e, startingID, processPotential);
		if (bitmask.count()>3)
		{
			_visitor->assignEdgeToNodeParent(startingID, e, processPotential, bitmask.to_ulong());
			_removeEdgeFromSyblingsSparse(startingID, e, processPotential, bitmask);
		}
	}
}

void OctreeCompressor::_removeEdgeFromSyblingsSparse(unsigned int startingID, unsigned int edge, bool checkPotential, const std::bitset<OCTREE_NUM_CHILDREN>& bitmask)
{
	for (unsigned int i = 0; i<OCTREE_NUM_CHILDREN; ++i)
	{
		if (!bitmask[i])
			continue;

		auto node = _visitor->getOctree()->getNode(startingID + i);

		if (node)
		{
			if (checkPotential)
				node->edgesMayCastMap[255].erase(std::lower_bound(node->edgesMayCastMap[255].begin(), node->edgesMayCastMap[255].end(), edge));
			else
				node->edgesAlwaysCastMap[255].erase(std::lower_bound(node->edgesAlwaysCastMap[255].begin(), node->edgesAlwaysCastMap[255].end(), edge));
		}
	}
}