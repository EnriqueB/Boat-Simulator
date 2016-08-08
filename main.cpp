#include <GL/glut.h>
#include <chrono>
#include "physVector.h"
#include "boat.h"

#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <cmath>
#include <vector>

#define FPS 60.0

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
double w[] = {0.0, 0.0, 6.0};
physVector wind(3, w);
double originalHeading;


void initVectors(){
    srand(time(NULL));
    timeStep=0;
    physVector pos;

    //generate random original positions
    double sailingPoints[] = {10.0,  85.0, 95.0};
    double speedPoints[] = {0.3, 0.95, 0.8};
    double x, y, ang;
    for(int i=0; i<4; i++){
        x= (double)rand() / RAND_MAX;               //HARDCODED
        x = -20 + x*40;
        y=(double)rand()/RAND_MAX;
        y = -20+y*40;
        cout<<x<<" "<<y<<endl;

        ang = (double)rand()/RAND_MAX;
        ang*=360.0;

        boat b(0, 10, 270, sailingPoints, speedPoints);
        boats.push_back(b);
        sailingPoints[0] = sailingPoints[0]+(double)i*5.0;
    }

    //generate tide
    tide.setComponent(0,((double)rand()/RAND_MAX)*.01-0.005);
    tide.setComponent(2,((double)rand()/RAND_MAX)*.01-0.005);
    tide.setComponent(1, 0);

    //generate targets
    for(int i=0; i<4; i++){
        x = (double)rand()/RAND_MAX;
        x = -70 + x*140;
        y = (double)rand()/RAND_MAX;
        y = -70 + y*140;
        targets[i].setComponent(0, x);
        targets[i].setComponent(2, y);
        targets[i].setComponent(1, 1);
    }
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
            glColor3d(1-(double)boatIndex*0.22,1-(double)boatIndex*0.15,1-(double)boatIndex*0.2);
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
    if(timeStep%300 == 0)
        cout<<"Angle to target: "<<angleToTarget<<"   "<<direction<<"    ";
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

    if(timeStep%300==0){
        cout<<"Tacking stat: "<<tackStatus<<" # ";
    }

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
        if(timeStep%300 == 0)
            cout<<"Angle to target: "<<angleToTarget<<"  ";
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
            boats[boatIndex].setSail(0.4); //reduce speed while turning
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
            boats[boatIndex].setSail(0.4);  //reduce speed while turning
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
        cout<<"Begin tack\n";
        physVector vectToTarget = targets[targetIndex]-boats[boatIndex].getPosition();
        physVector directionVector(3);
        directionVector.setComponent(0, cos((boats[boatIndex].getDirection()/180.0)*M_PI));
        directionVector.setComponent(2, sin((boats[boatIndex].getDirection()/180.0)*M_PI));

        //find closest direction to target
        int direction = directionVector|vectToTarget;

        if(direction==-1){
            //boat's direction is to the left
            //check all angles from originalHeading -45 to originalHeading
            tackAngle = boats[boatIndex].bestAngle(wind, originalHeading, tackLimit, targets[targetIndex]);
            tackStatus = 1;
            //If there is no suitable angle, try to tack for a small time in the other direction
            if(tackAngle == -1){
                tackStatus = 2;
                tackLimit = 200;
                tackAngle = boats[boatIndex].bestAngle(wind, originalHeading-45.0, tackLimit, targets[targetIndex]);
                if(tackAngle == -1){
                    //If again no suitable angle is found, try to tack
                    //with a larger margin in the original direction
                    tackAngle = ((int)(originalHeading +55.0)%360);
                    cout<<"\n********\nNo suitable angle found\n********\n";
                }
            }

        }
        else{
            tackAngle = boats[boatIndex].bestAngle(wind, originalHeading-45.0, tackLimit, targets[targetIndex]);
            tackStatus = 2;
            //If there is no suitable angle, try to tack for a small time in the other direction
            if(tackAngle == -1){
                tackStatus = 1;
                tackLimit = 200;
                tackAngle = boats[boatIndex].bestAngle(wind, originalHeading, tackLimit, targets[targetIndex]);
                if(tackAngle == -1){
                    //If again no suitable angle is found, try to tack
                    //with a larger margin in the original direction
                    int temp = (int)(originalHeading- 55.0);
                    if(temp< 0){
                        temp = 360 + temp;
                    }
                    tackAngle = temp;
                    cout<<"\n********\nNo suitable angle found\n********\n";
                }
            }
        }
        cout<<"TackAngle: "<<tackAngle<<endl;

    }
    if(tackTimer > tackLimit){
        cout<<"Finished tack\n";
        tackLimit = 600;
        tackTimer = 0;
        //change tack
        tackStatus = 0;
    }
    if(tackStatus ==1){
        //turn until
        if(abs(tackAngle - boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()+0.5);
        }
        else{
            //keep tack for a while
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    else if(tackStatus == 2){
        if(abs(tackAngle- boats[boatIndex].getDirection()) >= 1.0){
            boats[boatIndex].setRudder(boats[boatIndex].getRudder()-0.5);
        }
        else{
            boats[boatIndex].setRudder(90.0);
            tackTimer++;
        }
    }
    boats[boatIndex].setTackAngle(tackAngle);
    boats[boatIndex].setTackTimer(tackTimer);
    boats[boatIndex].setTackLimit(tackLimit);
    boats[boatIndex].setTackStatus(tackStatus);

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
    if(timeStep%300==0){
        cout<<"AngleDifference: "<<angleDifference<<"  ";
    }

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

        for(int i =0; i<4; i++){
            //move and draw boats
            if(timeStep%300 == 0){
                cout<<"Boat: "<<i<<" ";
            }
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
            exit(0);
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


 //Program entry point

int main(int argc, char *argv[]){
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

    glutMainLoop();

    return EXIT_SUCCESS;
}

