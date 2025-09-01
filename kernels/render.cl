inline float linearToGamma(float linear_component) {
    if (linear_component > 0.0f)
        return sqrt(linear_component);
    return 0.0f;
}




inline bool hit_sphere(const ray r, float ray_tmin, float ray_tmax, sphereInfo sphere, hitRec* rec) {
	
	float3 center = sphere.m_center;
	float radius = sphere.m_radius;

    float3 oc = center - r.m_origin;
    float a = dot(r.m_dir, r.m_dir);
    float h = dot(r.m_dir, oc);
    float c = dot(oc, oc) - radius*radius;
    float discriminant = h*h - a*c;


    if(discriminant < 0){
        return 0;
    }

    float sqrtd = sqrt(discriminant);

    float root = (h - sqrtd) / a;
    if (root <= ray_tmin || ray_tmax <= root) {
        root = (h + sqrtd) / a;
        if (root <= ray_tmin || ray_tmax <= root){
            return false;
        }
    }

    rec->t = root;

    float3 temp = point3D_at(r, rec->t);
    rec->P = temp;
    float3 outwardNormal = (temp - center) / radius;
    bool frontFace = dot(r.m_dir, outwardNormal) < 0.0f;
    rec->front_face = frontFace;
    rec->normal = frontFace ? outwardNormal : -outwardNormal;

    rec->objID = sphere.objID;

    return true;
}

inline bool hitSomething(const ray r, float ray_tmin, float ray_tmax, hitRec* rec, __constant sphereInfo* spheresPointer, int numSpheres){

    hitRec tempRec;
    bool hitAnything = false;
    float closestSoFar = ray_tmax;

    for(int i = 0; i < numSpheres; i++){
        if(hit_sphere(r, ray_tmin, closestSoFar, spheresPointer[i], &tempRec)){
            hitAnything = true;
            closestSoFar = tempRec.t;
            *rec = tempRec;
            
        }
    }
    return hitAnything;

}

inline float3 rayColor(const ray r, float ray_tmin, float ray_tmax, __constant sphereInfo* spheresPointer, int numSpheres, int* seed){

    ray currentRay = r;

    hitRec rec;

    float3 color = (float3)(1, 1, 1); //start at full intensity

    for(int bounce = 0; bounce < 5; bounce++){
        if(!hitSomething(currentRay, ray_tmin, ray_tmax, &rec, spheresPointer, numSpheres)){

            float3 unit_direction = normalize(currentRay.m_dir);
            float a = 0.5 * (unit_direction.y + 1.0);
            return color * ((float3)(1.0f, 1.0f, 1.0f) * (1.0f - a) + (float3)(0.5f, 0.7f, 1.0f) * a);

        }
        else{
            float3 dir = rec.normal + randomUnitFloat3(seed);
            currentRay = ray_new(rec.P, dir);

            if(rec.objID == 1){
                color *= (float3)(0.2, 0.5, 0);
            }
            else if (rec.objID == 2){
                color *= 0.5;
            }
        }
    }
    return (float3)(0.0f, 0.0f, 0.0f);
}




__kernel void ray_trace(int task, __global float3* accum, int width, int height, __constant cameraInfo* cameraPtr, 
                        __constant sphereInfo* spheresPointer, int numSpheres, __global int* seed_memory, 
                        __global float* debug, __global uchar* output, int maxSamples, int samplesPerThread) {



    int i = get_global_id(0);
    int j = get_global_id(1);
    int pixel_idx = j * width + i;          
    int dst_idx = pixel_idx * 3;  
    int lid = get_local_id(0);



    if (i < width && j < height) {

        if(task == 0){
            accum[pixel_idx] = (float3)(0, 0, 0); 
            return;
        }

        if(task == 2){

            float inv = 1.0f / (float)maxSamples;
            float3 avg = accum[pixel_idx] * inv;
            float3 g  = (float3)(
                linearToGamma(avg.x),
                linearToGamma(avg.y),
                linearToGamma(avg.z)
            );
            output[dst_idx + 0] = (uchar)(g.x*255.99f);
            output[dst_idx + 1] = (uchar)(g.y*255.99f);
            output[dst_idx + 2] = (uchar)(g.z*255.99f);


            return;
        }

        int global_id = j * get_global_size(0) + i;
        int seed = seed_memory[pixel_idx];

    

        cameraInfo cam = cameraPtr[0];
        float3 pixel00 = cam.pixel00;
        float3 delta_u = cam.delta_u;
        float3 delta_v = cam.delta_v;
        float3 cameraCenter = cam.camera_center;



        float3 pixelCenter;
        ray newRay;
        float3 pixel_color = (float3)(0.0f, 0.0f, 0.0f);

        for(int sample = 0; sample < samplesPerThread; sample++){

            pixelCenter = pixel00 + (delta_u * ((float)i + rand(&seed) + 0.5f)) + (delta_v * ((float)j + rand(&seed) + 0.5f));
            
            newRay.m_origin = pixelCenter;
            newRay.m_dir = pixelCenter - cameraCenter;
            pixel_color += rayColor(newRay, 0.001f, 100000000.0f, spheresPointer, numSpheres, &seed);

        }

        seed_memory[pixel_idx] = seed;

        float3 sum = (float3)(pixel_color.x, pixel_color.y, pixel_color.z);
        accum[pixel_idx] += sum;
    }
    
}
