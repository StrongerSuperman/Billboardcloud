#ifndef LINEARALGEBRA_H
#define LINEARALGEBRA_H

#include <glm/glm.hpp>
#include <eigen/Dense>
#include <vector>

static std::vector<Eigen::Vector3f> trans_vector(std::vector<glm::vec3> points)
{
	std::vector<Eigen::Vector3f> data;
	for (auto point : points)
	{
		data.emplace_back(Eigen::Vector3f(point.x, point.y, point.z));
	}
	return data;
}

static std::vector<glm::vec3> rever_trans_vector(std::vector<Eigen::Vector3f> points)
{
	std::vector<glm::vec3> data;
	for (auto point : points)
	{
		data.emplace_back(glm::vec3(point(0), point(1), point(2)));
	}
	return data;
}

static Eigen::Vector3f trans(glm::vec3 point)
{
	return Eigen::Vector3f(point.x, point.y, point.z);
}

static glm::vec3 rever_trans(Eigen::Vector3f point)
{
	return glm::vec3(point(0), point(1), point(2));
}

static std::pair<glm::vec3, glm::vec3> best_plane_from_points(const std::vector<glm::vec3>& points)
{
	auto c = trans_vector(points);

	// copy coordinates to matrix in Eigen format
	size_t num_atoms = c.size();
	//Eigen::Matrix< Vector3::Scalar, Eigen::Dynamic, Eigen::Dynamic > coord(3, num_atoms);
	Eigen::MatrixXf coord(3, num_atoms);
	for (size_t i = 0; i < num_atoms; ++i) coord.col(i) = c[i];

	// calculate centroid
	Eigen::Vector3f centroid(coord.row(0).mean(), coord.row(1).mean(), coord.row(2).mean());

	// subtract centroid
	coord.row(0).array() -= centroid(0); coord.row(1).array() -= centroid(1); coord.row(2).array() -= centroid(2);

	// we only need the left-singular matrix here
	//  http://math.stackexchange.com/questions/99299/best-fitting-plane-given-a-set-of-points
	Eigen::JacobiSVD<Eigen::MatrixXf> svd(coord, Eigen::ComputeThinU | Eigen::ComputeThinV);
	//auto svd = coord.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV);
	Eigen::Vector3f plane_normal = svd.matrixU().rightCols<1>();

	return std::make_pair(rever_trans(centroid), rever_trans(plane_normal));
}

#endif // !LINEARALGEBRA_H
