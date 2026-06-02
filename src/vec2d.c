/* 
 * vec2d - Vector 2D
 */

#include <math.h>

#include "vec2d.h"

Vec2 vec2_subtract(Vec2 v1, Vec2 v2)
{
    Vec2 result = { v2.x - v1.x, v2.y - v1.y };

    return result;
}

float vec2_length(Vec2 v)
{
    float result = sqrtf((v.x * v.x) + (v.y * v.y));

    return result;
}

float vec2_distance_sqr(Vec2 v1, Vec2 v2)
{
    Vec2 d = vec2_subtract(v1, v2);
    float result = (d.x * d.x) + (d.y * d.y);

    return result;
}

Vec2 vec2_normalize(Vec2 v)
{
    float len = vec2_length(v);

    if (len > 0.0f) {
        v.x /= len;
        v.y /= len;
    }

    return v;
}

Vec2 vec2_scale(Vec2 v, float scale)
{
    v.x *= scale;
    v.y *= scale;

    return v;
}

