#ifndef BOAT_H
#define BOAT_H

#include "physVector.h"
#include "battery.h"
#include <string.h>
#include <iostream>

#define FPS 60.0

class boat{

    private:
        physVector position;
        double rudder;
        double sail;
    	battery bat;
	    double direction;
        double angle_sailingPoints[3];
        double speed_sailingPoints[3];

    public:
       boat();
        boat(double x, double y, double dir, double angles[]);
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
        double getSailingPoint(int index);

        void moveBoat(physVector wind, physVector tide, long long timeStep);
        double bestAngle(physVector wind, double startAngle, int tackLimit, physVector target);
};

boat::boat(){
    position.setDimensions(3);
    rudder = 90;
    sail = 0;
    direction = 0;
    memset(angle_sailingPoints, 0, sizeof(angle_sailingPoints));
    memset(speed_sailingPoints, 0, sizeof(speed_sailingPoints));

}

boat::boat(double x, double y, double dir, double angles[]){
    position.setComponent(0, x);
    position.setComponent(2, y);
    position.setComponent(1, 0);
    rudder = 90;
    sail = 0;
    direction = dir;
    angle_sailingPoints[0] = angles[0];
    angle_sailingPoints[1] = angles[1];
    angle_sailingPoints[2] = angles[2];
    memset(speed_sailingPoints, 0, sizeof(speed_sailingPoints));
}

void boat::setPosition(physVector pos){
    position = pos;
}

void boat::setRudder(double r){
    //90 degrees is the center, less is to the left and more is to the right
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

double boat::getSailingPoint(int index){
    return angle_sailingPoints[index];
}

double boat::bestAngle(physVector wind, double startAngle, int tackLimit, physVector target){
    /*
    TODO: Change this function so that it returns
    the best angle that after x timeSteps will
    minimize the distance to target
    */
    if(startAngle < 0){
        startAngle = 360 + startAngle;
    }
    double bestMagnitude = 1000000000000;
    double bestAng = -1;
    for(int i = (int)startAngle; i<(int)startAngle+45; i++){
        double dir = (double)(i%360);
        double speed=0;
        physVector north(3);
        north.setComponent(0, 1);
        double windAngle = (wind*-1.0)%north;
        //compare windAngle to boatAngle
        double BoatWindAngle = abs(windAngle-dir);

        if (BoatWindAngle <= angle_sailingPoints[0]){
            //no go zone
            speed = 0;
        }
        else if(BoatWindAngle > angle_sailingPoints[0] && BoatWindAngle <=angle_sailingPoints[1]){
            double m = (0.95-0.35)/(angle_sailingPoints[1]-angle_sailingPoints[0]);
            double b = 0.35 - m*angle_sailingPoints[0];
            speed = m* BoatWindAngle + b;
        }
        else if(BoatWindAngle > angle_sailingPoints[1] && BoatWindAngle <=angle_sailingPoints[2]){
            speed = 0.95;
        }

        else if(BoatWindAngle > angle_sailingPoints[2]){
            double m = (0.7-0.95)/(180-angle_sailingPoints[2]);
            double b = 0.95 - m*angle_sailingPoints[2];
            speed = m*BoatWindAngle+b;
        }
        speed *= wind.getMagnitude()/60.0;
        physVector directionVector(3);
        directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
        directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed); //Z axis has positive values towards viewer
        physVector pos = position;
        pos = pos + directionVector*((double)tackLimit);
        physVector vectToTarget = target-pos;
        double magnitude = vectToTarget.getMagnitude();

        if(magnitude<bestMagnitude){
            bestMagnitude = magnitude;
            bestAng = dir;
        }
    }
    return bestAng;

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
        BoatWindAngle = 360.0-BoatWindAngle;
    }
    if (BoatWindAngle <= angle_sailingPoints[0]){
        //no go zone
        speed = 0.15;
    }
    else if(BoatWindAngle > angle_sailingPoints[0] && BoatWindAngle <=angle_sailingPoints[1]){
        double m = (0.95-0.35)/(angle_sailingPoints[1]-angle_sailingPoints[0]);
        double b = 0.35 - m*angle_sailingPoints[0];
        speed = m* BoatWindAngle + b;
    }
    else if(BoatWindAngle > angle_sailingPoints[1] && BoatWindAngle <=angle_sailingPoints[2]){
        speed = 0.95;
    }

    else if(BoatWindAngle > angle_sailingPoints[2]){
        //double a1 = 0.7-0.95;
        //double a2 = 180-angle_sailingPoints[2];
        double m = (0.7-0.95)/(180.0-angle_sailingPoints[2]);
        double b = 0.95 - m*angle_sailingPoints[2];
        speed = m*BoatWindAngle+b;
    }
    speed *= wind.getMagnitude()/FPS;
    speed*=sail;

    //account for rudder
    if(rudder < 90){
	    direction-=((90.0-rudder)/100.0);
	    if(direction<0.0001){
		    direction+=360.0;
        }
    }
    else{
	    direction+=((rudder-90.0)/100.0);
        if(direction > 359.9999){
		    direction -=360.0;
	    }
    }
    physVector directionVector(3);
    directionVector.setComponent(0, cos((direction/180.0)*M_PI)*speed);
    directionVector.setComponent(2, sin((direction/180.0)*M_PI)*speed); //Z axis has positive values towards viewer

    position = position + (directionVector);
    if(timeStep%300==0){
        std::cout<<"Rudder: "<<rudder<<" Direction: "<<direction<<" Speed: "<<speed<<" BoatWingAngle: "<<BoatWindAngle<<"\n";
    }
    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*3.141600))/150.0);
}

#endif
