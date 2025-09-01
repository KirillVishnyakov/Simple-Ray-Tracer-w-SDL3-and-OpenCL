#include "utils.h"

#include <fstream>
#include <vector>


class render {

	camera cam;
	int m_width;
	int m_height;



	double aspectRatio = 16.0 / 9.0;

public:

	struct CameraState {
		cl_float3 pixel00;
		cl_float3 delta_u;
		cl_float3 delta_v;
		cl_float3 camera_center;
		
	} cameraInfo;
#pragma pack(push, 1)
	struct hitRec {
		cl_float3 P;
		cl_float3 normal;
		cl_float t;
		cl_int objID;
		bool front_face;

	};
#pragma pack(pop)
#pragma pack(push, 1)
	struct SphereInfo {
		cl_float3 m_center;
		cl_float m_radius;
		hitRec sphereHitRecord;
		cl_int objID;

	};
#pragma pack(pop)

	SphereInfo sphereOneInfo;
	SphereInfo sphereTwoInfo;
	
		render(int width, int height, int camX = 0, int camY = 0, int camZ = 1) : m_width{ width }, m_height{height} {
			cam.aspect_ratio = aspectRatio;
			cam.image_width = width;

			point3D sphereCenterOne = point3D(0, -200.5, -3.2);
			float r1 = 199;


			point3D sphereCenterTwo = point3D(0, -0.6, -3.2);
			float r2 = 1;
			


			cam.lookfrom = point3D(camX, camY, camZ);
			cam.lookat = point3D(0, 0, -1);
			cam.initialize();


			
			cameraInfo.pixel00.x = (float)cam.pixel_00.x();
			cameraInfo.pixel00.y = (float)cam.pixel_00.y();
			cameraInfo.pixel00.z = (float)cam.pixel_00.z();

			cameraInfo.delta_u.x = (float)cam.delta_u.x();
			cameraInfo.delta_u.y = (float)cam.delta_u.y();
			cameraInfo.delta_u.z = (float)cam.delta_u.z();

			cameraInfo.delta_v.x = (float)cam.delta_v.x();
			cameraInfo.delta_v.y = (float)cam.delta_v.y();
			cameraInfo.delta_v.z = (float)cam.delta_v.z();

			cameraInfo.camera_center.x = (float)cam.camera_center.x();
			cameraInfo.camera_center.y = (float)cam.camera_center.y();
			cameraInfo.camera_center.z = (float)cam.camera_center.z();

			//sphere1
			sphereOneInfo.m_center.x = sphereCenterOne.x();
			sphereOneInfo.m_center.y = sphereCenterOne.y();
			sphereOneInfo.m_center.z = sphereCenterOne.z();
			sphereOneInfo.m_radius = r1;
			sphereOneInfo.objID = 1;

			//sphere2
			sphereTwoInfo.m_center.x = sphereCenterTwo.x();
			sphereTwoInfo.m_center.y = sphereCenterTwo.y();
			sphereTwoInfo.m_center.z = sphereCenterTwo.z();
			sphereTwoInfo.m_radius = r2;
			sphereTwoInfo.objID = 2;

			//std::cout << sphereOneInfo.m_center.x << ", " << sphereOneInfo.m_center.y << ", " << sphereOneInfo.m_center.z << ", " << sphereOneInfo.m_radius << "\n";
			//std::cout << sphereTwoInfo.m_center.x << ", " << sphereTwoInfo.m_center.y << ", " << sphereTwoInfo.m_center.z << ", " <<  sphereTwoInfo.m_radius << "\n";
			
	
		}
};