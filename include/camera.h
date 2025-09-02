#ifndef CAMERA_H
#define CAMERA_H





class camera {

public:

    double aspect_ratio;
    int    image_width;
    int    image_height;
    float viewPortToImageRatio;
    int    max_depth = 50;
    double vfov = 60;


    point3D lookfrom;
    point3D lookat;
    vec3    vup = vec3(0, 1, 0);     

       
    point3D camera_center;   
    point3D pixel_00;  
    vec3   delta_u;  
    vec3   delta_v;  
    vec3   u, v, w;
    vec3 viewport_u;
    vec3 viewport_v;
    vec3 viewUpperLeft;

    void initialize() {

        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        camera_center = lookfrom;
        auto focal_length = (lookfrom - lookat).length();
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        viewPortToImageRatio = image_width / viewport_width;



        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        viewport_u = viewport_width * u;
        viewport_v = viewport_height * -v;

        delta_u = viewport_u / image_width; 
        delta_v = viewport_v / image_height; 

        viewUpperLeft = camera_center - (focal_length * w) - viewport_v / 2 - viewport_u / 2;
        pixel_00 = viewUpperLeft + 0.5 * (delta_u + delta_v);

        /*std::cout << "viewUpperLeft: x: " << viewUpperLeft.x() << ", y: " << viewUpperLeft.y() << ", z:" << viewUpperLeft.z() << "\n";
        std::cout << "Pixel00: x: " << pixel_00.x() << ", y: " << pixel_00.y() << ", z:" << pixel_00.z() << "\n";
        std::cout << "delta_u: x: " << delta_u.x() << ", y: " << delta_u.y() << ", z:" << delta_u.z() << "\n";
        std::cout << "delta_v: x: " << delta_v.x() << ", y: " << delta_v.y() << ", z:" << delta_v.z() << "\n";
        std::cout << "viewport_height: " << viewport_height << ", viewport_width: " << viewport_width << "\n";
        std::cout << "viewPortToImageRatio: " << viewPortToImageRatio << "\n\n";*/

    }
    void updateCamera(vec3& delta, float dimensionCorrector) {

        vec3 newLookAt = lookat + delta / viewPortToImageRatio / dimensionCorrector;
        
        lookat = newLookAt;
        initialize();
        

        //std::cout << "CurrentLookAt: " << lookat << ", centeredNewLookAtVec: "<< newLookAt << "\n";

    }

};



#endif 