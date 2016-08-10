#include <GL/freeglut.h>
#include <chrono>
#include "physVector.h"
#include "boat.h"

#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <cmath>
#include <vector>

#define FPS 60.0

#define POPULATION_SIZE 50
#define TOURNAMENT_SIZE 5
#define GENERATIONS 100
#define XOVER_RATE 0.9

using namespace std;

//Clock used for enforcing FPS
auto start = chrono::steady_clock::now();

// actual vector representing the camera's direction
float lx=0.0f,lz=-1.0f, cy=0;
// XZ position of the camera
float x=0.0f,z=100.0f;

float red=1.0f, blue=1.0f, green=1.0f;

// angle for rotating triangle
float angle = 0.0f;
float acceleration;
float accelerationVal;

vector <boat> boats;
physVector tide(3);
physVector targets [4];
physVector north(3);

long long timeStep;
long long lastLoop = 0;
double w[] = {0.0, 0.0, 10.0};
physVector wind(3, w);
double originalHeading;

double sailingPoints[][3] = {{10.0,  85.0, 95.0},
                                {15.0, 85.0, 95.0},
                                {20.0, 80.0, 100.0},
                                {25.0, 85.0, 95.0}};
double speedPoints[][3] = {{0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8},
                                {0.3, 0.95, 0.8}};
bool usedSrand = false;
struct INDIVIDUAL{
    int iterations;
    double fitness;
    /*
     * 0: minAngle
     * 1: maxAngle
     * 2: angleStep
     * 3: minTack
     * 4: maxTack
     * 5: tackStep
     */
    int parameters[6];

    INDIVIDUAL(){
        if(!usedSrand){
            srand(time(NULL));
            usedSrand = true;
        }
        fitness = 0;
        iterations = 0;
        parameters[0] = rand()%180;
        parameters[1] = rand()%180;
        parameters[2] = rand()%50 + 1;
        parameters[3] = rand()%100 + 1;
        parameters[4] = rand()%3000 + 1;
        parameters[5] = rand()%100 + 1;

        while((parameters[3] > parameters[4]) || (parameters[3] == parameters[4])){
            parameters[3] = rand()%100 +1;
            parameters[4] = rand()%3000 +1;
        }
    }
};

vector <INDIVIDUAL> individuals(POPULATION_SIZE);

int tournament(bool type){
    //Tournament selection
    //type = true for normal, false for negative tournament
	double fitness = (double)INT32_MAX;
    if(!type)fitness = 0;
	int index = 0;
	for (int i = 0; i < TOURNAMENT_SIZE; i++) {
		int ind = rand() % POPULATION_SIZE;
        //check tournament type
		if ((individuals[ind].fitness< fitness)==type) {
			fitness = individuals[ind].fitness;
			index = ind;
		}
	}
	return index;
}

void generateOffspring(){
    double random = ((double)rand() / RAND_MAX);
    INDIVIDUAL ind;

    if(random < XOVER_RATE){ //crossover
        //pick two parents
        int parent1 = tournament(true);
        int parent2 = tournament(true);

        //TODO do xover

        int index = tournament(false);
        individuals[index] = ind;
    }
    else{                       //mutation
        //pick one parent
        int index = tournament(true);

        //TODO do mutation

        index = tournament(false);
        individuals[index] = ind;
    }
}

void initVectors(){
    timeStep=0;
    physVector pos;

    //generate random original positions
    double x, y, ang;
    for(int j=0; j<POPULATION_SIZE; j++){
        for(int i=0; i<4; i++){
        /*
            x= (double)rand() / RAND_MAX;               //HARDCODED
            x = -20 + x*40;
            y=(double)rand()/RAND_MAX;
            y = -20+y*40;
            cout<<x<<" "<<y<<endl;

            ang = (double)rand()/RAND_MAX;
            ang*=360.0;
        */
            boat b(0.0, 0.0, 270.0, sailingPoints[i], speedPoints[i]);
            boats.push_back(b);
            //sailingPoints[0] = sailingPoints[0];//+(double)i*5.0;
        }
    }

    boat base(0.0, 0.0, 270.0, sailingPoints[1], speedPoints[0]);
    boats.push_back(base);


    //generate tide
    /*
    tide.setComponent(0,((double)rand()/RAND_MAX)*.01-0.005);
    tide.setComponent(2,((double)rand()/RAND_MAX)*.01-0.005);
    tide.setComponent(1, 0);
    */
    //tide.setComponent(0, 0);
    tide.setComponent(2, -0.0005);
    //generate targets
    /*
    for(int i=0; i<2; i++){
        x = (double)rand()/RAND_MAX;
        x = -70 + x*140;
        y = (double)rand()/RAND_MAX;
        y = -70 + y*140;
        targets[i].setComponent(0, x);
        targets[i].setComponent(2, y);
        targets[i].setComponent(1, 1);
    }
    */
    targets[0].setComponent(0,0.0);
    targets[0].setComponent(2,-100.0);
    targets[0].setComponent(1, 0.5);

    targets[1].setComponent(0,0);
    targets[1].setComponent(2,100.0);
    targets[1].setComponent(1,0.5);
    /*
    targets[2].setComponent(0, 100);
    targets[2].setComponent(2, 0);
    targets[2].setComponent(1, 0.5);

    targets[3].setComponent(0, -100);
    targets[3].setComponent(2, 0);
    targets[3].setComponent(1, 0.5);
    north.setComponent(0, 1);
    */
}


void changeSize(int w, int h){
// Prevent a divide by zero, when window is too short
// (you cant make a window of zero width).
    if (h == 0)
        h = 1;
    float ratio = w * 1.0 / h;

    // Use the Projection Matrix
    glMatrixMode(GL_PROJECTION);

    // Reset Matrix
    glLoadIdentity();

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // Set the correct perspective.
    gluPerspective(45.0f, ratio, 0.1f, 100.0f);

    // Get Back to the Modelview
    glMatrixMode(GL_MODELVIEW);
}


void drawBoat(int boatIndex){
    physVector pos = boats[boatIndex].getPosition();
    double ang = 360-boats[boatIndex].getDirection();
    glPushMatrix();
        //glRotated(angle, 0, 1, 0);
        //hull
        glPushMatrix();
            glTranslated(pos.getComponent(0), pos.getComponent(1), pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(1.2, .8, 0.5);
            glColor3d(0, 0, 0);
            glutWireCube(4);
            glColor3d(0.55,0.35, 0);
            glutSolidCube(4);
        glPopMatrix();

        //mast
        glPushMatrix();
            glTranslated(pos.getComponent(0), pos.getComponent(1)+2, pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(0.3, 5, 0.3);
            glColor3d(0, 0, 0);
            glutWireCube(0.8);
            glColor3d(0.65,0.35,0);
            glutSolidCube(0.8);
        glPopMatrix();

        //cloth sail
        glPushMatrix();
            glTranslated(pos.getComponent(0)+0.5, pos.getComponent(1)+3.6, pos.getComponent(2));
            glRotated(ang, 0, 1, 0);
            glScalef(0.3, 0.9, 1);
            glColor3d(0, 0, 0);
            glutWireCube(0.8);
            if(boatIndex==boats.size()-1) glColor3d(0.0,0.0,0.0);
            else glColor3d(1,1,1);
            glutSolidCube(0.8);
        glPopMatrix();

        //sphere at the front of the boat
        glPushMatrix();
            glTranslated(pos.getComponent(0), pos.getComponent(1), pos.getComponent(2)+3);
            glRotated(ang, 0, 1, 0);
            glColor3d(0,0,0);
            //glutWireSphere(0.5, 20, 3);
            glColor3d(1, 0, 0);
            //glutSolidSphere(0.51, 20, 3);
        glPopMatrix();
        glTranslated(0,0,0);
    glPopMatrix();

}


void chaseTarget(int boatIndex){
    int targetIndex = boats[boatIndex].getTargetIndex();
    //move towards target
    //find the angle difference to the target
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();
    physVector directionVector(3);
    directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
    directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
    double angleToTarget = directionVector&vectToTarget;

    //find closest direction to target
    int direction = directionVector|vectToTarget;
    //if(timeStep%300 == 0)
     //   cout<<"Angle to target: "<<angleToTarget<<"   "<<direction<<"    ";
    //modify direction to reach target

    if(angleToTarget > -3 && angleToTarget < 3){
        boats[boatIndex].setRudder(90);
    }
    if(direction == -1){
        //boat's direction is to the left, move towards the right
        boats[boatIndex].setRudder(boats[boatIndex].getRudder()+(0.5));
        boats[boatIndex].setSail(0.4);
    }
    else{
        //boat's direction is to the right, move towards the left
        boats[boatIndex].setRudder(boats[boatIndex].getRudder()-(0.5));
        boats[boatIndex].setSail(0.4);
    }
    if(angleToTarget > -10 && angleToTarget <10){
        boats[boatIndex].setSail(1);
    }
}

void baseTacking(int boatIndex){
    /*
     * 1.- Turn towards target
     * 2.- Pick a random side and commit to a tack
     * 3.- Evaluate after tack, if target is still upwind, tack to the other side
     */
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    double tackAngle = boats[boatIndex].getTackAngle();
    int tackTimer = boats[boatIndex].getTackTimer();
    int tackLimit = boats[boatIndex].getTackLimit();

    //if(timeStep%300==0){
    //    cout<<"Tacking stat: "<<tackStatus<<" # ";
    //}

    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();
    physVector directionVector(3);
    directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
    directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
    double angleToTarget = directionVector&vectToTarget;
    double trueWindAngle = (wind*-1)%north;
    int direction = directionVector|vectToTarget;

    if(tackStatus == 0){
        //turn towards target
        //find closest direction to target
        //if(timeStep%300 == 0)
        //    cout<<"Angle to target: "<<angleToTarget<<"  ";
        //modify direction to reach target
        if(angleToTarget > -2 && angleToTarget < 2){
            boats[boatIndex].setRudder(90);
            //pick a side at random
            tackStatus = rand()%2 + 1;
            if(tackStatus==1){
                tackAngle = trueWindAngle + boats[boatIndex].getSailingPoint(0) + 5.0;
                if(tackAngle>=360.0){
                    tackAngle = tackAngle - 360.0;
                }
            }
            else{
                tackAngle = trueWindAngle - boats[boatIndex].getSailingPoint(0) - 5.0;
                if(tackAngle<0.0){
                    tackAngle = 360+tackAngle;
                }
            }
        }
        else{
            boats[boatIndex].setSail(0.2); //reduce speed while turning
            //face target
            if(direction == -1){
                //boat's direction is to the left, move towards the right
                boats[boatIndex].setRudder(boats[boatIndex].getRudder() + 0.5);
            }
            else{
                //boat's direction is to the right, move towards the left
                boats[boatIndex].setRudder(boats[boatIndex].getRudder() - 0.5);
            }
        }
    }
    if(tackTimer == tackLimit){
        double trueTargetAngle = (vectToTarget)%north;
        double angleDifference = abs(trueWindAngle-trueTargetAngle);
        if(angleDifference>180.0){
            angleDifference = 360-angleDifference;
        }
        if(angleDifference >=30.0){
            tackStatus = 0;
            tackTimer = 0;
            tackLimit = 300;
            //boat can sail towards target
            chaseTarget(boatIndex);
        }
        else{
            tackLimit = 600;
            tackTimer = 0;
            if(tackStatus == 1){
                tackStatus = 2;
                tackAngle = trueWindAngle-boats[boatIndex].getSailingPoint(0) - 5.0;
                if(tackAngle < 0.0){
                    tackAngle = 360.0 + tackAngle;
                }

            }
            else{
                tackStatus = 1;
                tackAngle = trueWindAngle + boats[boatIndex].getSailingPoint(0) + 5.0;
                if(tackAngle >=360.0){
                    tackAngle = tackAngle-360.0;
                }
            }
        }
    }
    if(tackStatus ==1){
        //turn until facing direction
        if(abs(tackAngle - boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setSail(0.3); //reduce speed while turning
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()+0.5);
        }
        else{
            boats[boatIndex].setSail(1);
            //keep tack for a while
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    else if(tackStatus == 2){
        if(abs(tackAngle- boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setSail(0.3);  //reduce speed while turning
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()-0.5);
        }
        else{
            boats[boatIndex].setSail(1);
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    boats[boatIndex].setTackAngle(tackAngle);
    boats[boatIndex].setTackTimer(tackTimer);
    boats[boatIndex].setTackLimit(tackLimit);
    boats[boatIndex].setTackStatus(tackStatus);
}

void tackManager(int boatIndex){
    /*
    Check what side to start tacking (closest one to boat's direction)
    From there, check what angle would give the best speed
    Follow that until tack limit, repeat
    */
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    double tackAngle = boats[boatIndex].getTackAngle();
    int tackTimer = boats[boatIndex].getTackTimer();
    int tackLimit = boats[boatIndex].getTackLimit();


    if(tackStatus == 0){
        //obtain side
        //cout<<"Begin tack\n";
        int iterations = 0;
        int bestTack = 0;
        //tackAngle = boats[boatIndex].bestAngle(wind, tide, 0, 359, 5, 10, 3000, 20, targets[targetIndex], iterations, bestTack);
        double targetAngle = (targets[targetIndex])%north;
        int minAngle = targetAngle - individuals[boatIndex/4].parameters[0];
        int maxAngle = targetAngle + individuals[boatIndex/4].parameters[1];
        int angleStep = individuals[boatIndex/4].parameters[2];
        int minTack = individuals[boatIndex/4].parameters[3];
        int maxTack = individuals[boatIndex/4].parameters[4];
        int tackStep = individuals[boatIndex/4].parameters[5];
        tackAngle = boats[boatIndex].bestAngle(wind, tide, minAngle, maxAngle, angleStep, minTack, maxTack, tackStep, targets[targetIndex], iterations, bestTack);
        individuals[boatIndex/4].iterations = iterations;
        tackLimit = bestTack;
        cout<<"Boat: "<<boatIndex<<" TackAngle: "<<tackAngle<<" Iterations: "<<iterations<<" BestTack: "<<bestTack<<endl;
        tackStatus = 1;
    }
    if(tackTimer > tackLimit){
        //cout<<"Finished tack\n";
        tackTimer = 0;
        //change tack
        tackStatus = 0;
    }
    if(tackStatus == 1){
        //turn until
        if(abs(tackAngle - boats[boatIndex].getDirection()) >= 1){
            physVector directionVector(3);
            directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
            directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));
            physVector wantedDirection(3);
            wantedDirection.setComponent(0, cos((tackAngle/180.0)*M_PI));
            wantedDirection.setComponent(2, sin((tackAngle/180.0)*M_PI));

            //find closest direction to target
            int direction = directionVector|wantedDirection;
            boats[boatIndex].setSail(0.4);

            if(direction == -1){
                //boat direction is to the left

                boats[boatIndex].setRudder(boats[boatIndex].getRudder()+0.8);

            }
            else{
                boats[boatIndex].setRudder(boats[boatIndex].getRudder()-0.8);
            }
        }
        else{
            //keep tack for a while
            boats[boatIndex].setSail(1);
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    boats[boatIndex].setTackAngle(tackAngle);
    boats[boatIndex].setTackTimer(tackTimer);
    boats[boatIndex].setTackLimit(tackLimit);
    boats[boatIndex].setTackStatus(tackStatus);
}

void advancedMovement(int boatIndex){
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();

    double trueWindAngle = (wind*-1)%north;
    double trueTargetAngle = (vectToTarget)%north;
    double angleDifference = abs(trueWindAngle-trueTargetAngle);

    if(vectToTarget.getMagnitude()<3.0){
        //reached target, move to next;
        if(targetIndex==0){
            int loopCount = boats[boatIndex].getLoopCount();
            cout<<"************************\n";
            cout<<"Boat: "<<boatIndex<<" ";
            cout<<"Loop count: "<<loopCount;
            cout<<" In: "<<(double)(timeStep-boats[boatIndex].getLastLoop())/FPS<<" seconds\n";
            cout<<"************************\n";
            boats[boatIndex].completedLoop();
            boats[boatIndex].setLastLoop(timeStep);
        }
        targetIndex=(++targetIndex%2);
        boats[boatIndex].setTargetIndex(targetIndex);
    }
    if(angleDifference >=35.0){
        tackStatus = 0;
        //boat can sail towards target
        chaseTarget(boatIndex);
        boats[boatIndex].setTackStatus(tackStatus);
    }
    else{
        tackManager(boatIndex);
    }
}

void ruleSet(int boatIndex){
    /*
     * 1.- Understand the situation:
     *          Distance to target
     *          Angle to target
     *          Direction of the wind
     * 2.- Depending on direction of the wind:
     *          If the desired direction is inside
     *              the no-go zone, then the boat
     *              must tack to reach its destination
     *          If the desired direction is not inside
     *              the no-go zone, then a direct
     *              approach can be used.
     */
    int targetIndex = boats[boatIndex].getTargetIndex();
    int tackStatus = boats[boatIndex].getTackStatus();
    int tackTimer = boats[boatIndex].getTackTimer();
    int tackLimit = boats[boatIndex].getTackLimit();
    //obtain true wind angle
    double trueWindAngle = (wind*-1)%north;
    physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();

    if(vectToTarget.getMagnitude()<3.0){
        //reached target, move to next;
        if(targetIndex==0){
            int loopCount = boats[boatIndex].getLoopCount();
            cout<<"************************\n";
            cout<<"Boat: "<<boatIndex<<" ";
            cout<<"Loop count: "<<loopCount;
            cout<<" In: "<<(double)(timeStep-boats[boatIndex].getLastLoop())/FPS<<" seconds\n";
            cout<<"************************\n";
            boats[boatIndex].completedLoop();
            boats[boatIndex].setLastLoop(timeStep);
        }
        targetIndex=(++targetIndex%2);
        boats[boatIndex].setTargetIndex(targetIndex);
    }

    //obtain vectToTarget angle
    double trueTargetAngle = (vectToTarget)%north;

    double angleDifference = abs(trueWindAngle-trueTargetAngle);

    if(angleDifference>180.0){
        angleDifference = 360-angleDifference;
    }
    //if(timeStep%300==0){
       // cout<<"AngleDifference: "<<angleDifference<<"  ";
    //}

    if(angleDifference >=30.0 && tackStatus == 0){
        tackStatus = 0;
        tackTimer = 0;
        tackLimit = 300;
        //boat can sail towards target
        chaseTarget(boatIndex);
        boats[boatIndex].setTackStatus(tackStatus);
        boats[boatIndex].setTackTimer(tackTimer);
        boats[boatIndex].setTackLimit(tackLimit);
    }
    else{
        //If the angle towards the wind is lower than 30, tack
        if(tackStatus == 0){
            originalHeading = trueTargetAngle;
        }
        baseTacking(boatIndex);
    }
}

static void display(void){
    if(timeStep >= 18000){
        glutLeaveMainLoop();
    }
    auto now = chrono::steady_clock::now();
    chrono::duration<double> diff = now-start;

    //60 FPS
    if(diff.count() >  1.0/FPS){
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glFrustum (-1.0, 1.0, -1.0, 1.0, 1.5, 2000.0);
        glMatrixMode(GL_MODELVIEW);
        const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
        const double a = t*90.0;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLoadIdentity();
        // Set the camera
        gluLookAt(	x, 10.0f+cy, z,
        x+lx, 10.0f, z+lz,
        0.0f, 1.0f, 0.0f);

        // Draw ground

        //????
        //glColor3f(0,0,.8);
        glClearColor(0, 0, 1, 1);
        glBegin(GL_QUADS);
            glColor3d(0, 0, 0.6);
            glVertex3f(-1000.0f, -0.05f, -1000.0f);
            glVertex3f(-1000.0f, -0.05f, 1000.0f);
            glVertex3f( 1000.0f, -0.05f, 1000.0f);
            glVertex3f( 1000.0f, -0.05f, -1000.0f);
        glEnd();

        glColor3d(0.65,0.35,0);
        physVector pos;

        //draw targets
        for(int i=0; i<2; i++){
            glPushMatrix();
                glTranslated(targets[i].getComponent(0), targets[i].getComponent(1), targets[i].getComponent(2));
                glColor3d(0, 0, 0);
                glutWireSphere(1, 10, 10);
                glColor3d(1, (double)i*0.25, (double)i*0.2);
                glutSolidSphere(1, 10, 10);
            glPopMatrix();
        }

        for(int i =0; i<boats.size(); i++){
            //move and draw boats
            if(i<boats.size()-1)
               advancedMovement(i);
            else
                ruleSet(i);
            
            boats[i].moveBoat(wind, tide, timeStep);
            drawBoat(i);
        }
        glutSwapBuffers();
        timeStep++;
	    start = chrono::steady_clock::now();
    }
}


static void key(unsigned char key, int xx, int yy){
    float fraction = 0.1f;
    switch (key){
        case 27 :
        case 'q':
            glutLeaveMainLoop();
            break;
    }

    glutPostRedisplay();
}

static void idle(void){
    glutPostRedisplay();
}

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void processSpecialKeys(int key, int xx, int yy){

    float fraction = 1.0f;

    switch (key) {
        case GLUT_KEY_LEFT :
            angle -= 0.01f;
            lx = sin(angle);
            lz = -cos(angle);
            break;
        case GLUT_KEY_RIGHT :
            angle += 0.01f;
            lx = sin(angle);
            lz = -cos(angle);
            break;
        case GLUT_KEY_UP :
            x += lx * fraction;
            z += lz * fraction;
            //cy += fraction/10;
            break;
        case GLUT_KEY_DOWN :
            x -= lx * fraction;
            z -= lz * fraction;
            //cy -=fraction/10;
            break;
    }
}


void initAndRun(int argc, char *argv[]){
    glutInit(&argc, argv);
    glutInitWindowSize(1024,768);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("GLUT Shapes");

    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutReshapeFunc(changeSize);
    glutSpecialFunc(processSpecialKeys);
    glutIdleFunc(idle);


    glClearColor(0.9,.9,.9,1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();
}

 //Program entry point

int main(int argc, char *argv[]){

    /*
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
*/

    initVectors();
    initAndRun(argc, argv);
    /*
    int p;
    cin>>p;

    initAndRun(argc, argv);
    */
    //check fitness
    for(int i=0; i<POPULATION_SIZE; i++){
        double fitness = 0;
        for(int j=0; j<4; j++){
            fitness+=boats[i+j].getLoopCount();
        }
        individuals[i].fitness = fitness/4.0 - 0.001*((double)individuals[i].iterations);
    }

    for(int i=0; i<POPULATION_SIZE; i++){
        cout<<"Fitness: "<<individuals[i].fitness<<" Iterations: "<<individuals[i].iterations<<endl;
        cout<<"MinAngle: "<<individuals[i].parameters[0]<<" MaxAngle: "<<individuals[i].parameters[1]<<" AngleStep: "<<individuals[i].parameters[2]<<endl;
        cout<<"MinTack: "<<individuals[i].parameters[3]<<" MaxTack: "<<individuals[i].parameters[4]<<" TackStep: "<<individuals[i].parameters[5]<<endl<<endl;
    }

    return EXIT_SUCCESS;
}

