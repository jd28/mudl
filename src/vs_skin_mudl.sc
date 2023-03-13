$input a_position, a_texcoord0, a_normal, a_tangent, a_indices, a_weight
$output v_texcoord0

uniform mat4 u_joints[64];

#include "common.sh"

void main()
{
    mat4 model = mul(u_model[0], a_weight.x * u_joints[int(a_indices.x)] +
        a_weight.y * u_joints[int(a_indices.y)] +
        a_weight.z * u_joints[int(a_indices.z)] +
        a_weight.w * u_joints[int(a_indices.w)]);

    vec3 v_wpos = mul(model, vec4(a_position, 1.0) ).xyz;

    gl_Position = mul(u_viewProj, vec4(v_wpos, 1.0));
    v_texcoord0 = a_texcoord0;
}
