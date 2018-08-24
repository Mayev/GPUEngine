#include "HSSV.hpp"
#include "SidesProgram.hpp"

#include <glm/gtx/transform.hpp>
#include "geGL/StaticCalls.h"
#include "MultiplicityCoding.hpp"
#include "HighResolutionTimer.hpp"

#include <fstream>

#include "OctreeSerializer.hpp"
#include "OctreeCompressor.hpp"

HSSV::HSSV(
	std::shared_ptr<Model> model,
	const glm::vec3& sceneAABBscale,
	unsigned maxOctreeLevel,
	unsigned int subgroupSize,
	unsigned int workgroupSize,
	unsigned char potentialMethod,
	unsigned char silhouetteMethod,
	std::shared_ptr<ge::gl::Texture> const& shadowMask,
	std::shared_ptr<ge::gl::Texture> const& depth,
	ShadowVolumesParams const& params) : ShadowVolumes(shadowMask, depth, params)
{
	std::vector<float>vertices;
	model->getVertices(vertices);
	const auto nofVertexFloats = vertices.size();

	printf("Renderer: %s %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));

	_vertices = new float[vertices.size()];
	memcpy(_vertices, vertices.data(), vertices.size() * sizeof(float));
	vertices.clear();

	size_t const nofTriangles = nofVertexFloats / (verticesPerTriangle*componentsPerVertex3D);
	_edges = std::make_shared<Adjacency const>(_vertices, nofTriangles, 2);

	std::cout << "NoF Edges: " << _edges->getNofEdges() << std::endl;

	_capsDrawer = std::make_shared<GSCaps>(_edges);

	AABB sceneBbox = _getSceneAabb(_vertices, nofVertexFloats);
	_fixSceneAABB(sceneBbox);

	auto minP = sceneBbox.getMinPoint();
	auto maxP = sceneBbox.getMaxPoint();
	std::cout << "Scene AABB " << minP.x << ", " << minP.y << ", " << minP.z << " Max: " << maxP.x << ", " << maxP.y << ", " << maxP.z << "\n";

	sceneBbox.setCenterExtents(sceneBbox.getCenterPoint(), sceneBbox.getExtents()*sceneAABBscale);

	minP = sceneBbox.getMinPoint();
	maxP = sceneBbox.getMaxPoint();
	std::cout << "Octree working space: " << minP.x << ", " << minP.y << ", " << minP.z << " Max: " << maxP.x << ", " << maxP.y << ", " << maxP.z << "\n";

	_loadGpuEdges(_edges);

	HighResolutionTimer t;
	OctreeSerializer serializer;
	t.reset();
	//_octree = serializer.loadFromFile(model->modelFilename, sceneAABBscale, maxOctreeLevel);
	if (!_octree)
	{
		_octree = std::make_shared<Octree>(maxOctreeLevel, sceneBbox);
		_visitor = std::make_shared<OctreeVisitor>(_octree);

		t.reset();
		_visitor->addEdgesGPU(_edges, _gpuEdges, subgroupSize);
		//_visitor->addEdges(_edges);
		const auto dt = t.getElapsedTimeFromLastQuerySeconds();

		std::cout << "Building octree took " << dt << " seconds\n";

		OctreeCompressor compressor;
		compressor.compressOctree(_visitor, 2);

		std::cout << "Compresing octree took " << t.getElapsedTimeFromLastQuerySeconds() << "s\n";

		_visitor->shrinkOctree();
		std::cout << "Shrinking octree took " << t.getElapsedTimeFromLastQuerySeconds() << "s\n";

		t.reset();
		serializer.storeToFile(model->modelFilename, sceneAABBscale, _octree);
		std::cout << "Storing model to file took " << t.getElapsedTimeSeconds() << "s\n";
	}
	else
	{
		std::cout << "Loading model from file took " << t.getElapsedTimeSeconds() << "s\n";
		_visitor = std::make_shared<OctreeVisitor>(_octree);
	}

	//_octreeSidesDrawer = std::make_shared<OctreeSidesDrawer>(_visitor, workgroupSize, DrawingMethod(potentialMethod), DrawingMethod(silhouetteMethod));
	//_octreeSidesDrawer->init(_gpuEdges);
	
	_prepareBuffers(2 * _edges->getNofEdges() * 6 * 4 * sizeof(float));
	_prepareProgram();
	//_octree.reset();
	//_edges.reset();
	//delete[] _vertices;
}

HSSV::~HSSV()
{
	if (_vertices)
		delete[] _vertices;
}

void HSSV::drawCaps(glm::vec4 const& lightPosition, glm::mat4 const& viewMatrix, glm::mat4 const& projectionMatrix)
{
	_capsDrawer->drawCaps(lightPosition, viewMatrix, projectionMatrix);
}

void HSSV::drawSides(glm::vec4 const& lightPosition, glm::mat4 const& viewMatrix, glm::mat4 const& projectionMatrix)
{
	const glm::mat4 mvp = projectionMatrix * viewMatrix;
	
	std::vector<float> sidesGeometry;
	
	_getSilhouetteFromLightPos(lightPosition, sidesGeometry);
	
	_updateSidesVBO(sidesGeometry);

	_sidesVAO->bind();
	_sidesProgram->use();
	_sidesProgram->setMatrix4fv("mvp", glm::value_ptr(mvp));

	ge::gl::glDrawArrays(GL_TRIANGLES, 0, sidesGeometry.size() / 4);

	_sidesVAO->unbind();
	//*/
	//_octreeSidesDrawer->drawSides(mvp, lightPosition);
}

AABB HSSV::_getSceneAabb(float* vertices3fv, size_t numVerticesFloats)
{
	AABB bbox;
	bbox.updateWithVerticesVec3(vertices3fv, numVerticesFloats);

	return bbox;
}

void HSSV::_fixSceneAABB(AABB& bbox)
{
	float ex, ey, ez;
	bbox.getExtents(ex, ey, ez);

	const float minSize = 0.01f;

	if(ex<minSize)
	{
		auto min = bbox.getMinPoint();
		auto max = bbox.getMaxPoint();

		bbox.setMinMaxPoints(min - glm::vec3(minSize, 0, 0), max + glm::vec3(minSize, 0, 0));

		std::cout << "Fixing X AABB dimension\n";
	}

	if(ey<minSize)
	{
		auto min = bbox.getMinPoint();
		auto max = bbox.getMaxPoint();

		bbox.setMinMaxPoints(min - glm::vec3(0, minSize, 0), max + glm::vec3(0, minSize, 0));

		std::cout << "Fixing Y AABB dimension\n";
	}

	if(ez<minSize)
	{
		auto min = bbox.getMinPoint();
		auto max = bbox.getMaxPoint();

		bbox.setMinMaxPoints(min - glm::vec3(0, 0, minSize), max + glm::vec3(0, 0, minSize));

		std::cout << "Fixing Z AABB dimension\n";
	}
}

void HSSV::_prepareBuffers(size_t maxVboSize)
{
	_sidesVBO = std::make_shared <ge::gl::Buffer>(maxVboSize);
	_sidesVAO = std::make_shared<ge::gl::VertexArray>();

	_sidesVAO->addAttrib(_sidesVBO, 0, 4, GL_FLOAT);
}

void HSSV::_prepareProgram()
{
	std::shared_ptr<ge::gl::Shader> vs = std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER, hssvVsSource);
	std::shared_ptr<ge::gl::Shader> fs = std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER, hssvFsSource);

	_sidesProgram = std::make_shared<ge::gl::Program>(vs, fs);
}

void HSSV::_updateSidesVBO(const std::vector<float>& vertices)
{
	float* ptr = reinterpret_cast<float*>(_sidesVBO->map(GL_MAP_WRITE_BIT));

	memcpy(ptr, vertices.data(), vertices.size() * sizeof(float));

	_sidesVBO->unmap();
}

void HSSV::_getSilhouetteFromLightPos(const glm::vec3& lightPos, std::vector<float>& sidesVertices)
{
	const int lowestNode = _visitor->getLowestNodeIndexFromPoint(lightPos);

	static bool isLightOut = false;

	if (lowestNode < 0)
	{
		if (!isLightOut)
		{
			std::cerr << "Light (" << lightPos.x << " " << lightPos.y << " " << lightPos.z << ") is out of octree range\n";
			isLightOut = true;
		}
		return;
	}

	isLightOut = false;

	std::vector<unsigned int> silhouetteEdges;
	std::vector<unsigned int> potentialEdges;

	_visitor->getSilhouttePotentialEdgesFromNodeUp(potentialEdges, silhouetteEdges, lowestNode);

	sidesVertices.clear();
	sidesVertices.reserve((potentialEdges.size() + silhouetteEdges.size()) * 4);

	//--
	std::vector<int> ed;
	//ed.insert(ed.end(), silhouetteEdges.begin(), silhouetteEdges.end());
	//--
	
	for(const auto edge : silhouetteEdges)
	{
		const int multiplicity = decodeEdgeMultiplicityFromId(edge);
		const glm::vec3& lowerPoint = getEdgeVertexLow(_edges, decodeEdgeFromEncoded(edge));
		const glm::vec3& higherPoint = getEdgeVertexHigh(_edges, decodeEdgeFromEncoded(edge));;

		_generatePushSideFromEdge(lightPos, lowerPoint, higherPoint, multiplicity, sidesVertices);
	}
	//*/
	for(const auto edge : potentialEdges)
	{
		const int multiplicity = GeometryOps::calcEdgeMultiplicity(_edges, edge, lightPos);
		if (multiplicity != 0)
		{
			const glm::vec3& lowerPoint = getEdgeVertexLow(_edges, edge);
			const glm::vec3& higherPoint = getEdgeVertexHigh(_edges, edge);;

			_generatePushSideFromEdge(lightPos, lowerPoint, higherPoint, multiplicity, sidesVertices);
			ed.push_back(encodeEdgeMultiplicityToId(edge, multiplicity));
		}
	}
	
	static bool printOnce = false;
	if (!printOnce)
	{
		std::cout << "Light node: " << lowestNode << std::endl;
		std::cout << "Light " << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << std::endl;
		const auto n = _octree->getNode(lowestNode);
		auto minP = n->volume.getMinPoint();
		auto maxP = n->volume.getMaxPoint();
		std::cout << "Node space " << minP.x << ", " << minP.y << ", " << minP.z << " Max: " << maxP.x << ", " << maxP.y << ", " << maxP.z << "\n";
		minP = n->volume.getCenterPoint();
		n->volume.getExtents(maxP.x, maxP.y, maxP.z);
		std::cout << "Center " << minP.x << ", " << minP.y << ", " << minP.z << " Extents: " << maxP.x << ", " << maxP.y << ", " << maxP.z << "\n";
		
		std::cout << "Num potential: " << potentialEdges.size() << ", numSilhouette: " << silhouetteEdges.size() << std::endl;
		std::cout << "Silhouette consists of " << ed.size() << " edges\n";
		
		std::ofstream of;
		of.open("edges_compress.txt");
		std::sort(potentialEdges.begin(), potentialEdges.end());
		std::sort(silhouetteEdges.begin(), silhouetteEdges.end());
		
		of << "Potential:\n";
		for (const auto e : potentialEdges)
			of << e << std::endl;

		of << "\nSilhouette:\n";
		for (const auto e : silhouetteEdges)
			of << decodeEdgeFromEncoded(e) << " multiplicity: " << decodeEdgeMultiplicityFromId(e) << std::endl;
		of.close();
		
		std::ofstream sof;
		sof.open("silhouette.txt");
		sof << "SIL\n";
		for (const auto e : silhouetteEdges)
			sof << decodeEdgeFromEncoded(e) << "(" << decodeEdgeMultiplicityFromId(e) << ")" << std::endl;
		
		sof << "POT\n";
		for (const auto e : ed)
			sof << decodeEdgeFromEncoded(e) << "(" << decodeEdgeMultiplicityFromId(e) << ")" << std::endl;
		//*/
		sof.close();
		printOnce = true;
	}
	//*/
}

void HSSV::_generatePushSideFromEdge(const glm::vec3& lightPos, const glm::vec3& lowerPoint, const glm::vec3& higherPoint, int multiplicity, std::vector<float>& sides)
{
	const glm::vec3 lowInfinity = lowerPoint - lightPos;
	const glm::vec3 highInfinity = higherPoint - lightPos;

	const unsigned int absMultiplicity = abs(multiplicity);

	if (multiplicity > 0)
	{
		for (unsigned int i = 0; i < absMultiplicity; ++i)
		{
			sides.push_back(highInfinity.x); sides.push_back(highInfinity.y); sides.push_back(highInfinity.z); sides.push_back(0);
			sides.push_back(higherPoint.x); sides.push_back(higherPoint.y); sides.push_back(higherPoint.z); sides.push_back(1.0f);
			sides.push_back(lowerPoint.x); sides.push_back(lowerPoint.y); sides.push_back(lowerPoint.z); sides.push_back(1.0f);

			sides.push_back(lowInfinity.x); sides.push_back(lowInfinity.y); sides.push_back(lowInfinity.z); sides.push_back(0);
			sides.push_back(highInfinity.x); sides.push_back(highInfinity.y); sides.push_back(highInfinity.z); sides.push_back(0);
			sides.push_back(lowerPoint.x); sides.push_back(lowerPoint.y); sides.push_back(lowerPoint.z); sides.push_back(1.0f);
		}
	}
	else if (multiplicity < 0)
	{
		for (unsigned int i = 0; i < absMultiplicity; ++i)
		{
			sides.push_back(lowInfinity.x); sides.push_back(lowInfinity.y); sides.push_back(lowInfinity.z); sides.push_back(0);
			sides.push_back(lowerPoint.x); sides.push_back(lowerPoint.y); sides.push_back(lowerPoint.z); sides.push_back(1.0f);
			sides.push_back(higherPoint.x); sides.push_back(higherPoint.y); sides.push_back(higherPoint.z); sides.push_back(1.0f);

			sides.push_back(highInfinity.x); sides.push_back(highInfinity.y); sides.push_back(highInfinity.z); sides.push_back(0);
			sides.push_back(lowInfinity.x); sides.push_back(lowInfinity.y); sides.push_back(lowInfinity.z); sides.push_back(0);
			sides.push_back(higherPoint.x); sides.push_back(higherPoint.y); sides.push_back(higherPoint.z); sides.push_back(1.0f);
		}
	}
}

void HSSV::_loadGpuEdges(AdjacencyType edges)
{
	_gpuEdges = std::make_shared<GpuEdges>();
	_gpuEdges->_edges = std::make_shared<ge::gl::Buffer>();
	_gpuEdges->_oppositeVertices = std::make_shared<ge::gl::Buffer>();

	std::vector<float> serializedEdges;
	std::vector<glm::vec3> serializedOppositeVertices;

	_serializeEdges(edges, serializedEdges, serializedOppositeVertices);

	_gpuEdges->_edges->alloc(serializedEdges.size() * sizeof(float), serializedEdges.data(), GL_STATIC_READ);
	_gpuEdges->_oppositeVertices->alloc(serializedOppositeVertices.size() * 3 * sizeof(float), serializedOppositeVertices.data(), GL_STATIC_READ);
}

void HSSV::_serializeEdges(AdjacencyType edges, std::vector<float>& serializedEdges, std::vector<glm::vec3>& serializedOppositeVertices)
{
	const auto nofEdges = edges->getNofEdges();

	const glm::vec3* vertices = reinterpret_cast<const glm::vec3*>(edges->getVertices());

	for (size_t edgeIndex = 0; edgeIndex<nofEdges; ++edgeIndex)
	{
		glm::vec3 v1 = vertices[edges->getEdgeVertexA(edgeIndex) / 3];
		glm::vec3 v2 = vertices[edges->getEdgeVertexB(edgeIndex) / 3];

		const unsigned int nofOpposite = edges->getNofOpposite(edgeIndex);
		const unsigned int starting_index = serializedOppositeVertices.size() * 3;

		serializedEdges.push_back(v1.x); serializedEdges.push_back(v1.y); serializedEdges.push_back(v1.z);
		serializedEdges.push_back(v2.x); serializedEdges.push_back(v2.y); serializedEdges.push_back(v2.z);
		serializedEdges.push_back(*((float*)&nofOpposite));
		serializedEdges.push_back(*((float*)&starting_index));

		for (unsigned int i = 0; i<nofOpposite; ++i)
			serializedOppositeVertices.push_back(vertices[edges->getOpposite(edgeIndex, i) / 3]);
	}
}

void HSSV::setTimeStamper(std::shared_ptr<TimeStamp> stamper)
{
	timeStamp = stamper;
	if (_octreeSidesDrawer) _octreeSidesDrawer->setStamper(stamper);
}

