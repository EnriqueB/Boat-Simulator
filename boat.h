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

    public:
       boat();
        boat(double x, double y);
        //setters
        void setPosition(physVector pos);
        void setRudder(double r);
        void setSail(double s);

        //getters
        physVector getPosition();
        double getRudder();
        double getSail();

        void moveBoat(physVector wind, physVector tide, double direction, long long timeStep);
};

boat::boat(){
    position.setDimensions(3);
    rudder = 0;
    sail = 0;
}

boat::boat(double x, double y){
    position.setComponent(0, x);
    position.setComponent(2, y);
    position.setComponent(1, 0);
    rudder = 0;
    sail = 0;
}

void boat::setPosition(physVector pos){
    position = pos;
}

void boat::setRudder(double r){
    rudder = r;
}

void boat::setSail(double s){
    sail = s;
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

void boat::moveBoat(physVector wind, physVector tide, double direction, long long timeStep){
    position = position + tide;
    //leeway, depends on boat


    //direction going

    //get angle of wind
    double speed;
    physVector north(3);
    north.setComponent(0, 1);
    double windAngle = (wind)%north;
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
        speed = (180.0-BoatWindAngle)/(180.0-95.0);
    }
    speed *= wind.getMagnitude()/60.0;
    physVector directionVector(3);
    directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
    directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed);
    directionVector.print();

    position = position + (directionVector);

    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*3.141600))/150.0);
}

#endif
