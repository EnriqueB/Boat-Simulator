#ifndef BOAT_H
#define BOAT_H

#include "physVector.h"
#include "battery.h"

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

        void moveBoat(physVector wind, physVector tide, physVector direction, long long timeStep);
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

void boat::moveBoat(physVector wind, physVector tide, physVector direction, long long timeStep){
    position = position + tide;
    //leeway, depends on boat


    //direction going

    //get angle of wind
    double speed;
    double windAngle = direction%(wind*-1);
    if (windAngle> 180.0){
        windAngle = 360-windAngle;
    }
    if (windAngle <= 25.0){
        //no go zone
        speed = 0;
    }
    else if(windAngle > 25.0 && windAngle <=85.0){
        speed = (windAngle-25.0)/(85.0-25.0);
    }
    else if(windAngle > 85.0 && windAngle <=95.0){
        speed = 0.9;
    }

    else if(windAngle > 95.0){
        speed = (180.0-windAngle)/(180.0-95.0);
    }
    speed *= wind.getMagnitude()*500;  //was 200  ...??

    position = position + (direction)*(speed);

    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*.31416))/10000);
}

#endif // BOAT_H
