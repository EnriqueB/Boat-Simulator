#ifndef BOAT_H
#define BOAT_H

#include "physVector.h"
#include "battery.h"
#include <string.h>
#include <iostream>
#include <cfloat>
#include <algorithm>

#define FPS 60.0

class boat{

    private:
        physVector position;
        physVector wind;
        physVector tide;
        double rudder;
        double sail;
    	battery bat;
        int targetIndex;
	    double direction;
        double angle_sailingPoints[3];
        double speed_sailingPoints[3];
        double tackAngle;
        double leeway;
        int tackTimer;
        int tackLimit;
        int tackStatus;
        int loopCount;
        long long lastLoop;
        int pilot;
        long long finishTime;

    public:
       boat();
        boat(physVector w, physVector t, double x, double y, double dir,
                double angles[], double speeds[], int indi, double lw);

        //setters
        void setPosition(physVector pos) { position = pos; }
        void setRudder(double r);
        void setSail(double s) { sail = s; }
	    void setDirection(double a);
        void setTargetIndex(int t) { targetIndex = t; }
        void setTackAngle(double t);
        void setTackTimer(int t) { tackTimer = t; }
        void setTackLimit(int t) { tackLimit = t; }
        void setTackStatus(int t) { tackStatus = t; }
        void setLastLoop(long long l) { lastLoop = l; }
        void setPilot(int i) { pilot = i; }
        void setLeeway(double l) { leeway = l; }

        //getters
        physVector getPosition() { return position; }
        double getRudder() { return rudder; }
        double getSail() { return sail; }
        double getDirection() { return direction; }
        double getSailingPoint(int index) { return angle_sailingPoints[index]; }
        int getTargetIndex() { return targetIndex; }
        double getTackAngle() { return tackAngle; }
        int getTackTimer() { return tackTimer; }
        int getTackLimit() { return tackLimit; }
        int getTackStatus() { return tackStatus; }
        int getLoopCount() { return loopCount; }
        long long getLastLoop() { return lastLoop; }
        int getPilot() { return pilot; }
        double getLeeway() { return leeway; }
        long long getFinishTime(){ return finishTime; }

        void completedLoop(long long time) { 
                                loopCount++; 
                                if(loopCount==4)    finishTime = time;
                             }

        void moveBoat(long long timeStep);
        double bestAngle(int minAngle, int maxAngle, int angleStep,
                        int minTack, int maxTack, int tackStep, physVector target,
                        long long &iterations, int &bestTack);
};

boat::boat(){
    position.setDimensions(3);
    rudder = 90;
    sail = 0;
    direction = 0;
    memset(angle_sailingPoints, 0, sizeof(angle_sailingPoints));
    memset(speed_sailingPoints, 0, sizeof(speed_sailingPoints));
    tackLimit = 300;
    tackTimer = 0;
    targetIndex = 0;
    tackStatus = 0;
    loopCount = 0;
    lastLoop = 0;
    leeway = 0;
    finishTime = 18000;
}

boat::boat(physVector w, physVector t, double x, double y, double dir, double angles[], double speeds[], int ind, double lw){
    wind = w;
    tide = t;

    position.setComponent(0, x);
    position.setComponent(2, y);
    position.setComponent(1, 0);
    rudder = 90;
    sail = 0;
    direction = dir;
    angle_sailingPoints[0] = angles[0];
    angle_sailingPoints[1] = angles[1];
    angle_sailingPoints[2] = angles[2];

    speed_sailingPoints[0] = speeds[0];
    speed_sailingPoints[1] = speeds[1];
    speed_sailingPoints[2] = speeds[2];

    tackLimit = 300;
    tackTimer = 0;
    targetIndex = 0;
    tackStatus = 0;
    loopCount = 0;
    lastLoop = 0;
    pilot = ind;
    leeway = lw;
    finishTime = 18000;
}

void boat::setRudder(double r){
    //90 degrees is the center, less is to the left and more is to the right
    rudder = r;
    if(rudder < 20){
	    rudder = 20;
    }
    else if(rudder >160) {
        rudder = 160;
    }
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

void boat::setTackAngle(double t){
    tackAngle = t;
    if(tackAngle >= 360.0){
        tackAngle = tackAngle - 360.0;
    }
    if(tackAngle < 0.0){
        tackAngle = 360 + tackAngle;
    }
}

double boat::bestAngle(int minAngle, int maxAngle, int angleStep,
                int minTack, int maxTack, int tackStep, physVector target,
                long long &iterations, int &bestTack){


    if(minAngle < 0.0){
        minAngle = 360.0 + minAngle;
        maxAngle += 360.0;
    }
    //if(maxAngle >= 360.0)   maxAngle = maxAngle - 360.0;

    if(minTack <0)          minTack = 0;

    if(minTack>maxTack){
        int temp = maxTack;
        maxTack = minTack;
        minTack = temp;
    }

    if(angleStep<=0)        angleStep = 1;
    if(tackStep<=0)         tackStep = 1;

    tackTimer = 0;

    double leewayForce=0;
    iterations = 0;
    double bestMagnitude = DBL_MAX;
    double bestAng = -1;
    bestTack = -1;
    physVector north(3);
    north.setComponent(0, 1);
    double windAngle = (wind*-1.0)%north;
    for(int angle = (int)minAngle; angle<(int)maxAngle; angle+=angleStep){
        double dir = (double)(angle%360);
        double speed=0;

        //compare windAngle to boatAngle
        double BoatWindAngle = abs(windAngle-dir);

        if (BoatWindAngle <= angle_sailingPoints[0]){
            //no go zone
            speed = 0.10;
            leewayForce = 0.3;
        }
        else if(BoatWindAngle > angle_sailingPoints[0] && BoatWindAngle <=angle_sailingPoints[1]){
            double m = (speed_sailingPoints[1]-speed_sailingPoints[1])/(angle_sailingPoints[1]-angle_sailingPoints[0]);
            double b = speed_sailingPoints[0] - m*angle_sailingPoints[0];
            speed = m* BoatWindAngle + b;
            leewayForce = 1-speed+0.15;
        }
        else if(BoatWindAngle > angle_sailingPoints[1] && BoatWindAngle <=angle_sailingPoints[2]){
            speed = speed_sailingPoints[1];
            leewayForce = 0.3;
        }

        else if(BoatWindAngle > angle_sailingPoints[2]){
            double m = (speed_sailingPoints[2]-speed_sailingPoints[1])/(180-angle_sailingPoints[2]);
            double b = speed_sailingPoints[1] - m*angle_sailingPoints[2];
            speed = m*BoatWindAngle+b;
            leewayForce = 0.1;
        }
        speed *= wind.getMagnitude()/FPS;
        physVector directionVector(3);
        directionVector.setComponent(0, cos((dir/180.0)*M_PI)*speed);
        directionVector.setComponent(2, sin((dir/180.0)*M_PI)*speed); //Z axis has positive values towards viewer
        physVector pos;

        physVector leewayVector(3);
        leewayVector.setComponent(0, -1*directionVector.getComponent(2));
        leewayVector.setComponent(2, directionVector.getComponent(0));

        int side = directionVector|(wind*-1);

        if(side==-1){
            //direction is to the left
            leewayVector = leewayVector*-1.0;
        }
        leewayVector = (leewayVector * (leewayForce * leeway))*0.005;

        for(int tack = minTack; tack<maxTack; tack+=tackStep){
            pos = position;
            pos = pos + directionVector*((double)tack);
            pos = pos + leewayVector*((double)tack);

            //check for tide along the path
            //tide is stronger in deep waters (negative x coordinates)
            double origX = position.getComponent(0);
            double movementX = (directionVector.getComponent(0));
            double x = (-1*origX)/movementX;
            int timeDeepWaters = 0;
            if(origX <0){
                //original position is in deep waters
                if(x>tack || x < 0){
                    //boat never reaches shallow waters
                    timeDeepWaters = tack;
                }
                else{
                    //boat reaches shallow waters
                    timeDeepWaters = (int)x;
                }
            }
            else{
                //original position is in shallow waters
                if(x>tack || x< 0){
                    //boat never reaches deep waters
                    timeDeepWaters = 0;
                }
                else{
                    //boat reaches deep waters
                    timeDeepWaters = tack-x;
                }
            }

            pos = pos + (tide*2.5)*((double)timeDeepWaters) + tide*((double)tack-timeDeepWaters);

            physVector vectToTarget = target-pos;
            double magnitude = vectToTarget.getMagnitude();

            if(magnitude<bestMagnitude){
                bestMagnitude = magnitude;
                bestAng = dir;
                bestTack = tack;
            }
            iterations++;
        }
        iterations++;
    }
    tackLimit = bestTack;
    tackAngle = bestAng;
    if(iterations == 0){
        std::cout<<"MinAngle: "<<minAngle<<" maxAngle: "<<maxAngle<<" AngleStep: "<<angleStep;
        std::cout<<" MinTack: "<<minTack<<" maxTack: "<<maxTack<<" tackSte: "<<tackStep<<std::endl;
    }
    return bestAng;
}

void boat::moveBoat(long long timeStep){
    double leewayForce=0;
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
    if (BoatWindAngle < angle_sailingPoints[0]){
        //no go zone
        speed = 0.10;
        leewayForce = 0.3;
    }
    else if(BoatWindAngle >= angle_sailingPoints[0] && BoatWindAngle <=angle_sailingPoints[1]){
        double m = (speed_sailingPoints[1]-speed_sailingPoints[0])/(angle_sailingPoints[1]-angle_sailingPoints[0]);
        double b = speed_sailingPoints[0] - m*angle_sailingPoints[0];
        speed = m* BoatWindAngle + b;
        leewayForce = 1-speed+0.15;
    }
    else if(BoatWindAngle > angle_sailingPoints[1] && BoatWindAngle <=angle_sailingPoints[2]){
        speed = speed_sailingPoints[1];
        leewayForce = 0.3;
    }

    else if(BoatWindAngle > angle_sailingPoints[2]){
        double m = (speed_sailingPoints[2]-speed_sailingPoints[1])/(180.0-angle_sailingPoints[2]);
        double b = speed_sailingPoints[1] - m*angle_sailingPoints[2];
        speed = m*BoatWindAngle+b;
        leewayForce = 0.1;
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

    //leeway:
    //boat is moved sideways due to air force
    physVector leewayVector(3);
    leewayVector.setComponent(0, -1*directionVector.getComponent(2));
    leewayVector.setComponent(2, directionVector.getComponent(0));

    int side = directionVector|(wind*-1);

    if(side==-1){
        //direction is to the left
        leewayVector = leewayVector*-1.0;
    }
    leewayVector = (leewayVector * (leewayForce * leeway))*0.005;
    position = position + leewayVector;

    if(position.getComponent(0) > 0){
        position = position + tide;
    }
    else{
        position = position + tide*2.5;
    }

    //wave movement
    position.setComponent(1, position.getComponent(1)+sin((timeStep/180.0*3.141600))/150.0);
}

#endif
