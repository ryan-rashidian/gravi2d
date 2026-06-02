/* 
 * vec2d - Vector 2D
 */

#ifndef VEC_H
#define VEC_H

typedef struct {
    float x, y;
} Vec2;

Vec2  vec2_subtract(Vec2 v1, Vec2 v2);
float vec2_length(Vec2 v);
float vec2_distance_sqr(Vec2 v1, Vec2 v2);
Vec2  vec2_normalize(Vec2 v);
Vec2  vec2_scale(Vec2 v, float scale);

#endif

