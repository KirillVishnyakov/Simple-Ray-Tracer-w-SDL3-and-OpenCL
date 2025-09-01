typedef struct {
    float3 m_origin;
    float3 m_dir;
} ray;

static inline ray ray_new(const float3 origin, const float3 dir) {
    ray r;
    r.m_origin = origin;
    r.m_dir = dir;
    return r;
}

static inline ray ray_zero() {
    return ray_new((float3)(0.0f, 0.0f, 0.0f), (float3)(0.0f, 0.0f, 0.0f));
}

static inline float3 point3D_at(const ray r, const float t) {
    return r.m_origin + r.m_dir * t;
}