typedef struct {
	float3 pixel00;
	float3 delta_u;
	float3 delta_v;
	float3 camera_center;
} cameraInfo;

typedef struct __attribute__((packed)){
	float3 P;
	float3 normal;
	float t;
	int objID;
	bool front_face;

} hitRec;

 
typedef struct __attribute__((packed)){
	float3 m_center;
	float m_radius;
	hitRec hitRecord;
	int objID;

} sphereInfo;

inline float rand(int* seed) {
    int const a = 16807; 
    int const m = 2147483647; 
    int const q = m / a;
    int const r = m % a;

    int k = *seed / q;
    *seed = a * (*seed - k * q) - r * k;

    if (*seed < 0) {
        *seed += m;
    }

    return (float)(*seed) / (float)m; 
}

inline float randMinMax(float min, float max, int* seed) {
    return min + (max - min) * rand(seed);
}

inline float3 float3_randomMinMax(float min, float max, int* seed){

    return (float3)(randMinMax(min, max, seed), randMinMax(min, max, seed), randMinMax(min, max, seed));
}

inline float3 randomUnitFloat3(int* seed) {
    float x1, x2, s;
    do {
        x1 = randMinMax(-1.0f, 1.0f, seed);
        x2 = randMinMax(-1.0f, 1.0f, seed);
        s = x1*x1 + x2*x2;
    } while (s >= 1.0f);
    
    float sqrt_val = sqrt(1.0f - s);
    return (float3)(2.0f * x1 * sqrt_val,
                   2.0f * x2 * sqrt_val,
                   1.0f - 2.0f * s);
}