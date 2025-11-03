#pragma once
#include<vector>
#include<string>
#include"glm/glm.hpp"
#include"glm/gtc/matrix_transform.hpp "
//头文件内尽量不使用
//using namespace std;


template<class Number>
using Point2 = glm::tvec2<Number>;
typedef Point2<float> Point2f;
//存储点的数组
typedef std::vector<Point2f> CPointArray;
typedef std::vector<CPointArray> CBoundary;
//表示层结构
struct CLayer
{
	float zmin;
	///float zthickness;
	CBoundary bound;
	//Point2f mid;
	CLayer() {}
	CLayer(float p_z, unsigned __int32 p_boundariesSize)
		:zmin(p_z), bound(p_boundariesSize) {
		;
	}
};

//表示总的图层结构
typedef std::vector<CLayer> CLayers;

//struct CBoundLayers {
//	CLayers layers;
//	CBoundary points;
//};



class SLCReader
{
public:
#pragma pack(push)
#pragma pack(1)
	struct SamplingTable {
		float minimumZLevel;
		float layerThickness;
		float lineWidthCompensation;
		float reserved;
	};
#pragma pack(pop)
	using SamplingTables = std::vector<SamplingTable>;
	void init(const std::string& p_fileName);

	const char* head(void) const {
		return m_head;
	}
	const SamplingTables& sampleTables(void) const {
		return m_samplingTables;
	};
	float topOfPartMaximumZLevel(void) const {
		return m_topOfPartMaximumZLevel;
	};
	CLayers& layers(void) {
		return m_layers;
	}
	void clear() {
		m_layers.clear();
	}
protected:

	const static int headSize = 2048;
	char m_head[headSize + 1];
	std::vector<SamplingTable> m_samplingTables;
	float m_topOfPartMaximumZLevel;
	CLayers m_layers;
};

