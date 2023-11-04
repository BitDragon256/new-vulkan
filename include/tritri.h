#pragma once

// returns 1 on intersection, 0 on no intersection
int tri_tri_intersect(float V0[3], float V1[3], float V2[3],
					  float U0[3], float U1[3], float U2[3]);

// little faster tri-tri intersection (without divisions)
int NoDivTriTriIsect(float V0[3], float V1[3], float V2[3],
					 float U0[3], float U1[3], float U2[3]);

// This version computes the line of intersection as well(if they are not coplanar) :
int tri_tri_intersect_with_isectline(float V0[3], float V1[3], float V2[3],
									 float U0[3], float U1[3], float U2[3], int* coplanar,
									 float isectpt1[3], float isectpt2[3]);
