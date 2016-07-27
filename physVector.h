#ifndef VECTOR_H
#define VECTOR_H
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <iostream>


class physVector {
	private:
		int dimensions;
		double components [10]; //10 dimensions max
		double magnitude;

	public:
		//constructors
		physVector();
		physVector(int dim);
		physVector(int dim, double comp[10]);

		void setComponent(int dim, double val);
		void setDimensions(int dim);

		double getComponent(int dim);
		double getMagnitude();
		int getDimensions();

		void updateMagnitude();

		physVector operator+(const physVector& vect);
		physVector operator-(const physVector& vect);
		physVector operator*(double val);
		double operator^(const physVector& vect); //dot product
		physVector operator/(const physVector& vect); //cross product
		double operator%(const physVector& vect); //angle between vectors
		physVector unit();

		void print();

		/*
		To add:
		=,
		*/
};

physVector::physVector(){
	dimensions = 0;
	magnitude = 0;
	memset(components, 0, sizeof(components));
}

physVector::physVector(int dim){
	dimensions = dim;
	if (dimensions > 10 || dimensions < 1) {
		dimensions = 0;
	}
	magnitude = 0;
	memset(components, 0, sizeof(components));
}
physVector::physVector(int dim, double comp[10]) {
	dimensions = dim;
	memset(components, 0, sizeof(components));
	if (dimensions > 10 || dimensions < 1) {
		dimensions = 1;
	}
	magnitude = 0;
	for (int i = 0; i < dimensions; i++) {
		components[i] = comp[i];
		magnitude += pow(components[i], 2);
	}
	magnitude = sqrt(magnitude);
}

void physVector::setComponent(int dim, double val) {
	if (dim >= 0 && dim < 10) {
        if(dimensions<=dim){
            dimensions = dim+1;
        }
		components[dim] = val;
		updateMagnitude();
	}
}

void physVector::setDimensions(int dim) {
	if (dim >= 0 && dim < 10) {
		dimensions = dim;
		magnitude = 0;
	}
}

double physVector::getComponent(int dim) {
	if (dim >= 0 && dim < 10) {
		return components[dim];
	}
}

double physVector::getMagnitude() {
	return magnitude;
}

int physVector::getDimensions() {
	return dimensions;
}

void physVector::updateMagnitude() {
	magnitude = 0;
	for (int i = 0; i < dimensions; i++) {
		magnitude += pow(components[i], 2);
	}
	magnitude = sqrt(magnitude);
}

physVector physVector::operator+(const physVector& vect) {
	physVector ans;
	if (this->dimensions >= vect.dimensions) {
		ans.setDimensions(this->dimensions);
		for (int i = 0; i < vect.dimensions; i++) {
			ans.setComponent(i, this->components[i] + vect.components[i]);
		}
		for (int i = vect.dimensions; i < this->dimensions; i++) {
			ans.setComponent(i, this->components[i]);
		}
	}
	else {
		ans.setDimensions(vect.dimensions);
		for (int i = 0; i < this->dimensions; i++) {
			ans.setComponent(i, this->components[i] + vect.components[i]);
		}
		for (int i = this->dimensions; i < vect.dimensions; i++) {
			ans.setComponent(i, vect.components[i]);
		}
	}
	return ans;
}

physVector physVector::operator-(const physVector& vect) {
	physVector ans;
	if (this->dimensions >= vect.dimensions) {
		ans.setDimensions(this->dimensions);
		for (int i = 0; i < vect.dimensions; i++) {
			ans.setComponent(i, this->components[i] - vect.components[i]);
		}
		for (int i = vect.dimensions; i < this->dimensions; i++) {
			ans.setComponent(i, this->components[i]);
		}
	}
	else {
		ans.setDimensions(vect.dimensions);
		for (int i = 0; i < this->dimensions; i++) {
			ans.setComponent(i, this->components[i] - vect.components[i]);
		}
		for (int i = this->dimensions; i < vect.dimensions; i++) {
			ans.setComponent(i, 0 - vect.components[i]);
		}
	}
	return ans;
}

physVector physVector::operator*(double val){
    physVector ans;
    for(int i=0; i<this->dimensions; i++){
        ans.setComponent(i, this->components[i]*val);
    }
    return ans;
}

double physVector::operator^(const physVector& vect) {
	if (this->dimensions != vect.dimensions) {
		return 0.0;
	}
	double ans = 0;
	for (int i = 0; i < this->dimensions; i++) {
		ans += (this->components[i] * vect.components[i]);
	}
	return ans;
}
physVector physVector::operator/(const physVector& vect) {
	physVector ans;
	if (this->dimensions != vect.dimensions) {
		return ans;
	}
	if (this->dimensions != 2 && this->dimensions != 3) {
		return ans;
	}
	if (this->dimensions == 2) {
		ans.setDimensions(2);
		ans.setComponent(0, this->components[0] * vect.components[1]);
		ans.setComponent(1, -1 * (this->components[1] * vect.components[0]));
	}
	else {
		ans.setDimensions(3);
		ans.setComponent(0, this->components[1] * vect.components[2] - this->components[2] * vect.components[1]);
		ans.setComponent(1, this->components[2] * vect.components[0] - this->components[0] * vect.components[2]);
		ans.setComponent(2, this->components[0] * vect.components[1] - this->components[1] * vect.components[0]);
	}
	return ans;
}
double physVector::operator%(const physVector& vect) {
    /*
	double scalar = (*this)^vect;
	if (scalar > -0.00000000001 && scalar < 0.00000000001) {
		return 90;
	}
	double angle = acos(scalar / (this->magnitude * vect.magnitude));
    angle *=180/M_PI;
    //check if the angle is greater than 180


	return angle*180/M_PI;
    */
    //HARDCODED FOR 2 DIMENSIONS, USING X AND Z
    //obtain true angle for vector 1
    double angle1 = 0;
    if((this->components[0]<-0.0001 || this->components[0] > 0.0001) || (this->components[2]<-0.0001 || this->components[2]>0.0001)){
        angle1= atan2(this->components[2], this->components[0])*180.0/M_PI;
    }
    if(angle1 <0){
        angle1 = 360+angle1;
    }
    //obtain true angle for vector 2
    double angle2 = 0;
    if((vect.components[0]<-0.0001 || vect.components[0] > 0.0001) || (vect.components[2]<-0.0001 || vect.components[2]>0.0001)){
            angle2 = atan2(vect.components[2], vect.components[0])*180.0/M_PI;
    }
    if(angle2 < 0){
        angle2 = 360+angle2;
    }
    return abs(angle1-angle2);
}

physVector physVector::unit() {
	physVector ans(this->dimensions);
	double mag = this->magnitude;
	for (int i = 0; i < this->dimensions; i++) {
		ans.setComponent(i, this->components[i] / mag);
	}
	return ans;
}

void physVector::print(){
	for(int i=0; i<dimensions; i++){
		std::cout<<i<<": "<<components[i]<<"\t";
	}
	std::cout<<std::endl;
}

#endif

