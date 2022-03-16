#pragma once

#include <cmath>
#include <vector>



#define one_six 0.166666667 // value of 1/6
#define _INFINITE_ 9999 // value of infinite
#define epsilon 1e-8 // threshold
#define quadrant_pi_radius 0.785398163 // pi/4 used for determine the prientation



class Vertex;
class Vector3d;
class Volume;
class RoofSurface;



// class to store vertex:
class Vertex {
public:
	double x;
	double y;
	double z;

	int vid; // might be used
public:
	Vertex() :
		x(0), y(0), z(0), vid(0){}

	Vertex(double X, double Y, double Z, int id) :
		x(X), y(Y), z(Z), vid(id){}

	Vertex(double X, double Y, double Z):
		x(X), y(Y), z(Z), vid(0) {}
};



// class to represent normal vector
class Vector3d : public Vertex 
{
public:
	Vector3d() :
		Vertex() {}

	Vector3d(double X, double Y, double Z):
		Vertex(X, Y, Z){}

public:

	/*
	* assign quadrant according to x y coordinates
	* @return: 1, 2, 3, 4 -- indicating four quadrants
	* if ">" not ">=", use: x+epsilon > 0, the same as "<"
	* compared with 0 can directly use > or >= ?
	*/
	static int assign_quadrant(double x, double y)
	{
		if (x > epsilon || abs(x) < epsilon) { // x > 0 or x = 0 : quadrant 1 or 4
			if (y > epsilon || abs(y) < epsilon)return 1; // y > 0 or y = 0 : quadrant 1
			else return 4; // y < 0 : quadrant 4
		}
		else { // quadrant 2 or 3
			if (y > epsilon || abs(y) < epsilon)return 2; // y > 0 or y = 0 : quadrant 2
			else return 3; // y < 0: quadrant 3
		}
	}


	// get cross product
	static Vector3d cross(Vector3d& v1, Vector3d& v2) {
		return Vector3d(
			(v1.y * v2.z - v1.z * v2.y),
			(v1.z * v2.x - v1.x * v2.z),
			(v1.x * v2.y - v1.y * v2.x)
		);
	}


	// get normal vector of one face
	// RoofVertices: store 3 vertices of one face, v1, v2, v3 should be oriented as CCW from outside
	static Vector3d find_normal(std::vector<Vertex>& RoofVertices) {
		
		// use the first 3 vertices to define two vectors of this roof surface
		// v1: starts from vertex[0], ends at vertex[1]
		// v2: starts from vertex[1], ends at vertex[2]
		Vector3d v1(
			(RoofVertices[1].x - RoofVertices[0].x),
			(RoofVertices[1].y - RoofVertices[0].y),
			(RoofVertices[1].z - RoofVertices[0].z)
		);

		Vector3d v2(
			(RoofVertices[2].x - RoofVertices[1].x),
			(RoofVertices[2].y - RoofVertices[1].y),
			(RoofVertices[2].z - RoofVertices[1].z)
		);

		return cross(v1, v2);
	}


	// 2x2 matrix:
	// x1 y1
	// x2 y2
	static double determinant_2x2(Vertex& v1, Vertex& v2)
	{
		return (v1.x * v2.y - v1.y * v2.x);
	}

};



// class related to volume
class Volume {
public:
	/*
	* calculate the determinant of one triangulated face
	*/
	static double calculate_determinant(Vertex& v1, Vertex& v2, Vertex& v3)
	{
		/*
		* matrix:
		* x1 y1 z1
		* x2 y2 z2
		* x3 y3 z3
		* D = x1 y2 z3 + y1 z2 x3 + z1 y3 x2 - z1 y2 x3 - y1 x2 z3 - x1 y3 z2
		*
		* matrix:
		* v1.x v1.y v1.z
		* v2.x v2.y v2.z
		* v3.x v3.y v3.z
		*/
		return
			v1.x * v2.y * v3.z +
			v1.y * v2.z * v3.x +
			v1.z * v3.y * v2.x -
			v1.z * v2.y * v3.x -
			v1.y * v2.x * v3.z -
			v1.x * v3.y * v2.z;
	}

	/*
	* calculate the "volume" of each solid
	* Vertices_one_solid: [[v1, v2, v3], [v4, v5, v6], ... []]
	*/
	static double calculate_volume_one_shell(
		std::vector<std::vector<Vertex>>& v_one_shell)
	{
		double sum_det = 0;

		for (auto& v_each_triangulated_face : v_one_shell)
		{
			/*
			* each v_each_triangulated_face: [vertex1, vertex2, vertex3]
			* oriented: CCW or CW but faces must be the same orientation
			*/
			sum_det += calculate_determinant(v_each_triangulated_face[0], v_each_triangulated_face[1], v_each_triangulated_face[2]);
		}

		/*
		* because absolute value is used in the fomula
		* the orientation of each triangulated face can be CCW or CW
		* but all the faces' orientations should be associated(all CCW or all CW)
		*/
		return one_six * abs(sum_det); // (1/6)*|sum_det|
	}

};



// class to store the roofsurface
class RoofSurface {
public:
	std::string BuildingPart_id; // belongs to which BuildingPart
	std::string type;

	int boundaries_index; // also semantics_values_index
	int semantics_surfaces_index;
		
	std::string orientation; // indicates orientaion
	double area; // indicates area

	std::vector<Vertex> exteriorSurface; // store the ecterior vertices, should be CCW from outside
	std::vector<std::vector<Vertex>> interiorSurfaces; // store the inner vertices, should be CW from outside
public:
	RoofSurface():
		BuildingPart_id("null"),
		type("RoofSurface"),
		boundaries_index(_INFINITE_),
		semantics_surfaces_index(_INFINITE_),
		orientation("null"),
		area(0)
	{}

public:

	/*
	* calculate and assign the orientation of roof surface
	* use y-axis as the North(North vector: [0, 1, 0])
	*/
	static std::string calculate_orientation(RoofSurface& roof)
	{
		// use exterior surface of current roof surface to get the normal vector
		Vector3d& normal = Vector3d::find_normal(roof.exteriorSurface);

		// situation cannot use alpha = arctan(x/y)
		// orientaion is either East or West(using 2d coordinates x, y to estimate the orientation)
		// in the orientation, only 8 values + "horizontal", E, W, N, S should be replaced with proper values(like EN)
		if (abs(normal.y) < epsilon) // y = 0
		{
			if (normal.x > epsilon)return "EN"; // x > 0 (y = 0)
			else if (abs(normal.x) < epsilon)return "horizontal"; // x = 0 (y = 0)
			else return "WN"; // x < 0 (y = 0)
		}

		// situation can use alpha = arctan(x/y)
		// double alpha = atan(abs_normal.x / abs_normal.y);
		// 
		// use normal to decide the quadrant
		// use abs_normal to calculate the angle
		// 
		// first assign the quadrant according to x,y signs
		int quadrant = Vector3d::assign_quadrant(normal.x, normal.y);

		// use abs_normal to get the angle(radius, value of atan(): [-pi, pi])
		double radius_angle = atan(abs(normal.x) / abs(normal.y));

		switch (quadrant)
		{
		case 1: // 1-th quadrant
			if (radius_angle > quadrant_pi_radius + epsilon) // > pi/4
				return "EN";
			else return "NE";
			break;
			
		case 2: // 2-th quadrant
			if (radius_angle > quadrant_pi_radius + epsilon) // > pi/4
				return "WN";
			else return "NW";
			break;

		case 3: // 3-th quadrant
			if (radius_angle > quadrant_pi_radius + epsilon) // > pi/4
				return "WS";
			else return "SW";
			break;

		case 4: // 4-th quadrant
			if (radius_angle > quadrant_pi_radius + epsilon) // > pi/4
				return "ES";
			else return "SE";
			break;

		default:
			break;
		}

		return "null"; // if no matching found, return null
	}



	/*
	* calculate the area of roof surface(2d)
	*/
	static double calculate_area_2d(RoofSurface& roof)
	{
		// First calculate the area of exterior
		//long N = (long)RoofVertices.size(); // DO NOT use size() - 1 directly, may cause subscript error

		//double sum_determinant = 0;
		//for (long i = 0; i != N - 1; ++i)
		//{
		//	sum_determinant += Vector3d::determinant_2x2(RoofVertices[i], RoofVertices[i + 1]);
		//}
		//sum_determinant += Vector3d::determinant_2x2(RoofVertices[N-1], RoofVertices[0]);

		//double result = 0.5 * abs(sum_determinant);

		//return result;
	}



	/*
	* calculate the area of roof surface(3d)
	* area must be >= 0
	*/
	static double calculate_area_3d(RoofSurface& roof)
	{
		return 0;
	}
};

