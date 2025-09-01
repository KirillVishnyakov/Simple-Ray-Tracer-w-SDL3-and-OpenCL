#ifndef CAMERA_H
#define CAMERA_H





class camera {

public:

    double aspect_ratio;
    int    image_width;
    int    image_height;
    int    max_depth = 50;
    double vfov = 60;

    point3D lookfrom = point3D(0, 0, 1);   
    point3D lookat = point3D(0, 0, -1);  
    vec3    vup = vec3(0, 1, 0);     

       
    point3D camera_center;   
    point3D pixel_00;  
    vec3   delta_u;  
    vec3   delta_v;  
    vec3   u, v, w;

    void initialize() {

        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        camera_center = lookfrom;
        auto focal_length = (lookfrom - lookat).length();
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2 * h * focal_length;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        vec3 viewport_u = viewport_width * u;
        vec3 viewport_v = viewport_height * -v;

        delta_u = viewport_u / image_width; 
        delta_v = viewport_v / image_height; 

        vec3 viewUpperLeft = camera_center - (focal_length * w) - viewport_v / 2 - viewport_u / 2;
        pixel_00 = viewUpperLeft + 0.5 * (delta_u + delta_v);

    }

};



#endif 