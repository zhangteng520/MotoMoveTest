#include<fstream>
#include<iostream>
#include<filesystem>
#include<assert.h>


#include"slcReader.h"
using namespace std;

using u8 = unsigned __int8;
using u32 = unsigned __int32;

template <class T>
inline void binRead(istream& i, T& a)
{
	i.read((char*)(&a), sizeof a);
}

template <class T>
inline T binRead(istream& i)
{
	T t;
	binRead(i, t);
	return t;
}
void SLCReader::init(const string& p_fileName) {
	ifstream s;
	s.open(p_fileName, ios::binary);

	//获取文件大小
	s.seekg(0, std::ios::end);
	float len = s.tellg();
	//cout << file_size << endl;
	s.seekg(0, std::ios::beg);

	static char buff[2 * 1024 * 1024];
	s.rdbuf()->pubsetbuf(buff, sizeof buff);

	s.read(m_head, headSize);
	m_head[headSize + 1 - 1] = 0;
	char* pos = strstr(m_head, "\r\n\x1a");
	if (pos == nullptr) {
		cout << "Read Slc error\n";
		return;
	};
	*pos = 0;
	s.seekg(pos - m_head + 3 + 256);
	//256 for 3D Reserved Section(预留部分)

	auto fileSize = filesystem::file_size(p_fileName);

	auto samplingTableSize = binRead<u8>(s);
	m_samplingTables.resize(samplingTableSize);
	s.read((char*)m_samplingTables.data(), samplingTableSize * sizeof(float) * 4);

	m_layers.reserve(256);
	while (true) {
		float z;
		binRead(s, z);

		u32 numberOfBoundaries = binRead<u32>(s);
		if (numberOfBoundaries == 0xffffffff) {
			//Termination Value：1 Unsigned Integer (0xFFFFFFFF)
			m_topOfPartMaximumZLevel = z;
			break;
		}


		assert(z < 1e5f);//todo 合法性检查
		assert(numberOfBoundaries < 10000);

		auto& layer = m_layers.emplace_back(z, numberOfBoundaries);
		for (u32 i = 0; i < numberOfBoundaries; i++) {
			auto& boundarie = layer.bound[i];

			auto boundarie_size = binRead<u32>(s);
			boundarie.resize(boundarie_size);
			//read Number of Vertices
			binRead<u32>(s);
			//read Number of Gaps

			/*s.read((char*)&boundarie[0], sizeof(glm::vec2) * boundarie_size);*/
			s.read((char*)&boundarie[0], sizeof(Point2f) * boundarie_size);
			//read Vertices List;
		}
	}
};