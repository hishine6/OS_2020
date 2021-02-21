#ifndef FT_H
#define FT_H

#include<stdio.h>
#define FRAC (1<<14)
//int + float
int int_plus_float(int,int);
int int_minus_float(int,int);
int int_mul_float(int,int);
int float_div_int(int,int);
int float_plus_float(int,int);
int float_minus_float(int,int);
int float_mul_float(int,int);
int float_div_float(int,int);


int int_plus_float(int i, int f){
	return i*FRAC + f;
}

//int - float
int int_minus_float(int i, int f){
	return i*FRAC - f;
}

//int x float
int int_mul_float(int i, int f){
  return i*f;
}

//float / int
int float_div_int(int f, int i){
	return f/i;
}


//float + float
int float_plus_float(int f1, int f2){
	return f1+f2;
}

//float - float
int float_minus_float(int f1, int f2){
	return f1-f2;
}

//float x float
int float_mul_float(int f1, int f2){
	int64_t temp=f1;
	temp = temp*f2;
	temp = temp/FRAC;
	return (int)temp;
}

//float / float
int float_div_float(int f1, int f2){
	int64_t temp=f1;
	temp = temp*FRAC;
	temp = temp/f2;
	return (int)temp;
}

#endif
