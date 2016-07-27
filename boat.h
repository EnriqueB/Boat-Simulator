#ifndef BOAT_H
#define BOAT_H

#include "physVector.h"
#include "battery.h"
#include <iostream>

class boat{

    private:
        physVector position;
        double rudder;
        double sail;
    	battery bat;
	    double direction;

    public:
       boat();
        boat(double x, double y, double dir);
        //setters
        void setPosition(physVector pos);
        void setRudder(double r);
        void setSail(double s);
	void setDirection(double a);

        //getters
        physVector getPosition();
        double getRudder();
        double getSail();
	double getDirection();

        void moveBoat(physVector wind, physVector tide, long long timeStep);
};

boat::boat(){
    position.setDimensions(3);
    rudder = 90;
    sail = 0;
    direction = 0;
}

boat::boat(double x, double y, double dir){
    position.setComponent(0, x);
    position.setComponent(2, y);
    position.setComponent(1, 0);
    rudder = 90;
    sail = 0;
    direction = dir;
}

void boat::setPosition(physVector pos){
    position = pos;
}

void boat::setRudder(double r){
    rudder = r;
    if(rudder < 20){
	    rudder = 20;
    }
    else if(rudder >160){
        rudder = 160;
    }
}

void boat::setSail(double s){
    sail = s;
}

void boat::setDirection(double a){
    direction = a;
    if(direction>360){
        direction -= 360;
    }
    if(direction<0){
        direction += 359.999;
    }
}

physVector boat::getPosition(){
    return position;
}

double boat::getRudder(){
    return rudder;
}

double boat::getSail(){
    return sail;
}

double boat::getDirection(){
    return direction;
}

void boat::moveBoat(physVector wind, physVector tide, long long timeStep){
    position = position + tide;
    //leeway, depends on boat


    //direction going

    //get angle of wind
    double speed=0;
    physVector north(3);
    north.setComponent(0, 1);
    double windAngle = (wind*-1.0)%north;
    //compare windAngle to boatAngle
    double BoatWindAngle = abs(windAngle-direction);

    if (BoatWindAngle> 180.0){
        BoatWindAngle = 360-BoatWindAngle;
    }
    if (BoatWindAngle <= 25.0){
        //no go zone
        speed = 0;
    }
    else if(BoatWindAngle > 25.0 && BoatWindAngle <=85.0){
        speed = (BoatWindAngle-25.0)/(85.0-25.0);
    }
    else if(BoatWindAngle > 85.0 && BoatWindAngle <=95.0){
        speed = 0.9;
    }

    else if(BoatWindAngle > 95.0){
        speed = (0.7-1)/(180-95)*BoatWindAngle + (1-((0.7-1)/(180-95)*95));
    }
    speed *= wind.getMagnitude()/60.0;

    //account for rudder
    if(rudder < 90){
	    direction-=((90.0-rudder)/600.0);
	    if(direction > 359.9999){
		    direction -=360.0;
	    }
    }
    else{
	    direction+=((rudder-90.0)/600.0);
	    if(direction<0.0001){
		    direction+=360.0;
	    }
    }
    physVector directionVector(3);
    directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
    directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed); //Z axis has positive values towards viewer

    position = position + (directionVector);
    if(timeStep%10==0){
        directionVector.print();
        std::cout<<"Rudder: "<<rudder<<" Direction: "<<direction<<" Speed: "<<speed<<" BoatWingAngle: "<<BoatWindAngle<<"\n";
    }
    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*3.141600))/150.0);
}

#endif
