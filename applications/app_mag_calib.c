#include <malloc.h>

#include "lib_math.h"

#include "app_debug.h"
#include "app_mag_calib.h"
#include "app_param_calib.h"

#define MAG_SAMPLE 3000

float *mx_cal = NULL;
float *my_cal = NULL;
float *mz_cal = NULL;

float mag_radius = 0.0f;
float mag_calib_val[3] = {0};
uint16_t mag_calib_count = 0;

void mag_calib_define_param(void)
{
	param_calib_define(MAG_OFFSET_X_NAME,MAG_OFFSET_X_DEF,&mag_calib_val[0]);
	param_calib_define(MAG_OFFSET_Y_NAME,MAG_OFFSET_Y_DEF,&mag_calib_val[1]);
	param_calib_define(MAG_OFFSET_Z_NAME,MAG_OFFSET_Z_DEF,&mag_calib_val[2]);
	param_calib_define(MAG_RADIUS_NAME,MAG_RADIUS_DEF,&mag_radius);	
}

void mag_calib_start(void)
{
	mx_cal = malloc(sizeof(float)*MAG_SAMPLE);
	my_cal = malloc(sizeof(float)*MAG_SAMPLE);
	mz_cal = malloc(sizeof(float)*MAG_SAMPLE);
	mag_calib_count = 0;
}

int sphere_fit_least_squares(const float x[], const float y[], const float z[],
			     unsigned int size, unsigned int max_iterations, float delta, float *sphere_x, float *sphere_y, float *sphere_z, float *sphere_radius)
{

	float x_sumplain = 0.0f;
	float x_sumsq = 0.0f;
	float x_sumcube = 0.0f;

	float y_sumplain = 0.0f;
	float y_sumsq = 0.0f;
	float y_sumcube = 0.0f;

	float z_sumplain = 0.0f;
	float z_sumsq = 0.0f;
	float z_sumcube = 0.0f;

	float xy_sum = 0.0f;
	float xz_sum = 0.0f;
	float yz_sum = 0.0f;

	float x2y_sum = 0.0f;
	float x2z_sum = 0.0f;
	float y2x_sum = 0.0f;
	float y2z_sum = 0.0f;
	float z2x_sum = 0.0f;
	float z2y_sum = 0.0f;

	unsigned int i;
	for (i = 0; i < size; i++) {

		float x2 = x[i] * x[i];
		float y2 = y[i] * y[i];
		float z2 = z[i] * z[i];

		x_sumplain += x[i];
		x_sumsq += x2;
		x_sumcube += x2 * x[i];

		y_sumplain += y[i];
		y_sumsq += y2;
		y_sumcube += y2 * y[i];

		z_sumplain += z[i];
		z_sumsq += z2;
		z_sumcube += z2 * z[i];

		xy_sum += x[i] * y[i];
		xz_sum += x[i] * z[i];
		yz_sum += y[i] * z[i];

		x2y_sum += x2 * y[i];
		x2z_sum += x2 * z[i];

		y2x_sum += y2 * x[i];
		y2z_sum += y2 * z[i];

		z2x_sum += z2 * x[i];
		z2y_sum += z2 * y[i];
	}

	//
	//Least Squares Fit a sphere A,B,C with radius squared Rsq to 3D data
	//
	//    P is a structure that has been computed with the data earlier.
	//    P.npoints is the number of elements; the length of X,Y,Z are identical.
	//    P's members are logically named.
	//
	//    X[n] is the x component of point n
	//    Y[n] is the y component of point n
	//    Z[n] is the z component of point n
	//
	//    A is the x coordiante of the sphere
	//    B is the y coordiante of the sphere
	//    C is the z coordiante of the sphere
	//    Rsq is the radius squared of the sphere.
	//
	//This method should converge; maybe 5-100 iterations or more.
	//
	float x_sum = x_sumplain / size;        //sum( X[n] )
	float x_sum2 = x_sumsq / size;    //sum( X[n]^2 )
	float x_sum3 = x_sumcube / size;    //sum( X[n]^3 )
	float y_sum = y_sumplain / size;        //sum( Y[n] )
	float y_sum2 = y_sumsq / size;    //sum( Y[n]^2 )
	float y_sum3 = y_sumcube / size;    //sum( Y[n]^3 )
	float z_sum = z_sumplain / size;        //sum( Z[n] )
	float z_sum2 = z_sumsq / size;    //sum( Z[n]^2 )
	float z_sum3 = z_sumcube / size;    //sum( Z[n]^3 )

	float XY = xy_sum / size;        //sum( X[n] * Y[n] )
	float XZ = xz_sum / size;        //sum( X[n] * Z[n] )
	float YZ = yz_sum / size;        //sum( Y[n] * Z[n] )
	float X2Y = x2y_sum / size;    //sum( X[n]^2 * Y[n] )
	float X2Z = x2z_sum / size;    //sum( X[n]^2 * Z[n] )
	float Y2X = y2x_sum / size;    //sum( Y[n]^2 * X[n] )
	float Y2Z = y2z_sum / size;    //sum( Y[n]^2 * Z[n] )
	float Z2X = z2x_sum / size;    //sum( Z[n]^2 * X[n] )
	float Z2Y = z2y_sum / size;    //sum( Z[n]^2 * Y[n] )

	//Reduction of multiplications
	float F0 = x_sum2 + y_sum2 + z_sum2;
	float F1 =  0.5f * F0;
	float F2 = -8.0f * (x_sum3 + Y2X + Z2X);
	float F3 = -8.0f * (X2Y + y_sum3 + Z2Y);
	float F4 = -8.0f * (X2Z + Y2Z + z_sum3);

	//Set initial conditions:
	float A = x_sum;
	float B = y_sum;
	float C = z_sum;

	//First iteration computation:
	float A2 = A * A;
	float B2 = B * B;
	float C2 = C * C;
	float QS = A2 + B2 + C2;
	float QB = -2.0f * (A * x_sum + B * y_sum + C * z_sum);

	//Set initial conditions:
	float Rsq = F0 + QB + QS;

	//First iteration computation:
	float Q0 = 0.5f * (QS - Rsq);
	float Q1 = F1 + Q0;
	float Q2 = 8.0f * (QS - Rsq + QB + F0);
	float aA, aB, aC, nA, nB, nC, dA, dB, dC;

	//Iterate N times, ignore stop condition.
	unsigned int n = 0;

	while (n < max_iterations) {
		n++;

		//Compute denominator:
		aA = Q2 + 16.0f * (A2 - 2.0f * A * x_sum + x_sum2);
		aB = Q2 + 16.0f * (B2 - 2.0f * B * y_sum + y_sum2);
		aC = Q2 + 16.0f * (C2 - 2.0f * C * z_sum + z_sum2);
		aA = (fabsf(aA) < FLT_EPSILON) ? 1.0f : aA;
		aB = (fabsf(aB) < FLT_EPSILON) ? 1.0f : aB;
		aC = (fabsf(aC) < FLT_EPSILON) ? 1.0f : aC;

		//Compute next iteration
		nA = A - ((F2 + 16.0f * (B * XY + C * XZ + x_sum * (-A2 - Q0) + A * (x_sum2 + Q1 - C * z_sum - B * y_sum))) / aA);
		nB = B - ((F3 + 16.0f * (A * XY + C * YZ + y_sum * (-B2 - Q0) + B * (y_sum2 + Q1 - A * x_sum - C * z_sum))) / aB);
		nC = C - ((F4 + 16.0f * (A * XZ + B * YZ + z_sum * (-C2 - Q0) + C * (z_sum2 + Q1 - A * x_sum - B * y_sum))) / aC);

		//Check for stop condition
		dA = (nA - A);
		dB = (nB - B);
		dC = (nC - C);

		if ((dA * dA + dB * dB + dC * dC) <= delta) { break; }

		//Compute next iteration's values
		A = nA;
		B = nB;
		C = nC;
		A2 = A * A;
		B2 = B * B;
		C2 = C * C;
		QS = A2 + B2 + C2;
		QB = -2.0f * (A * x_sum + B * y_sum + C * z_sum);
		Rsq = F0 + QB + QS;
		Q0 = 0.5f * (QS - Rsq);
		Q1 = F1 + Q0;
		Q2 = 8.0f * (QS - Rsq + QB + F0);
	}

	*sphere_x = A;
	*sphere_y = B;
	*sphere_z = C;
	*sphere_radius = sqrtf(Rsq);

	return 0;
}

bool mag_calib_update(float dt,float mag[3])
{
	if(mag_calib_count < MAG_SAMPLE){		
		mx_cal[mag_calib_count] = mag[0];
		my_cal[mag_calib_count] = mag[1];
		mz_cal[mag_calib_count] = mag[2]; 
		mag_calib_count++;		
		return false;
	}else{
		sphere_fit_least_squares(mx_cal,my_cal,mz_cal,MAG_SAMPLE,1000,0.0f,&mag_calib_val[0],&mag_calib_val[1],&mag_calib_val[2],&mag_radius);
		printf("[MCAL] magnetmeter calibration end ...\r\n");
		printf("[MCAL] new offset:%-8.3f,  %-8.3f,  %-8.3f,  %-8.3f\r\n",mag_calib_val[0],mag_calib_val[1],mag_calib_val[2],mag_radius);
		printf("please hold the plane on level ...\r\n");
		return true;
	}
}

void mag_calib_get_offset(float offset[3])
{
	offset[0] = mag_calib_val[0];
	offset[1] = mag_calib_val[1];
	offset[2] = mag_calib_val[2];
}

void mag_calib_save_param(void)
{
	param_calib_set(MAG_OFFSET_X_NAME,mag_calib_val[0]);
	param_calib_set(MAG_OFFSET_Y_NAME,mag_calib_val[1]);
	param_calib_set(MAG_OFFSET_Z_NAME,mag_calib_val[2]);
	param_calib_set(MAG_RADIUS_NAME,mag_radius);
	param_calib_save();
}

void mag_calib_end(void)
{
	if(mx_cal != NULL){
		free(mx_cal);
		mx_cal = NULL;
	}
	if(my_cal != NULL){
		free(my_cal);
		mx_cal = NULL;
	}
	if(mz_cal != NULL){
		free(mz_cal);
		mx_cal = NULL;
	}
	mag_calib_save_param();
}

float length3(float a,float b,float c)
{
	return sqrt(a * a + b * b + c* c);
}

bool mag_calib_check(float mag[3])
{
	if( ((mag_calib_val[0] * mag_calib_val[1] * mag_calib_val[2] * mag_radius) - 0.0f) < 1e-8f){
		return false;
	}else if(fabs(length3(mag[0],mag[1],mag[2]) / mag_radius - 1.0f) > 0.3f){
		return false;
	}

	return true;
}

uint8_t mag_callib_get_progress ()
{
	return (uint8_t)((float)mag_calib_count / MAG_SAMPLE * 100.0f);
}

