#include"Voxelization.h"
CubeD GetCubf(const CLayers& clayers) {
	//
	double xmin = INT32_MAX;
	double xmax = INT32_MIN;
	double ymin = INT32_MAX;
	double ymax = INT32_MIN;
	double zmin = clayers.front().zmin;
	double zmax = clayers.back().zmin;
	for (auto& clayer : clayers) {
		for (auto& cb : clayer.bound) {
			for (auto& point : cb) {
				if (point.x < xmin)
					xmin = point.x;
				if (point.x > xmax)
					xmax = point.x;
				if (point.y < ymin)
					ymin = point.y;
				if (point.y > ymax)
					ymax = point.y;
			}
		}

	}
	CubeD cubD ;
	cubD.min = { xmin,ymin,zmin };
	cubD.max = { xmax,ymax,zmax };
	return cubD;
}
void MakeVoxelDimension(const CubeD& cube,VoxelDimension & voxelDis) {
	voxelDis.n_x = ceil(cube.w() / voxelDis.precision) + 2 * (voxelDis.num_margin+1);
	voxelDis.n_y = ceil(cube.h() / voxelDis.precision) + 2 * (voxelDis.num_margin+1);
	voxelDis.X_begin = cube.min.x - (voxelDis.num_margin+1) * voxelDis.precision;
	voxelDis.Y_begin = cube.min.y - (voxelDis.num_margin+1) * voxelDis.precision;
	
}
Voxel ExecVoxelization(const CBoundary& contour, const VoxelDimension& voxelDis) {
	Voxel data(voxelDis.n_y, std::vector<unsigned char>(voxelDis.n_x, 0));
	CBoundary cb = contour;
	Clipper2Lib::Paths64 fill, ct;
	RasterPlaning(cb, voxelDis.precision,0, 0, AirOutlet::Right,fill, ct);
	for (const auto& i : fill) {
		int index_X1 = ceil((i[0].x  - voxelDis.X_begin*5882.f)/voxelDis.precision/5882.f);
		int index_Y1 = ceil((i[0].y  - voxelDis.Y_begin*5882.f)/voxelDis.precision/5882.f);

		int index_X2 = ceil((i[1].x - voxelDis.X_begin*5882.f)/voxelDis.precision/5882.f);
		
		int MinX = std::min(index_X1, index_X2);
		int MaxX = std::max(index_X1, index_X2);
		for (size_t j = MinX; j < MaxX; ++j) {
			data[index_Y1][j] = 1;
		}
	}
	return data;
}
double distanceFromOrigin(int i, int j, int k, double precision,double thickness) {
	return std::sqrt(std::pow(i * precision, 2) + std::pow(j * precision, 2) + std::pow(k * thickness, 2));
}
//std::vector<std::vector<Point2ID>>MakeIndex(int num) {
//	std::vector<std::vector<Point2ID>>data;
//	for (int i = 0; i < num; i++) {
//		std::vector<Point2ID>tmp;
//		int dy = std::round(sqrt((num )*(num) - i *i));
//		for (int j = -dy; j <= dy; j++) {
//			int dx = std::round(sqrt((dy) * (dy)-j * j));
//			for (int k = -dx; k <= dx; k++) {
//				float dis = 
//				Point2ID p = { k,j };
//				tmp.push_back(p);
//			}
//		}
//		data.push_back(tmp);
//		
//	}
//}


std::vector<std::vector<Point2ID>> makeSquereID(const int sizeR, const int sizeZ, float precision, float thickness)
{
	
	std::vector<std::vector<Point2ID>> result;
	int Xmax = sizeR;
	int Ymax = sizeR;
	for (int i = 0; i < sizeZ&& sizeR * precision >= i * thickness; i++)
	{
		std::vector<Point2ID> IDs;
		//IDs.push_back({ 0,0, (i+1) * precision });
		for (int j = 1; j <= Xmax; j++)
		{
			float d = distanceFromOrigin(j, 0, i, precision,thickness);
			if (d <= sizeR * precision)
			{
				IDs.push_back({ j, 0, d });
				IDs.push_back({ -j, 0, d });
			}
		}
		for (int k = 1; k <= Ymax; k++)
		{
			float d = distanceFromOrigin(0, k, i, precision, thickness);
			if (d <= sizeR * precision)
			{
				IDs.push_back({ 0, k, d });
				IDs.push_back({ 0, -k, d });
			}
		}
		for (int j = 1; j <= Xmax; j++)
		{
			for (int k = 1; k <= Ymax; k++)
			{
				float d = distanceFromOrigin(j, k, i, precision, thickness);
				if (d <= sizeR*precision)
				{
					IDs.push_back({ j, k, d });
					IDs.push_back({ -j, k, d });
					IDs.push_back({ -j, -k, d });
					IDs.push_back({ j, -k, d });
				}
			}
		}
		result.push_back(IDs);
	}
	return result;
}


float Kernel(const FixedSizeQueue<Voxel>& queue, const std::vector<std::vector<Point2ID>> IDs,const int index_x,const int index_y ) {
	float result = 0.0;
	for (int i = 0; i < IDs.size(); i++)
	{
		for (auto &ID : IDs[i])
		{
			result += (queue[i][index_y + ID.dy][index_x + ID.dx] == 0 ? 1.0f : 100.0f) / ID.d;
		}
	}
	return result;
} 


float StandardFactor(const std::vector<std::vector<Point2ID>> IDs) {
	float result = 0.0;
	for (int i = 0; i < IDs.size(); i++)
	{
		for (auto &ID : IDs[i])
		{
			result +=  100.0/ ID.d;
		}
	}
	return result;
}
//计算一层的
VoxelFactor CaculateFactor(const CBoundary& contour,const int layer_index, const VoxelDimension& voxelDis
	, FixedSizeQueue<Voxel> &queue, const std::vector<std::vector<Point2ID>> &pointID,const float standardfactor) {
		Voxel voxel = ExecVoxelization(contour,voxelDis );
		VoxelFactor data(voxelDis.n_y, std::vector<float>(voxelDis.n_x, 0.5));
		queue.pop();
		queue.push(voxel);
			for (int i = 0; i < voxel.size(); i++) {
				for (int j = 0; j < voxel[i].size(); j++) {
					if (voxel[i][j]==1) {
						//计算当前能量 影响因子
						float factor = Kernel(queue, pointID, j, i);
						//归一
						float factor_to_one = factor / standardfactor;
						data[i][j] = factor_to_one;
					}
				}
			}
		
		return data;

	}

VariPathss  GeneratePaths(const Clipper2Lib::Paths64& path, const VoxelFactor &vf
	,const VoxelDimension& voxelDis,const float scanlength, float power_high, float power_low) {
	VariPathss vpss;
	for (auto& i : path) {
		VariPaths vps;
		vps.JumpPoint = i[0];
		double sinx = (i[1].y - i[0].y) / DisTwoPoints(i[0], i[1]);
		double cosx = (i[1].x - i[0].x) / DisTwoPoints(i[0], i[1]);
		Clipper2Lib::PointD cur ( i[0].x,i[0].y);
		cur.x += scanlength * 5882.f * cosx;
		cur.y += scanlength * 5882.f * sinx;
		//添加第一个点
		VariPath vp1;
		Clipper2Lib::PointD begin(i[0].x, i[0].y);
		vp1.point = { (int64_t)begin.x, (int64_t)begin.y };
		int index_X = ceil((begin.x - voxelDis.X_begin * 5882.f) / (voxelDis.precision * 5882.f));
		int index_Y = ceil((begin.y - voxelDis.Y_begin * 5882.f) / (voxelDis.precision * 5882.f));
		vp1.power = power_low + (power_high - power_low) * vf[index_Y][index_X];
		vps.MarkPoint.push_back(vp1);
		while (abs(cur.x - i[0].x) <= abs(i[1].x - i[0].x)
			&&abs(cur.y - i[0].y) <= abs(i[1].y - i[0].y)) {
			VariPath vp;
			vp.point = {(int64_t)cur.x,(int64_t)cur.y};

			int index_X = ceil((cur.x - voxelDis.X_begin*5882.f) / (voxelDis.precision*5882.f));
			int index_Y = ceil((cur.y - voxelDis.Y_begin*5882.f) / (voxelDis.precision*5882.f));
			vp.power = power_low + (power_high - power_low) * vf[index_Y][index_X];
			vps.MarkPoint.push_back(vp);
			cur.x += scanlength * 5882.f * cosx;
			cur.y += scanlength * 5882.f * sinx;
			 
		}
		//添加最后一个点
		VariPath vp;
		Clipper2Lib::PointD end(i[1].x, i[1].y);
		vp.point = { (int64_t)end.x, (int64_t)end.y };
		index_X = ceil((end.x - voxelDis.X_begin * 5882.f) / (voxelDis.precision * 5882.f));
		index_Y = ceil((end.y - voxelDis.Y_begin * 5882.f) / (voxelDis.precision * 5882.f));
		vp.power = power_low + (power_high - power_low) * vf[index_Y][index_X];
		vps.MarkPoint.push_back(vp);

		//过滤轮廓
		VariPaths vps2;
		vps2.JumpPoint = vps.JumpPoint;
		for (int j = 0; j < vps.MarkPoint.size();j++) {
			if ((j + 1) < vps.MarkPoint.size()) {
				if (abs(vps.MarkPoint[j].power - power_high) < 1e-2
					&& abs(vps.MarkPoint[j + 1].power - power_high) < 1e-2)
					continue;
				else
					vps2.MarkPoint.push_back(vps.MarkPoint[j]);
			}
			else
				vps2.MarkPoint.push_back(vps.MarkPoint[j]);
		}


		vpss.push_back(vps2);
	}
	return vpss;
}

VariPathss  GeneratePathsContour(const Clipper2Lib::Paths64& path, const VoxelFactor& vf
	, const VoxelDimension& voxelDis, const float scanlength, float power_high, float power_low) {
	VariPathss vpss;
	for (auto& i : path) {
		VariPaths vps;
		vps.JumpPoint = i[0];

		for (int j = 1; j <= i.size(); j++) {
			int n = j % i.size();
			double sinx = (i[n].y - i[j-1].y) / DisTwoPoints(i[n], i[j-1]);
			double cosx = (i[n].x - i[j-1].x) / DisTwoPoints(i[n], i[j-1]);
			Clipper2Lib::PointD cur(i[j-1].x, i[j-1].y);
			cur.x += scanlength * 5882.f * cosx;
			cur.y += scanlength * 5882.f * sinx;

			while (abs(cur.x - i[j-1].x) <= abs(i[n].x - i[j-1].x)
				&& abs(cur.y - i[j-1].y) <= abs(i[n].y - i[j-1].y)) {
				VariPath vp;
				vp.point = { (int64_t)cur.x,(int64_t)cur.y };

				int index_X = ceil((cur.x - voxelDis.X_begin * 5882.f) / (voxelDis.precision * 5882.f));
				int index_Y = ceil((cur.y - voxelDis.Y_begin * 5882.f) / (voxelDis.precision * 5882.f));
				vp.power = power_low + (power_high - power_low) * vf[index_Y][index_X];
				vps.MarkPoint.push_back(vp);
				cur.x += scanlength * 5882.f * cosx;
				cur.y += scanlength * 5882.f * sinx;

			}
			//添加最后一个点
			VariPath vp;
			Clipper2Lib::PointD end(i[n].x, i[n].y);
			vp.point = { (int64_t)end.x, (int64_t)end.y };
			int index_X = ceil((end.x - voxelDis.X_begin * 5882.f) / (voxelDis.precision * 5882.f));
			int index_Y = ceil((end.y - voxelDis.Y_begin * 5882.f) / (voxelDis.precision * 5882.f));
			vp.power = power_low + (power_high - power_low) * vf[index_Y][index_X];
			vps.MarkPoint.push_back(vp);

			

		}

		//添加第一个点制作封闭轮
		VariPath vp1;
		Clipper2Lib::PointD begin(i[0].x, i[0].y);
		vp1.point = { (int64_t)begin.x, (int64_t)begin.y };
		int index_X = ceil((begin.x - voxelDis.X_begin * 5882.f) / (voxelDis.precision * 5882.f));
		int index_Y = ceil((begin.y - voxelDis.Y_begin * 5882.f) / (voxelDis.precision * 5882.f));
		vp1.power = power_low + (power_high - power_low) * vf[index_Y][index_X];
		vps.MarkPoint.push_back(vp1);

		//过滤轮廓
		VariPaths vps2;
		vps2.JumpPoint = vps.JumpPoint;
		for (int j = 0; j < vps.MarkPoint.size(); j++) {
			if ((j + 1) < vps.MarkPoint.size()) {
				if (abs(vps.MarkPoint[j].power - power_high) < 1e-2
					&& abs(vps.MarkPoint[j + 1].power - power_high) < 1e-2)
					continue;
				else
					vps2.MarkPoint.push_back(vps.MarkPoint[j]);
			}
			else
				vps2.MarkPoint.push_back(vps.MarkPoint[j]);
		}


		vpss.push_back(vps2);


	}
	return vpss;
}
//绿色到蓝色再到红色
RGBA GetLinearColor(int step, float cur, float low, float high) {
	if (cur * 2 < (high + low)) {
		float half = (high + low) / 2;
		int subStep = step / 2;
		float t = 1.f / subStep;
		float curt = (half - cur) / (half - low);
		float spt = 0;
		for (int i = 1; i <= subStep; i++) {
			if (curt >= t * i) {
				spt = t * i;
			}
		}
		unsigned char r = 0;
		unsigned char g = static_cast<unsigned char>(255 * (1 - spt));
		unsigned char b = static_cast<unsigned char>(255 * spt);
		unsigned char a = 255; // 完全不透明

		RGBA rgba = { r, g, b, a };
		return rgba;
	}
	else {
		float half = (high + low) / 2;
		int subStep = step / 2;
		float t = 1.f / subStep;
		float curt = (cur - half) / (high-half);
		float spt = 0;
		for (int i = 1; i <= subStep; i++) {
			if (curt >= t * i) {
				spt = t * i;
			}
		}
		unsigned char r = static_cast<unsigned char>(255 * (spt));
		unsigned char g = 0;
		unsigned char b = static_cast<unsigned char>(255 * (1-spt));
		unsigned char a = 255; // 完全不透明

		RGBA rgba = { r, g, b, a };
		return rgba;
	}
}

void ShowVariPathss(const VariPathss& vpss,float power_high,float power_low) {
	Clipper2Lib::SvgWriter svg;//改色修改pencolor
	for (int i = 0; i < vpss.size(); i++) {
		for (int j = 0; j < vpss[i].MarkPoint.size(); j++) {
			if (j == 0) {
				Clipper2Lib::Path64 path64;
				path64.push_back(vpss[i].JumpPoint);
				path64.push_back(vpss[i].MarkPoint[0].point);
				Clipper2Lib::Paths64 paths64;
				paths64.push_back(path64);
				uint32_t color = RGBAToUnsigned(GetLinearColor(32, vpss[i].MarkPoint[0].power, power_low, power_high));
				svg.AddPaths(paths64, true, Clipper2Lib::FillRule::Negative, 0x00000000, color, 1.3, false);
			}
			else {
				Clipper2Lib::Path64 path64;
				path64.push_back(vpss[i].MarkPoint[j].point);
				path64.push_back(vpss[i].MarkPoint[j-1].point);
				Clipper2Lib::Paths64 paths64;
				paths64.push_back(path64);
				uint32_t color = RGBAToUnsigned(GetLinearColor(32, vpss[i].MarkPoint[j].power, power_low, power_high));
				svg.AddPaths(paths64,true, Clipper2Lib::FillRule::Negative, 0x00000000, color, 1.3, false);
			}
		}

	}
	Clipper2Lib::SvgSaveToFile(svg, "vpss.svg", 900, 1800, 20);
	System("vpss.svg");
}