#ifndef BIN_H
#define BIN_H

class Bin
{
public:
	float density;
	float thetaMin;
	float thetaMax;
	float phiMin;
	float phiMax;
	float roMin;
	float roMax;
	float thetaCenter;
	float phiCenter;
	float roCenter;
	glm::vec3 centerNormal;

	Bin()
		:density(0.0f),
		thetaMin(0.0f),
		thetaMax(0.0f),
		phiMin(0.0f),
		phiMax(0.0f),
		roMin(0.0f),
		roMax(0.0f),
		thetaCenter(0.0f),
		phiCenter(0.0f),
		roCenter(0.0f)
	{
	}

	Bin(float _thetaMin, float _thetaMax, float _phiMin, float _phiMax, float _roMin, float _roMax, float _density = 0)
		:thetaMin(_thetaMin), thetaMax(_thetaMax), phiMin(_phiMin), phiMax(_phiMax), roMin(_roMin), roMax(_roMax), density(_density)
	{
		calcuCenter();
		centerSphericalCoordToNormal();
	}

	Bin(const Bin& bin)
	{
		density = bin.density;
		thetaMin = bin.thetaMin;
		thetaMax = bin.thetaMax;
		phiMin = bin.phiMin;
		phiMax = bin.phiMax;
		roMin = bin.roMin;
		roMax = bin.roMax;
		thetaCenter = bin.thetaCenter;
		phiCenter = bin.phiCenter;
		roCenter = bin.roCenter;
		centerNormal = bin.centerNormal;
	}

	Bin& operator=(const Bin& bin)
	{
		density = bin.density;
		thetaMin = bin.thetaMin;
		thetaMax = bin.thetaMax;
		phiMin = bin.phiMin;
		phiMax = bin.phiMax;
		roMin = bin.roMin;
		roMax = bin.roMax;
		thetaCenter = bin.thetaCenter;
		phiCenter = bin.phiCenter;
		roCenter = bin.roCenter;
		centerNormal = bin.centerNormal;
		return *this;
	}

private:
	void calcuCenter()
	{
		thetaCenter = (thetaMin + thetaMax) / 2;
		phiCenter = (phiMin + phiMax) / 2;
		roCenter = (roMin + roMax) / 2;
	}

	// calculate bin center's normal
	void centerSphericalCoordToNormal()
	{
		float z = glm::sin(phiCenter);
		float xy = glm::cos(phiCenter);
		float x = xy * glm::cos(thetaCenter);
		float y = xy * glm::sin(thetaCenter);

		centerNormal = glm::vec3(x, y, z);
	}
};

#endif // !BIN_H